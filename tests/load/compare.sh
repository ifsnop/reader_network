#!/bin/bash
# Ejecuta los escenarios de run_load.sh con dos binarios y muestra una tabla.
#
# Uso: compare.sh <binario_antes> <binario_despues> <dir_trabajo> [escenarios...]
# Por defecto: base decode noise scrm malformed fdleak
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
A=$1 B=$2 WORK=$3
shift 3
SCENARIOS=${*:-base decode noise scrm malformed fdleak}
mkdir -p "$WORK"
for s in $SCENARIOS; do
    for bin in "$A" "$B"; do
        echo "== $s $(basename "$bin")" >&2
        "$HERE/run_load.sh" "$bin" "$s" "$WORK" > /dev/null
    done
done
python3 "$HERE/report.py" "$WORK" "$(basename "$A")" "$(basename "$B")" $SCENARIOS
