#!/usr/bin/env python3
"""Tabla comparativa de los resultados de run_load.sh (uso: ver compare.sh)."""
import json
import os
import sys


def load(path):
    try:
        with open(path) as fh:
            return json.load(fh)
    except (OSError, ValueError):
        return {}


def main():
    work, bins, scenarios = sys.argv[1], sys.argv[2:4], sys.argv[4:]
    cols = ["escenario", "binario", "estado", "pkt/s enviados", "perdidos %",
            "flujos con perdida", "peor flujo %", "flujo0 % / resto %",
            "duplicados grabados", "fds inicio->fin", "RcvbufErrors"]
    print("| " + " | ".join(cols) + " |")
    print("|" + "---|" * len(cols))
    for s in scenarios:
        for b in bins:
            run = os.path.join(work, "%s-%s" % (s, b))
            meta, m = load(os.path.join(run, "meta.json")), load(os.path.join(run, "metrics.json"))
            if m:
                row = [m["achieved_pkt_s"], m["lost_pct"], "%d/150" % m["flows_with_loss"],
                       m["worst_flow_loss_pct"],
                       "%s / %s" % (m["flow0_loss_pct"], m["other_flows_loss_pct"]),
                       m["dup_written"]]
            else:
                row = ["-"] * 6
            print("| " + " | ".join(str(x) for x in [s, b, meta.get("status", "?")] + row + [
                "%s->%s" % (meta.get("fd_start", "?"), meta.get("fd_end", "?")),
                meta.get("udp_rcvbuf_errors", "?")]) + " |")


if __name__ == "__main__":
    main()
