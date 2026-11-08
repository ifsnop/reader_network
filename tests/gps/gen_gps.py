#!/usr/bin/env python3
"""Generador de ficheros .gps sintéticos para probar las utilidades de src/utils.

Escribe un fichero con el mismo formato que graba reader_network:

- cabecera de --header bytes a 0xCD (2200 por defecto, como reader_network);
- por cada registro: --pre bytes a cero, un datablock y el trailer gps de 10
  bytes de reader_network.c (grupo multicast 239.255.0.1, longitud del
  datablock, hora en 1/128 s desde las 00:00 UTC en los bytes 6-8 y la
  marca 0xCD).

Los datablocks son CAT048 con una longitud aleatoria de 20-300 bytes y un
contenido de relleno que NO es ASTERIX válido: sirven para filtertime_s,
joingps*, filtercat*, cleanast*..., que solo miran la categoría, la
longitud y el trailer, pero no para los descodificadores.

Las horas son monótonas, una cada 1/--rate s desde --start, y vuelven a 0
al cruzar la medianoche, como en una grabación real (reader_network fecha
cada paquete con gettimeofday() al llegar). Por defecto empieza a las 23:55
y dura 24 h 15 min, así que ejercita el ajuste de medianoche de
filtertime_s. La semilla es fija: con los mismos argumentos sale siempre
el mismo fichero, byte a byte.

Ejemplos:
  gen_gps.py day.gps                       # 24 h a 100 registros/s, ~1,5 GB
  gen_gps.py small.gps --rate 200 --start 23:50 --hours 3 --pre 4
  gen_gps.py raw.gps --start 22:00 --hours 3 --header 0
"""
import argparse
import random
import struct


def parse_start(value):
    """HH:MM, HH:MM:SS o segundos desde las 00:00."""
    if ":" in value:
        parts = [float(p) for p in value.split(":")]
        while len(parts) < 3:
            parts.append(0.0)
        return parts[0] * 3600 + parts[1] * 60 + parts[2]
    return float(value)


def main():
    ap = argparse.ArgumentParser(
        description="Genera un fichero .gps sintético (ver la cabecera del script).",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    ap.add_argument("out", help="fichero de salida")
    ap.add_argument("--rate", type=int, default=100, help="registros por segundo")
    ap.add_argument("--start", type=parse_start, default=23 * 3600 + 55 * 60,
                    help="hora del primer registro (HH:MM[:SS] o segundos)")
    ap.add_argument("--hours", type=float, default=24.25, help="duración en horas")
    ap.add_argument("--pre", type=int, default=0, help="bytes antes de cada datablock")
    ap.add_argument("--header", type=int, default=2200, help="bytes de cabecera")
    ap.add_argument("--seed", type=int, default=1, help="semilla del generador aleatorio")
    args = ap.parse_args()

    rng = random.Random(args.seed)
    payload = bytes(rng.getrandbits(8) for _ in range(400))
    pre = b"\x00" * args.pre
    n = int(args.hours * 3600 * args.rate)

    with open(args.out, "wb", buffering=8 << 20) as f:
        f.write(b"\xcd" * args.header)
        chunk = []
        for i in range(n):
            t = args.start + i / args.rate
            raw = int(t * 128) % (86400 * 128)  # vuelve a 0 a medianoche
            size = rng.randint(20, 300)
            db = bytes([48]) + struct.pack(">H", size) + payload[:size - 3]
            trailer = (bytes([239, 255, 0, 1]) + struct.pack(">H", size)
                       + raw.to_bytes(3, "big") + b"\xcd")
            chunk.append(pre + db + trailer)
            if len(chunk) == 100000:
                f.write(b"".join(chunk))
                chunk = []
        f.write(b"".join(chunk))


if __name__ == "__main__":
    main()
