#!/usr/bin/env python3
"""Comprueba la salida de joingps_s contra sus entradas.

    check_join.py out.gps in1.gps in2.gps [... inN.gps] [--wraps K]

- La cabecera (2200 bytes) de la salida es la de in1.
- La salida tiene exactamente los registros de todas las entradas (ni falta
  ni sobra ninguno).
- La salida es, byte a byte, la mezcla de referencia con la regla de
  joingps_s, escrita aquí aparte: el primer registro de cada entrada se
  coloca a menos de 12 h del primero de la entrada anterior, dentro de cada
  entrada un retroceso de más de 12 h es un cruce de medianoche (+1 día), y
  con la misma hora va primero la entrada posterior.
- Sin mirar esa regla: la hora gps de la salida no retrocede nunca, salvo
  al cruzar la medianoche. Con --wraps K, además, la cruza exactamente K
  veces.

Lee los ficheros enteros en memoria: usar con ficheros de prueba pequeños
(gen_gps.py con --hours de unos minutos). Sale con 1 si algo falla.
"""
import argparse
import collections
import sys

HEADER = 2200
POST = 10
DAY = 86400 * 128
HALF = DAY // 2


def read_records(path):
    """cabecera y lista de (hora gps en 1/128 s, bytes del registro)"""
    with open(path, "rb") as fh:
        data = fh.read()
    header = data[:HEADER]
    recs = []
    off = HEADER
    while off + 3 <= len(data):
        size = (data[off + 1] << 8) | data[off + 2]
        if size < 3 or off + size + POST > len(data):
            break
        end = off + size + POST
        post = data[off + size:end]
        recs.append(((post[6] << 16) | (post[7] << 8) | post[8], data[off:end]))
        off = end
    return header, recs


def reference(inputs):
    keyed = []
    ref = None
    for idx, recs in enumerate(inputs):
        if not recs:
            continue
        day = 0
        first = recs[0][0]
        if ref is not None:
            while first + day < ref - HALF:
                day += DAY
            while first + day >= ref + HALF:
                day -= DAY
        ref = first + day
        prev = -1
        for pos, (ts, rec) in enumerate(recs):
            if prev >= 0 and prev - ts > HALF:
                day += DAY
            prev = ts
            keyed.append((ts + day, -idx, pos, rec))
    keyed.sort()
    return [k[3] for k in keyed]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("inputs", nargs="+")
    ap.add_argument("--wraps", type=int, default=None,
                    help="medianoches que tiene que cruzar la salida")
    args = ap.parse_args()

    ok = True
    out_header, out = read_records(args.out)
    ins = [read_records(p) for p in args.inputs]

    if out_header != ins[0][0]:
        print("FAIL: la cabecera no es la de la entrada 1")
        ok = False

    want = collections.Counter(rec for _, recs in ins for _, rec in recs)
    got = collections.Counter(rec for _, rec in out)
    if want != got:
        print("FAIL: registros distintos: faltan %d, sobran %d"
              % (sum((want - got).values()), sum((got - want).values())))
        ok = False

    if [rec for _, rec in out] != reference([recs for _, recs in ins]):
        print("FAIL: la salida no es la mezcla de referencia")
        ok = False

    wraps = back = 0
    for (a, _), (b, _) in zip(out, out[1:]):
        if b < a:
            if a - b > HALF:
                wraps += 1
            else:
                back += 1
    if back:
        print("FAIL: la hora retrocede %d veces sin cruzar la medianoche" % back)
        ok = False
    if args.wraps is not None and wraps != args.wraps:
        print("FAIL: la salida cruza la medianoche %d veces, se esperaban %d"
              % (wraps, args.wraps))
        ok = False

    print("%s: %d registros de %d entradas, %d cruces de medianoche"
          % ("OK" if ok else "FAIL", len(out), len(ins), wraps))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
