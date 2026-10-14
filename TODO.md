# TODO — problemas pendientes de reader_network

Problemas detectados al revisar `src/reader_network.c` para más de 100
flujos multicast y durante el desarrollo de `mode_continuous` (versiones
0.82-0.83), todavía sin corregir. Los números de línea corresponden a la
versión 0.84; si el código ha cambiado, buscar por el nombre de función o
el fragmento citado. Los puntos ya resueltos están al final, en
"Resueltos"; se conserva la numeración original para no romper las
referencias cruzadas entre puntos.

Para cualquier cambio en el bucle de captura: guardar antes el binario
actual (`cp bin/reader_network64 /tmp/rn.antes`, porque `build.sh`
sobrescribe `bin/`), aplicar el cambio y comparar con
`tests/load/compare.sh /tmp/rn.antes bin/reader_network64 <dir>`. Ver
`CLAUDE.md` (sección "Prueba de carga") y recordar el requisito de
compatibilidad con distribuciones Linux muy antiguas.

---

## Prioridad alta: robustez

### 1. La reconexión multicast es "todo o nada"

- **Dónde**: bucle principal, rama `else if ( select_count == 0 )`
  (`src/reader_network.c:1825`).
- **Problema**: los sockets solo se cierran y se vuelven a suscribir
  cuando `select()` pasa `SELECT_TIMEOUT` (10 s) sin datos en **ningún**
  socket. Con muchos flujos eso casi nunca ocurre, así que si un flujo
  concreto pierde su suscripción IGMP (reinicio de un switch, cambio de
  querier, etc.) no se recupera nunca mientras los demás sigan emitiendo.
  En `mode_continuous` el proceso no se relanza desde cron, así que el
  flujo queda perdido hasta que alguien reinicie el programa.
  Además, cuando sí se dispara, cierra y re-suscribe todos los grupos a
  la vez.
- **Propuesta**: guardar la hora del último paquete recibido por socket
  (array paralelo a `s_reader[]`) y, si un socket lleva más de N segundos
  sin datos mientras otros sí reciben, reconectar solo ese socket.
  - En 0.76 se intentó un rejoin periódico sin cerrar el socket y no
    funcionó ("socket ocupado", ver CHANGELOG), así que hay que cerrar y
    reabrir ese socket concreto.
  - Al reabrir, actualizar `radar_destination[j].socket` de todos los
    radares que comparten ese socket. Desde 0.84 se recorren con la lista
    del socket (`socket_first_radar[idx]` y `radar_destination[j].next`,
    ver `setup_input_network()`). Si se reabre en la misma posición de
    `s_reader[]`, `socket_index` y la lista no cambian.
  - Desde 0.84 un socket puede tener el grupo suscrito en varias
    interfaces (entradas con el mismo grupo:puerto e interfaz distinta):
    al reabrirlo hay que repetir todas esas suscripciones, no solo la de
    la primera entrada.
  - El bucle espera con `poll()` sobre `s_reader_poll[]`, que solo se
    rellena al final de `setup_input_network()`: si se reabre un socket
    suelto hay que actualizar su entrada (`s_reader_poll[idx].fd`). El
    socket nuevo tiene que ser no bloqueante, como los de
    `open_input_socket()` (el bucle vacía cada socket hasta `EAGAIN`).
    Para reaprovechar código, ver el "pool" de sockets de
    `setup_input_network()` que usa la recarga.
- **Verificar**: escenario nuevo en `tests/load/` en el que un flujo deja
  de emitir un rato y vuelve mientras el resto sigue; comprobar en el log
  que solo se reconecta ese socket y que el flujo se recupera.

### 2. Un error al rotar el fichero mata el proceso en `mode_continuous`

- **Dónde**: `setup_output_file()` (`src/reader_network.c:451` en
  adelante), llamada en cada rotación desde el bucle principal (bloque
  `pid_t pid = fork();`, línea 1528).
- **Problema**: casi todos los errores de `setup_output_file()` hacen
  `exit(EXIT_FAILURE)`: fallo del `mkdir` vía `system()`, fallo de
  `open()`, y sobre todo el chequeo de `dest_free_space` con `statvfs()`,
  que aborta si no hay espacio suficiente. Antes eso solo perdía una
  grabación y cron relanzaba el proceso; en modo continuo el proceso
  entero muere y no hay nada que lo relance.
