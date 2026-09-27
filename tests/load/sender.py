#!/usr/bin/env python3
"""Emisor de carga multicast para reader_network (lo lanza run_load.sh).

Cada flujo es un grupo multicast distinto (239.255.X.Y) y cada paquete UDP
lleva un datablock CAT048 con N registros. En cada registro:
SAC=1, SIC=numero de flujo, TOD=numero de secuencia del paquete en ese flujo,
lo que permite a analyze.py detectar perdidas y duplicados por flujo.
"""
import argparse
import collections
import json
import socket
import struct
import time


def group_addr(i):
    return "239.255.%d.%d" % (1 + i // 250, 1 + i % 250)


def datablock(flow, seq, records, sac=1):
    # FSPEC 0xF0 -> I048/010 (SAC/SIC), I048/140 (TOD), I048/020 (1 byte, FX=0), I048/040 (4 bytes)
    rec = (struct.pack(">BBB", 0xF0, sac, flow & 0xFF)
           + struct.pack(">I", seq & 0xFFFFFF)[1:]
           + b"\x20"
           + struct.pack(">HH", flow & 0xFFFF, seq & 0xFFFF))
    body = rec * records
    return struct.pack(">BH", 48, 3 + len(body)) + body


def mcast_socket(src, iface):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
    s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(iface))
    s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4 * 1024 * 1024)
    s.bind((src, 0))
    return s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--flows", type=int, required=True)
    ap.add_argument("--rate", type=float, required=True, help="paquetes/s por flujo")
    ap.add_argument("--records", type=int, default=1, help="registros CAT048 por paquete")
    ap.add_argument("--duration", type=float, required=True, help="segundos")
    ap.add_argument("--port", type=int, default=5000)
    ap.add_argument("--src", default="127.0.0.1")
    ap.add_argument("--iface", default="127.0.0.1")
    ap.add_argument("--dup-delay", type=float, default=0.0,
                    help="reenvia cada paquete (identico) N segundos despues, como una segunda red scrm")
    ap.add_argument("--noise-rate", type=float, default=0.0,
                    help="paquetes/s desde una ip de origen NO configurada (127.0.0.2) al grupo del flujo 0")
    ap.add_argument("--malformed-at", type=float, default=-1.0,
                    help="segundo en que se envia un datablock con tamaño 0 al flujo 0")
    ap.add_argument("--summary", required=True)
    args = ap.parse_args()

    groups = [group_addr(i) for i in range(args.flows)]
    s = mcast_socket(args.src, args.iface)
    noise = mcast_socket("127.0.0.2", args.iface) if args.noise_rate > 0 else None

    sent = [0] * args.flows
    dups = collections.deque()
    dup_sent = noise_sent = 0
    malformed_sent = False
    tick = 1.0 / args.rate
    noise_acc = 0.0
    start = time.time()
    next_t = start

    while True:
        now = time.time()
        if now - start >= args.duration:
            break
        if now < next_t:
            time.sleep(next_t - now)
            continue
        for f in range(args.flows):
            pkt = datablock(f, sent[f], args.records)
            s.sendto(pkt, (groups[f], args.port))
            if args.dup_delay > 0:
                dups.append((now + args.dup_delay, f, pkt))
            sent[f] += 1
        while dups and dups[0][0] <= now:
            _, f, pkt = dups.popleft()
            s.sendto(pkt, (groups[f], args.port))
            dup_sent += 1
        if noise is not None:
            noise_acc += args.noise_rate * tick
            while noise_acc >= 1.0:
                noise.sendto(datablock(0, noise_sent, 1, sac=2), (groups[0], args.port))
                noise_sent += 1
                noise_acc -= 1.0
        if args.malformed_at >= 0 and not malformed_sent and now - start >= args.malformed_at:
            s.sendto(b"\x30\x00\x00" + b"\x00" * 8, (groups[0], args.port))
            malformed_sent = True
        next_t += tick

    # enviar los duplicados pendientes para que todo paquete tenga su copia
    while dups:
        due, f, pkt = dups.popleft()
        delay = due - time.time()
        if delay > 0:
            time.sleep(delay)
        s.sendto(pkt, (groups[f], args.port))
        dup_sent += 1

    elapsed = time.time() - start
    total = sum(sent)
    with open(args.summary, "w") as fh:
        json.dump({
            "flows": args.flows,
            "sent": sent,
            "sent_total": total,
            "elapsed": elapsed,
            "achieved_pkt_s": total / elapsed if elapsed else 0,
            "records_per_pkt": args.records,
            "dup_sent": dup_sent,
            "noise_sent": noise_sent,
            "malformed_sent": malformed_sent,
        }, fh)


if __name__ == "__main__":
    main()
