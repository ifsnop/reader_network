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
#   malformed 20 pkt/s por flujo, en el segundo 5 llegan al flujo 0 un datablock de
#             tamaño 0, uno valido (SAC=4) + otro (SAC=3) que declara mas bytes de los
#             que trae el paquete, y uno valido (SAC=4) + 2 bytes sueltos: se deben
#             grabar los dos SAC=4 y ningun SAC=3, y contar 3 malformed
#   fdleak    sin trafico durante 35 s con dest_localhost=true (cuenta descriptores)
#   dupgroup  base + una entrada final que repite el grupo:puerto del flujo 0 (no
#             consecutiva) con ip de origen 0.0.0.0: si se abre un segundo socket
#             para el mismo grupo, cada paquete del flujo 0 se graba dos veces
#   spread    como base, pero los envios de cada ronda se reparten de forma uniforme
#             entre los flujos (radares no sincronizados): casi un paquete por
#             vuelta del bucle, el peor caso para el coste fijo por vuelta
#   spreaddecode  spread con dest_localhost=true
#   reload    20 pkt/s por flujo, mode_continuous con rotacion cada 20 s; se graban
#             los flujos 0..99 y, 2 s despues de un corte, se recarga con SIGHUP:
#             primero un .conf con error de sintaxis (se rechaza), luego 0..149 y
#             por ultimo 50..149 con otro dest_ftp_uri (file://) y timed cambiado
#             (se ignora). En el siguiente corte deben dejar de grabarse 0..49,
#             empezar 100..149 y 50..99 seguir sin huecos ni duplicados; lo
#             comprueba check_reload.py (no se compara con analyze.py)
#
# Deja en <dir_trabajo>/<escenario>-<binario>/ la configuracion, el log del
# lector, meta.json (estado, descriptores, errores de buffer UDP del kernel,
# CPU del lector y datablocks contados como malformed)
# y metrics.json (perdidas/duplicados por flujo, calculado por analyze.py).
set -u

BIN=$(readlink -f "$1")
SCEN=$2
WORK=$(readlink -f "$3")
HERE=$(cd "$(dirname "$0")" && pwd)

FLOWS=150 RATE=50 RECORDS=4 DURATION=20
LOCALHOST=false SCRM=false NOISE=0 DUP=0 MALFORMED=-1 DUPGROUP=false RELOAD=false SPREAD=false
case "$SCEN" in
    base) ;;
    decode) LOCALHOST=true ;;
    noise) NOISE=3000 ;;
    scrm) RATE=20 RECORDS=1 SCRM=true DUP=1.5 ;;
    malformed) RATE=20 RECORDS=1 DURATION=15 MALFORMED=5 ;;
    fdleak) RATE=0 LOCALHOST=true DURATION=35 ;;
    dupgroup) DUPGROUP=true ;;
    reload) RATE=20 RECORDS=1 DURATION=50 RELOAD=true ;;
    spread) SPREAD=true ;;
    spreaddecode) SPREAD=true LOCALHOST=true ;;
    *) echo "escenario desconocido: $SCEN" >&2; exit 2 ;;
esac

RUN="$WORK/$SCEN-$(basename "$BIN")"
rm -rf "$RUN"
mkdir -p "$RUN"
HASH=01234567890123456789012345678901

# write_conf <primer flujo> <ultimo flujo> [timed] [dest_ftp_uri]
write_conf() {
    local first=$1 last=$2 timed=${3:-0} ftp=${4:-}
    echo 'enabled = true'
    echo 'source = "multicast"'
    echo "timed = $timed"
    echo 'timed_stats_interval = 0'
    echo 'mode_daemon = false'
    echo "mode_scrm = $SCRM"
    echo "dest_localhost = $LOCALHOST"
    if [ "$RELOAD" = true ]; then
        echo 'mode_continuous = true'
        echo 'dest_file_timestamp = true'
        echo 'dest_file_nodirectory = true'
    else
        echo 'dest_file_timestamp = false'
    fi
    echo 'dest_file_compress = false'
    echo "dest_file = \"$RUN/rec\""
    echo 'dest_file_format = "gps"'
    [ -n "$ftp" ] && echo "dest_ftp_uri = { \"$ftp\" }"
    echo "asterix_versions = \"$HASH\""
    echo 'radar_definition = {'
    for ((i = first; i <= last; i++)); do
        sep=","; [ $i -eq $last ] && [ "$DUPGROUP" = false ] && sep=""
        echo "    \"f$i\", \"239.255.$((1 + i / 250)).$((1 + i % 250))\", \"5000\", \"127.0.0.1\", \"127.0.0.1\"$sep"
    done
    if [ "$DUPGROUP" = true ]; then
        echo '    "dup0", "239.255.1.1", "5000", "0.0.0.0", "127.0.0.1"'
    fi
    echo '}'
}