- **Propuesta** (a decidir):
  - En la rotación, si `setup_output_file()` falla, registrar el error,
    seguir sin fichero de salida (o con el anterior) y reintentar en la
    siguiente vuelta, en vez de `exit()`. Implica separar el código de
    error del `exit()` dentro de `setup_output_file()`.
  - Para disco lleno, decidir la política: dejar de grabar hasta que
    haya espacio, o borrar grabaciones antiguas.
  - Complementario: documentar el uso con un supervisor (systemd
    `Restart=on-failure`).

---

## Prioridad media: rendimiento con muchos flujos

### 20. Coste de `poll()` por vuelta con muchos sockets

- **Dónde**: `poll(s_reader_poll, socket_count, ...)` en el bucle
  principal.
- **Problema**: con tráfico repartido en el tiempo (radares reales, no
  sincronizados; escenario `spread` de `tests/load/`) casi cada paquete
  despierta el bucle por separado, y cada vuelta cuesta O(número de
  sockets) en el kernel: `poll()` registra y retira el proceso en la cola
  de espera de los 150 sockets aunque solo uno tenga datos. Con 150 flujos
  y 7.500 pkt/s el lector gasta ~5,2 s de CPU en 20 s (~26 % de un
  núcleo), unos 35 us por vuelta; el mismo tráfico en ráfagas (`base`)
  gasta 0,65 s. Quitar el `memset` (-3,5 %) y pasar de `select()` a
  `poll()` (-8 %) apenas lo mueven: el coste está en el kernel.
- **Propuesta**: `epoll` (Linux >= 2.6, glibc >= 2.3.2), con guarda de
  preprocesador y `poll()` como alternativa en sistemas sin él (requisito
  de compatibilidad). Con `epoll_wait()` el coste por vuelta depende de
  los sockets listos, no de los configurados. El conjunto de `epoll` hay
  que mantenerlo en `setup_input_network()` (arranque, reconexión y
  recarga: añadir los sockets nuevos y quitar los cerrados).
- **Verificar**: `spread` y `spreaddecode` contra el binario anterior
  (CPU) y el resto de escenarios sin pérdidas.

---

## Prioridad baja: correcciones menores

### 11. Corte de rotación en `mode_continuous` no es atómico por paquete

- **Dónde**: chequeo de rotación (bloque `if (mode_continuous &&
  timed_t_current.tv_sec >= next_rotation_t)`), `poll()`+`recvfrom()` y
  los `write()`/`writev()` a `fd_out_ast`/`fd_out_gps` del bucle principal.
- **Problema**: la decisión de rotar se toma una vez por vuelta del
  bucle, **antes** de leer los paquetes pendientes, y se basa en la hora
  de proceso (`gettimeofday` al principio de la vuelta), no en la hora
  real de llegada de cada paquete (no se usa `SO_TIMESTAMP` ni nada del
  kernel). Hasta 0.84 además solo se hacía **un** `recvfrom()` por socket
  y vuelta (hoy se vacía hasta `EAGAIN`, ver el caso 2). Esto abre dos
  ventanas de carrera, ambas acotadas a lo que tarda un
  `poll()`+`recvfrom()` (normalmente
  microsegundos, sin esperas artificiales en el bucle):
  1. Un paquete que llega justo **después** del corte pero se lee en una
     vuelta cuyo chequeo de rotación ya se había hecho (con
     `select()` bloqueado esperando ese mismo paquete) se escribe en el
     fichero **antiguo**, aunque su hora real de llegada sea ya del
     siguiente intervalo.
  2. Si en el instante del corte hay más de un datagrama ya encolado en
     el kernel para el mismo socket, solo se lee uno esa vuelta; el
     resto se lee en la vuelta siguiente, después de rotar, y acaba en
     el fichero **nuevo** aunque llegara antes del corte. Requiere
     backlog justo en el instante de la rotación (tráfico de varios
     flujos a la vez), el escenario que fuerza `tests/load/`.
  En ambos casos el impacto está acotado a como mucho un paquete por
  socket/flujo por rotación; con flujos de radar reales (no
  sincronizados entre sí) la probabilidad de coincidir con esa ventana
  de microsegundos en cada corte es baja, pero no nula, y crece con el
  número de flujos.
