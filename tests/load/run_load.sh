#!/bin/bash
# Prueba de carga de reader_network con muchos flujos multicast en loopback.
#
# Uso: run_load.sh <binario reader_network> <escenario> <dir_trabajo>
#
# Escenarios (todos con 150 flujos salvo que se diga):
#   base      50 pkt/s por flujo, 4 registros CAT048 por paquete, sin decodificar
#   decode    igual que base pero con dest_localhost=true (decodifica y reenvia)
#   noise     base + 3000 pkt/s desde una ip de origen no configurada al flujo 0
#   scrm      20 pkt/s por flujo, mode_scrm=true, cada paquete se repite 1,5 s despues
#   malformed 20 pkt/s por flujo, en el segundo 5 llega un datablock de tamaño 0
#   fdleak    sin trafico durante 35 s con dest_localhost=true (cuenta descriptores)
#   dupgroup  base + una entrada final que repite el grupo:puerto del flujo 0 (no
#             consecutiva) con ip de origen 0.0.0.0: si se abre un segundo socket
#             para el mismo grupo, cada paquete del flujo 0 se graba dos veces
#
# Deja en <dir_trabajo>/<escenario>-<binario>/ la configuracion, el log del
# lector, meta.json (estado, descriptores, errores de buffer UDP del kernel)
# y metrics.json (perdidas/duplicados por flujo, calculado por analyze.py).
set -u

BIN=$(readlink -f "$1")
SCEN=$2
WORK=$(readlink -f "$3")
HERE=$(cd "$(dirname "$0")" && pwd)

FLOWS=150 RATE=50 RECORDS=4 DURATION=20
LOCALHOST=false SCRM=false NOISE=0 DUP=0 MALFORMED=-1 DUPGROUP=false
case "$SCEN" in
    base) ;;
    decode) LOCALHOST=true ;;
    noise) NOISE=3000 ;;
    scrm) RATE=20 RECORDS=1 SCRM=true DUP=1.5 ;;
    malformed) RATE=20 RECORDS=1 DURATION=15 MALFORMED=5 ;;
    fdleak) RATE=0 LOCALHOST=true DURATION=35 ;;
    dupgroup) DUPGROUP=true ;;
    *) echo "escenario desconocido: $SCEN" >&2; exit 2 ;;
esac

RUN="$WORK/$SCEN-$(basename "$BIN")"
rm -rf "$RUN"
mkdir -p "$RUN"
HASH=01234567890123456789012345678901

{
    echo 'enabled = true'
    echo 'source = "multicast"'
    echo 'timed = 0'
    echo 'timed_stats_interval = 0'
    echo 'mode_daemon = false'
    echo "mode_scrm = $SCRM"
    echo "dest_localhost = $LOCALHOST"
    echo 'dest_file_timestamp = false'
    echo 'dest_file_compress = false'
    echo "dest_file = \"$RUN/rec\""
    echo 'dest_file_format = "gps"'
    echo "asterix_versions = \"$HASH\""
    echo 'radar_definition = {'
    for ((i = 0; i < FLOWS; i++)); do
        sep=","; [ $i -eq $((FLOWS - 1)) ] && [ "$DUPGROUP" = false ] && sep=""
        echo "    \"f$i\", \"239.255.$((1 + i / 250)).$((1 + i % 250))\", \"5000\", \"127.0.0.1\", \"127.0.0.1\"$sep"
    done
    if [ "$DUPGROUP" = true ]; then
        echo '    "dup0", "239.255.1.1", "5000", "0.0.0.0", "127.0.0.1"'
    fi
    echo '}'
} > "$RUN/test.conf"

udp_rcvbuf_errors() {
    awk '/^Udp:/ { if (!h) { for (i = 1; i <= NF; i++) col[$i] = i; h = 1 } else print $col["RcvbufErrors"] }' /proc/net/snmp
}
nfds() { ls "/proc/$1/fd" 2>/dev/null | wc -l; }

asterix_versions=$HASH "$BIN" "$RUN/test.conf" > "$RUN/reader.log" 2>&1 &
PID=$!
sleep 2
FD_START=$(nfds $PID)
ERR_START=$(udp_rcvbuf_errors)

if [ "$RATE" != "0" ]; then
    python3 "$HERE/sender.py" --flows $FLOWS --rate $RATE --records $RECORDS \
        --duration $DURATION --dup-delay $DUP --noise-rate $NOISE \
        --malformed-at $MALFORMED --summary "$RUN/sent.json"
else
    sleep $DURATION
fi
sleep 2

FD_END=$(nfds $PID)
ERR_END=$(udp_rcvbuf_errors)
STATUS=ok
if ! kill -0 $PID 2>/dev/null; then
    STATUS=died
else
    kill -TERM $PID
    for ((w = 0; w < 15; w++)); do
        kill -0 $PID 2>/dev/null || break
        sleep 1
    done
    if kill -0 $PID 2>/dev/null; then
        STATUS="hang"
        kill -KILL $PID
    fi
fi
wait $PID 2>/dev/null

printf '{"scenario":"%s","binary":"%s","status":"%s","fd_start":%s,"fd_end":%s,"udp_rcvbuf_errors":%s}\n' \
    "$SCEN" "$(basename "$BIN")" "$STATUS" "${FD_START:-0}" "${FD_END:-0}" \
    "$((ERR_END - ERR_START))" > "$RUN/meta.json"
if [ -f "$RUN/sent.json" ]; then
    python3 "$HERE/analyze.py" --summary "$RUN/sent.json" --file "$RUN/rec.gps" > "$RUN/metrics.json"
fi
cat "$RUN/meta.json"
[ -f "$RUN/metrics.json" ] && cat "$RUN/metrics.json"
exit 0