if [ "$RELOAD" = true ]; then
    mkdir -p "$RUN/rec" "$RUN/ftpA" "$RUN/ftpB"
    write_conf 0 99 20 "file://$RUN/ftpA/" > "$RUN/test.conf"
else
    write_conf 0 $((FLOWS - 1)) > "$RUN/test.conf"
fi

# reload: 2 s despues del proximo corte (multiplo de 20 s), tres SIGHUP seguidos
reload_sequence() {
    local now=$(date +%s)
    sleep $((20 - now % 20 + 2))
    kill -0 $PID 2>/dev/null || return
    cp "$RUN/test.conf" "$RUN/test.conf.orig"
    echo 'radar_definition = {' > "$RUN/test.conf"          # error de sintaxis
    kill -HUP $PID; sleep 1
    write_conf 0 149 20 "file://$RUN/ftpB/" > "$RUN/test.conf"
    kill -HUP $PID; sleep 1
    write_conf 50 149 30 "file://$RUN/ftpB/" > "$RUN/test.conf"  # timed cambiado: se ignora
    kill -HUP $PID
    date +%s > "$RUN/hup.epoch"
}

udp_rcvbuf_errors() {
    awk '/^Udp:/ { if (!h) { for (i = 1; i <= NF; i++) col[$i] = i; h = 1 } else print $col["RcvbufErrors"] }' /proc/net/snmp
}
nfds() { ls "/proc/$1/fd" 2>/dev/null | wc -l; }

asterix_versions=$HASH "$BIN" "$RUN/test.conf" > "$RUN/reader.log" 2>&1 &
PID=$!
sleep 2
FD_START=$(nfds $PID)
ERR_START=$(udp_rcvbuf_errors)

[ "$RELOAD" = true ] && { reload_sequence & HUPPID=$!; }

if [ "$RATE" != "0" ]; then
    python3 "$HERE/sender.py" --flows $FLOWS --rate $RATE --records $RECORDS \
        --duration $DURATION --dup-delay $DUP --noise-rate $NOISE \
        --malformed-at $MALFORMED $([ "$SPREAD" = true ] && echo --spread) --summary "$RUN/sent.json"
else
    sleep $DURATION
fi
sleep 2
[ "$RELOAD" = true ] && wait $HUPPID

FD_END=$(nfds $PID)
ERR_END=$(udp_rcvbuf_errors)
# CPU del lector (utime+stime, sin los hijos de rotacion), en segundos
CPU=$(LC_ALL=C awk -v hz="$(getconf CLK_TCK)" '{ printf "%.2f", ($14 + $15) / hz }' "/proc/$PID/stat" 2>/dev/null)
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

# datablocks descartados por tamaño invalido (linea final de estadisticas)
MALFORMED=$(sed -n 's/.*stats malformed\[\([0-9]*\)\].*/\1/p' "$RUN/reader.log" | tail -1)
printf '{"scenario":"%s","binary":"%s","status":"%s","fd_start":%s,"fd_end":%s,"udp_rcvbuf_errors":%s,"cpu_s":%s,"malformed_counted":%s}\n' \
    "$SCEN" "$(basename "$BIN")" "$STATUS" "${FD_START:-0}" "${FD_END:-0}" \
    "$((ERR_END - ERR_START))" "${CPU:-null}" "${MALFORMED:-0}" > "$RUN/meta.json"
if [ "$RELOAD" = true ] && [ -f "$RUN/sent.json" ]; then
    sleep 2 # subida (file://) del ultimo fichero rotado, en un hijo
    python3 -B "$HERE/check_reload.py" --summary "$RUN/sent.json" --log "$RUN/reader.log" \
        --ftp-a "$RUN/ftpA" --ftp-b "$RUN/ftpB" > "$RUN/metrics.json"
elif [ -f "$RUN/sent.json" ]; then
    python3 "$HERE/analyze.py" --summary "$RUN/sent.json" --file "$RUN/rec.gps" > "$RUN/metrics.json"
fi
cat "$RUN/meta.json"
[ -f "$RUN/metrics.json" ] && cat "$RUN/metrics.json"
exit 0