- **Decisión (2026-09-28)**: no se aborda por ahora, prioridad baja. Si
  se retoma:
  - El caso 2 está casi resuelto desde que el bucle vacía cada socket
    listo hasta `EAGAIN` (punto 5, en "Resueltos"): lo encolado en el
    instante del corte se lee en la misma pasada, antes de rotar. Solo
    queda si un socket tiene más de `RN_MAX_DRAIN` (64) paquetes
    encolados justo en el corte.
  - El caso 1 no se puede eliminar solo con drenaje: haría falta el
    timestamp real de llegada del paquete (p.ej. `SO_TIMESTAMP`) para
    decidir a qué fichero pertenece cada paquete por su hora real, no
    por la hora en que el proceso lo lee. Cambio más profundo en el
    formato interno de fechado.
- **Verificar**: no existe hoy ningún escenario en `tests/load/` que
  compruebe en qué lado del corte cae cada paquete cercano al instante
  de rotación (los escenarios actuales miden pérdidas/duplicados
  agregados, no la partición exacta por fichero). Habría que añadir uno
  que sincronice el envío de paquetes con los instantes de rotación
  configurados y compare, tras descomprimir, la hora de cada paquete
  contra el nombre/rango de su fichero.

### 18. Otras utilidades siguen calculando horas con `mktime()` (hora local)

- **Dónde**: `src/utils/reader_file.c:83` (`t3 = mktime(t2)`, medianoche
  del día), `src/utils/filtertime_s.c:149-150` (convierte las horas de
  inicio/fin del filtro con `mktime()` y las compara con la hora gps, que
  son segundos desde las 00:00 UTC) y `src/reader_rrd3.c:436`
  (deshabilitado en `build.sh`).
- **Problema**: el mismo que se corrigió en `reader_network` en 0.84
  (punto 14): `mktime()` interpreta la fecha como hora local, así que solo
  dan el resultado correcto si el sistema está en UTC. En `filtertime_s`
  el rango filtrado saldría desplazado el desfase de la zona horaria.
- **Propuesta**: calcular en UTC. Para la medianoche,
  `tv_sec - tv_sec % 86400`. Para `filtertime_s`, pasar `HH:MM:SS`
  directamente a segundos del día (`h*3600 + m*60 + s`) en vez de
  `strptime()` + `mktime()`.

### 19. SIGTERM/SIGINT no interrumpen la compresión/subida final

- **Dónde**: `handle_shutdown()` y el final de `main()`
  (`close_output_file()` + `send_output_file()`).
- **Problema**: el manejador solo pone `shutdown_requested = 1`, que se
  consulta en la condición del bucle de captura. Una vez fuera del bucle,
  durante el `bzip2` y los hasta 10 intentos de curl, un SIGTERM no hace
  nada (visto al probar 0.84 con un FTP inalcanzable: `timeout -s TERM`
  no paraba el proceso). Con los timeouts de 0.84, el peor caso con el
  FTP caído es de unos 10 × 62 s por URI; antes podía durar mucho más.
  Un supervisor (systemd) acabará mandando SIGKILL.
- **Propuesta** (a decidir): si llega un segundo SIGTERM/SIGINT durante el
  cierre, abandonar los reintentos de FTP (comprobar `shutdown_requested`
  entre intentos de curl) o restaurar el manejador por defecto al salir
  del bucle.

---

## Prueba de carga (`tests/load/`)

### 16. El escenario `noise` no llega a provocar pérdidas

- **Problema**: el fallo del `break` corregido en 0.83 solo produce
  pérdidas cuando el ruido de una IP no configurada supera lo que el
  bucle puede procesar. El emisor en Python no genera tanto tráfico en
  una máquina de 2 núcleos, así que el escenario solo verifica el
  contador `ignored`.
- **Propuesta**: emisor en C (o varios procesos Python en paralelo) para
  poder saturar, y medir también la latencia por flujo, no solo pérdidas.
  Desde 0.84 el emisor tiene `--spread` (escenarios `spread` y
  `spreaddecode`), que reparte los envíos como radares no sincronizados:
  es el caso que más CPU cuesta al lector (ver el punto 20).

### 17. Escenario para el punto 1

- Añadir un escenario de flujo que se corta y vuelve (punto 1), para poder
  verificar esa corrección cuando se haga. El del punto 3 (grupo:puerto
  repetido en entradas no consecutivas) ya existe desde 0.84: `dupgroup`.

---

## Resueltos

### En 0.84

- **3. Sockets duplicados si el mismo grupo:puerto no aparece en entradas
  consecutivas.** `setup_input_network()` busca en todas las entradas
  anteriores. Si la interfaz es otra, suscribe el grupo en esa interfaz
  sobre el mismo socket (antes nunca se suscribía, ni siquiera con
  entradas consecutivas). Solo se grababa duplicado cuando la IP de origen
  encajaba con las dos entradas (p.ej. `0.0.0.0`); en los demás casos el
  paquete se recibía dos veces y la copia sobrante contaba como `ignored`.
  Escenario `dupgroup` de `tests/load/`.
