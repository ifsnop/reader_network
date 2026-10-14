#!/usr/bin/env python3
"""Comprueba el escenario "reload" de run_load.sh (recarga con SIGHUP en mode_continuous).

Los ficheros grabados se toman del log del lector, en el orden en que se abrieron
("output data to file (2):..."); los abiertos despues de "reload applied" son los
de la configuracion nueva. Se graban 0..99, y tras la recarga 50..149:
  - 50..99 (siguen): secuencia completa 0..n-1 a traves de todos los ficheros,
    sin huecos ni duplicados;
  - 0..49 (quitados): 0..m-1 antes del corte y nada despues;
  - 100..149 (añadidos): nada antes del corte y m..n-1 despues.
Ademas, los ficheros cerrados antes del corte se suben a ftpA y el que se cierra en
el corte y los siguientes a ftpB (con el mismo contenido que en disco).
"""
import argparse
import json
import os
import re

from analyze import read_gps

KEPT = range(50, 100)
REMOVED = range(0, 50)
ADDED = range(100, 150)


def seqs_by_flow(paths, flows):
    seen = [[] for _ in range(flows)]
    for path in paths:
        for db in read_gps(path):
            if db[0] != 48 or len(db) < 14 or db[4] != 1 or db[5] >= flows:
                continue
            seen[db[5]].append((db[6] << 16) | (db[7] << 8) | db[8])
    return seen


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--summary", required=True)
    ap.add_argument("--log", required=True)
    ap.add_argument("--ftp-a", required=True)
    ap.add_argument("--ftp-b", required=True)
    args = ap.parse_args()

    with open(args.summary) as fh:
        summ = json.load(fh)
    flows, sent = summ["flows"], summ["sent"]
    with open(args.log, errors="replace") as fh:
        log = fh.read().splitlines()

    before, after, applied = [], [], False
    for line in log:
        if "reload applied" in line:
            applied = True
        m = re.search(r"output data to file \(2\):(\S+)", line)
        if m:
            (after if applied else before).append(m.group(1))

    errors = []
    lost = dup = 0
    flows_with_loss = 0
    worst = 0.0
    cut_removed, cut_added = [], []
    seq_before = seqs_by_flow(before, flows)
    seq_after = seqs_by_flow(after, flows)

    def account(f, got, expected):
        nonlocal lost, dup, flows_with_loss, worst
        l = len(set(expected) - set(got))
        d = len(got) - len(set(got))
        lost += l
        dup += d
        if l:
            flows_with_loss += 1
            worst = max(worst, 100.0 * l / len(expected) if expected else 0.0)
        if got != list(expected):
            errors.append("flow %d: %d recorded, expected %d..%d (lost %d, dup %d)"
                          % (f, len(got), expected[0] if expected else -1,
                             expected[-1] if expected else -1, l, d))

    for f in KEPT:
        account(f, seq_before[f] + seq_after[f], range(sent[f]))
    for f in REMOVED:
        if seq_after[f]:
            errors.append("flow %d removed but recorded after the cut" % f)
        m = len(seq_before[f])
        cut_removed.append(m)
        account(f, seq_before[f], range(m))
    for f in ADDED:
        if seq_before[f]:
            errors.append("flow %d added but recorded before the cut" % f)
        m = seq_after[f][0] if seq_after[f] else sent[f]
        cut_added.append(m)
        account(f, seq_after[f], range(m, sent[f]))

    def count(text):
        return sum(1 for line in log if text in line)

    if not applied:
        errors.append("no 'reload applied' in log")
    if not before or not after:
        errors.append("no files before/after the reload (%d/%d)" % (len(before), len(after)))
    if count("reload rejected") < 1:
        errors.append("syntax error not rejected")
    if count("replacing the previous pending") < 1:
        errors.append("second SIGHUP did not replace the pending configuration")
    if count("'timed' changed") < 1:
        errors.append("no warning for 'timed' changed")
    if count("reload applied") != 1:
        errors.append("'reload applied' %d times, expected 1" % count("reload applied"))
    sockets = [l for l in log if re.search(r"reload: \d+ input sockets", l)]
    expected_sockets = "reload: 100 input sockets (50 kept open, 50 opened, 50 closed)"
    if not sockets or expected_sockets not in sockets[0]:
        errors.append("sockets: %s (expected '%s')" % (sockets[0] if sockets else "none", expected_sockets))
    if cut_removed and max(cut_removed) - min(cut_removed) > 2:
        errors.append("removed flows stop at different points: %d..%d" % (min(cut_removed), max(cut_removed)))
    if cut_added and max(cut_added) - min(cut_added) > 2:
        errors.append("added flows start at different points: %d..%d" % (min(cut_added), max(cut_added)))

    # ftp: los cerrados antes del corte de aplicacion a A; el cerrado en el corte y los siguientes a B
    exp_a = sorted(os.path.basename(p) for p in before[:-1])
    exp_b = sorted(os.path.basename(p) for p in before[-1:] + after)
    got_a = sorted(os.listdir(args.ftp_a)) if os.path.isdir(args.ftp_a) else []
    got_b = sorted(os.listdir(args.ftp_b)) if os.path.isdir(args.ftp_b) else []
    if got_a != exp_a:
        errors.append("ftpA has %s, expected %s" % (got_a, exp_a))
    if got_b != exp_b:
        errors.append("ftpB has %s, expected %s" % (got_b, exp_b))
    for path in before + after:
        for d in (args.ftp_a, args.ftp_b):
            up = os.path.join(d, os.path.basename(path))
            if os.path.exists(up) and open(up, "rb").read() != open(path, "rb").read():
                errors.append("uploaded %s differs from %s" % (up, path))

    total = summ["sent_total"]
    print(json.dumps({
        "sent": total,
        "achieved_pkt_s": round(summ["achieved_pkt_s"]),
        "lost": lost,
        "lost_pct": round(100.0 * lost / total, 2) if total else 0.0,
        "dup_written": dup,
        "flows_with_loss": flows_with_loss,
        "worst_flow_loss_pct": round(worst, 1),
        "files_before": len(before),
        "files_after": len(after),
        "removed_stop_seq": [min(cut_removed), max(cut_removed)] if cut_removed else None,
        "added_start_seq": [min(cut_added), max(cut_added)] if cut_added else None,
        "ftp_a": got_a,
        "ftp_b": got_b,
        "reload_ok": not errors,
        "errors": errors[:20],
    }))


if __name__ == "__main__":
    main()
