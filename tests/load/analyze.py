#!/usr/bin/env python3
"""Compara lo enviado por sender.py con lo grabado por reader_network
(formato gps: cabecera de 2200 bytes 0xCD + [datablock + 10 postbytes]*)."""
import argparse
import bz2
import collections
import json


def read_gps(path):
    opener = bz2.open if path.endswith(".bz2") else open
    with opener(path, "rb") as fh:
        data = fh.read()
    off = 2200
    while off + 3 <= len(data):
        size = (data[off + 1] << 8) | data[off + 2]
        if size < 3 or off + size + 10 > len(data):
            break
        yield data[off:off + size]
        off += size + 10


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--summary", required=True)
    ap.add_argument("--file", required=True)
    args = ap.parse_args()

    with open(args.summary) as fh:
        summ = json.load(fh)
    flows = summ["flows"]
    seen = [collections.Counter() for _ in range(flows)]
    noise_written = 0
    try:
        for db in read_gps(args.file):
            if db[0] != 48 or len(db) < 14:
                continue
            sac, sic = db[4], db[5]
            seq = (db[6] << 16) | (db[7] << 8) | db[8]
            if sac == 2:
                noise_written += 1
            elif sac == 1 and sic < flows:
                seen[sic][seq] += 1
    except FileNotFoundError:
        pass

    lost = dup = 0
    flows_with_loss = 0
    worst = 0.0
    loss_per_flow = []
    for f in range(flows):
        n = summ["sent"][f]
        got = sum(1 for s in seen[f] if s < n)
        l = n - got
        lost += l
        dup += sum(c - 1 for c in seen[f].values() if c > 1)
        pct = 100.0 * l / n if n else 0.0
        loss_per_flow.append(pct)
        if l:
            flows_with_loss += 1
        worst = max(worst, pct)
    total = summ["sent_total"]
    print(json.dumps({
        "sent": total,
        "achieved_pkt_s": round(summ["achieved_pkt_s"]),
        "lost": lost,
        "lost_pct": round(100.0 * lost / total, 2) if total else 0.0,
        "dup_written": dup,
        "flows_with_loss": flows_with_loss,
        "worst_flow_loss_pct": round(worst, 1),
        "flow0_loss_pct": round(loss_per_flow[0], 1) if flows else 0.0,
        "other_flows_loss_pct": round(sum(loss_per_flow[1:]) / (flows - 1), 1) if flows > 1 else 0.0,
        "noise_written": noise_written,
    }))


if __name__ == "__main__":
    main()