- **7. Búsqueda lineal del radar por cada paquete.** Cada socket tiene su
  lista de radares (`socket_first_radar[]` + `radar_destination[].next`)
  con la IP de origen en binario. Al recibir se comparan enteros, sin
  `inet_ntoa()` ni `strcasecmp()`.
- **8. Copia en pila en formato gps.** Datablock + 10 bytes con un solo
  `writev()`. No se añade buffer de salida propio (decisión: no arriesgar
  datos sin escribir si el proceso muere de golpe).
- **9. Buffer de recepción fijo de 100 KB.** Nueva clave
  `source_recv_buffer_size` (bytes, por defecto 212992). Con root se pide
  con `SO_RCVBUFFORCE`. En el log de arranque aparecen el valor pedido y
  el que da el kernel.
- **10. Comprobación del máximo de radares.** `parse_config()` valida, para
  multicast y broadcast, que haya 5 cadenas por radar y como mucho
  `MAX_RADAR_NUMBER` (256) radares. Con 256 ya arranca.
- **4. Datablocks cuyo tamaño supera los bytes restantes del paquete.**
  Se descarta el resto del paquete y cuenta como `malformed` cualquier
  datablock con tamaño < 3 o mayor que lo que queda del paquete (también
  si quedan 1 o 2 bytes sueltos, que no llegan a cabecera). Los
  datablocks anteriores del mismo paquete sí se graban. Los 10 primeros se
  registran en el log (ip de origen, radar, cat, tamaño y bytes
  restantes), para detectar emisores reales con tamaños inconsistentes
  (el SMR del comentario del código); antes sus bytes sobrantes se
  grababan rellenos con ceros. Escenario `malformed` ampliado: el
  datablock sobredimensionado pasa de grabarse a no grabarse.
- **5. Coste fijo en cada vuelta del bucle.**
  - Sin el `memset` de 64 KB por vuelta (posible tras el punto 4). Con
    `dest_localhost` se ponen a cero solo `RN_DECODE_PAD` (4 KB) tras cada
    paquete, porque los descodificadores pueden leer algo más allá del
    último datablock. Ahorro medido: -3,5 % de CPU con tráfico repartido
    (`spread`), nada visible con ráfagas (`base`): había pocas vueltas por
    segundo.
  - Sockets no bloqueantes y vaciado de cada socket listo hasta `EAGAIN`
    (tope `RN_MAX_DRAIN`, 64). Con un socket con mucho tráfico (`noise`)
    -36 % de CPU (1,37 -> 0,87 s); con un paquete por socket cuesta un
    `recvfrom()` extra que devuelve `EAGAIN` (+4-10 %).
  - El coste que queda es el de `poll()` sobre todos los sockets en cada
    vuelta: ver el punto 20.
- **6. `select()` y el límite de `FD_SETSIZE`.** Se usa `poll()`
  (`s_reader_poll[]`, lo rellena `setup_input_network()`), sin límite de
  descriptor. Probado lanzando el lector con 1100 descriptores ya
  abiertos: con `select()` glibc aborta ("bit out of range 0 - FD_SETSIZE
  on fd_set"; sin `_FORTIFY_SOURCE` sería corrupción de memoria), con
  `poll()` graba todo. También -8 % de CPU en `spread`.
- **12. `EINTR` en `recvfrom()`.** Se corrigió con la recarga por SIGHUP
  (deja de leer ese socket en esa vuelta) y ahora va junto con `EAGAIN` en
  el vaciado.
- **13. Corte por `timed` hasta 20 s tarde con la fuente en silencio.** El
  timeout del `select()` (hoy `poll()`) se acorta hasta el final de la
  grabación (sin disparar la reconexión) y `timed_t_current` se actualiza
  después del `select()`.
- **14. `midnight_t` dependía de la zona horaria.** Ahora es
  `tv_sec - tv_sec % 86400`: medianoche UTC siempre. El resto de
  utilidades, en el punto 18.
- **15. Hijos de compresión/FTP sin límite en `mode_continuous`.** No se
  limitan; en su lugar curl aborta cada intento si no conecta en 60 s o
  si la transferencia se para 60 s (`CURLOPT_TIMEOUT` sigue en 300/7200 s
  para subidas lentas que avanzan).
