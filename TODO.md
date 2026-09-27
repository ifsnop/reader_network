# TODO — problemas pendientes de reader_network

Problemas detectados al revisar `src/reader_network.c` para más de 100
flujos multicast y durante el desarrollo de `mode_continuous` (versiones
0.82-0.83), todavía sin corregir. Los números de línea corresponden al
commit `7f192ff` (0.83); si el código ha cambiado, buscar por el nombre de
función o el fragmento citado.

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
  (`src/reader_network.c:1754-1763`).
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
    radares que comparten ese socket (ver `setup_input_network()`).
  - `select()` usa `s_reader[socket_count - 1] + 1` como `nfds`
    suponiendo que el último socket tiene el descriptor más alto; al
    reabrir un socket suelto eso deja de ser cierto. Calcular el máximo
    real, o resolverlo junto con el paso a `poll()` (punto 6).
- **Verificar**: escenario nuevo en `tests/load/` en el que un flujo deja
  de emitir un rato y vuelve mientras el resto sigue; comprobar en el log
  que solo se reconecta ese socket y que el flujo se recupera.

### 2. Un error al rotar el fichero mata el proceso en `mode_continuous`

- **Dónde**: `setup_output_file()` (`src/reader_network.c:435` en
  adelante), llamada en cada rotación desde el bucle principal (bloque
  `pid_t pid = fork();`, línea 1453).
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

### 3. Sockets duplicados si el mismo grupo:puerto no aparece en entradas consecutivas

- **Dónde**: `setup_input_network()`, condición
  `i>0 && !strcasecmp(radar_definition[(i*5)+1], radar_definition[((i-1)*5)+1]) && ...`
  (`src/reader_network.c:817`).
- **Problema**: solo se reutiliza el socket si la entrada **anterior**
  tiene el mismo grupo y puerto. Si el mismo grupo:puerto aparece en
  entradas separadas de `radar_definition`, se abre un segundo socket con
  `SO_REUSEADDR`; el kernel entrega cada paquete a los dos, así que se
  procesa dos veces y, sin `mode_scrm`, queda duplicado en la grabación.
  Con configuraciones de más de 100 entradas es fácil que ocurra.
- **Propuesta**: buscar en todas las entradas anteriores (no solo en
  `i-1`) un socket ya abierto con el mismo grupo y puerto, y reutilizarlo.
  Mantener el orden de `s_reader[]`.
- **Verificar**: escenario de `tests/load/` con una configuración que
  repita un grupo en entradas no consecutivas, sin `mode_scrm`; debe
  salir `duplicados grabados = 0`.

### 4. Datablocks cuyo tamaño supera los bytes restantes del paquete

- **Dónde**: bucle de datablocks (`do { ... } while (salir==0)`),
  `ast_size_datablock = (ast_ptr_raw[1]<<8) + ast_ptr_raw[2];`
  (línea 1589) y `ast_size_datablock = (ast_ptr_raw_tmp[1]<<8) + ...`
  (línea 1738).
- **Problema**: en 0.83 se descartan los tamaños menores que 3, pero si
  el tamaño declarado es mayor que lo que queda del paquete UDP, el
  código lee más allá de `udp_size` y graba bytes que no pertenecen al
  paquete. Hoy esos bytes son ceros gracias al `memset` de 64 KB que se
  hace en cada vuelta (línea 1479); **si se quita ese `memset`
  (punto 5), se grabarían restos de paquetes anteriores**.
- **Propuesta**: comprobar `ast_ptr_raw_tmp + ast_size_datablock <= ast_ptr_raw + udp_size`
  y decidir la política (descartar el resto del paquete y contarlo como
  `malformed`, o truncar). Hay que hacerlo **antes** del punto 5.
  Ojo con el comentario del código sobre el "cat 10 del smr de barajas y
  scr mal configurados": puede haber emisores reales que envíen tamaños
  inconsistentes; revisar con grabaciones reales antes de elegir.

---

## Prioridad media: rendimiento con muchos flujos

### 5. Coste fijo en cada vuelta del bucle

- **Dónde**: bucle principal, líneas 1479-1487 y bloque
  `if ( select_count > 0 )`.
- **Problema**: en cada vuelta se hace:
  - `memset` de `RN_MAX_PACKET_LENGTH` (64 KB) sobre `ast_ptr_raw`
    (línea 1479), innecesario porque `recvfrom()` devuelve el tamaño.
    A 5.000 vueltas/s son unos 320 MB/s de escritura en memoria.
  - `FD_ZERO` + `FD_SET` de todos los sockets (línea 1484), `select()`
    (O(descriptor máximo) en el kernel) y `FD_ISSET` de todos los sockets.
  - Un solo `recvfrom()` por socket listo y vuelta: con tráfico alto se
    repite todo lo anterior por cada paquete.
- **Propuesta** (en este orden, midiendo cada paso con `tests/load/`):
  1. Resolver antes el punto 4 y quitar el `memset`.
  2. Vaciar cada socket listo con `recvfrom(..., MSG_DONTWAIT)` hasta
     `EAGAIN`, con un tope por socket (p.ej. 64 paquetes) para no
     desatender al resto.
  3. Ver punto 6.

### 6. `select()` y el límite de `FD_SETSIZE`

- **Dónde**: línea 1487.
- **Problema**: `select()` no admite descriptores ≥ `FD_SETSIZE` (1024);
  `FD_SET` con un descriptor mayor escribe fuera del `fd_set` (corrupción
  de memoria, no un error controlado). Con hasta 255 radares no se llega
  hoy, pero cualquier fuga de descriptores (como la del socket de salida
  corregida en 0.83) acerca el límite, sobre todo en `mode_continuous`.
  Además `nfds` se calcula con el último socket (ver punto 1).
- **Propuesta**: pasar a `poll()`, disponible en cualquier distribución
  antigua. No usar `epoll`/`recvmmsg` salvo con guardas de preprocesador
  (Linux ≥ 2.6 / ≥ 2.6.33), por el requisito de compatibilidad.

### 7. Búsqueda lineal del radar por cada paquete

- **Dónde**: bucle `for(j=0;(j<radar_count/5); j++)` tras el
  `recvfrom()` (líneas 1531-1552).
- **Problema**: por cada paquete recorre todos los radares y compara la
  IP de origen como texto (`inet_ntoa()` + `strcasecmp()`). Es O(número
  de radares) por paquete.
- **Propuesta**: en `setup_input_network()`, precalcular para cada socket
  la lista de radares que lo usan con la IP de origen ya en binario
  (`in_addr_t`, `inet_addr()`) y un indicador de comodín para `0.0.0.0`.
  Al recibir, recorrer solo esa lista comparando enteros.

### 8. Un `write()` sin buffer por cada datablock y copia en pila en formato gps

- **Dónde**: `write(fd_out_ast, ...)` (línea 1690) y, en formato gps,
  `unsigned char output_ptr[RN_MAX_PACKET_LENGTH]` + `memcpy` + `write()`
  (líneas 1697-1730).
- **Problema**: una llamada al sistema por datablock (varios por paquete
  si el paquete trae varios), y en gps además se copia cada datablock a un
  buffer de 64 KB en la pila solo para añadirle los 10 bytes de fecha.
- **Propuesta**:
  - gps: usar `writev()` con dos segmentos (datablock + 10 bytes) y
    eliminar la copia.
  - Opcional: buffer propio de salida (p.ej. 64 KB) volcado por tamaño o
    por tiempo. Si se hace, **vaciarlo antes del `fork()` de la rotación**
    (si no, el hijo hereda datos sin escribir y se duplicarían o se
    perderían) y antes de salir. Valorar el riesgo de perder hasta un
    buffer entero si el proceso muere de golpe.

### 9. Buffer de recepción fijo de 100 KB por socket

- **Dónde**: `setup_input_network()`, `int recv_buffer_size = 100 * 1024;`
  (línea 849).
- **Problema**: con flujos de mucho tráfico, cualquier parón del bucle
  (compresión en el propio proceso al cerrar, disco lento, etc.) llena el
  buffer y el kernel descarta paquetes (se ve en `RcvbufErrors` de
  `/proc/net/snmp`, que `tests/load/` ya mide).
- **Propuesta**: hacerlo configurable en el `.conf`. El kernel lo limita a
  `net.core.rmem_max` salvo `SO_RCVBUFFORCE` (requiere root).

---

## Prioridad baja: correcciones menores

### 10. Comprobación del máximo de radares incorrecta

- **Dónde**: `parse_config()` (línea 316) y `setup_input_network()`
  (línea 905).
- **Problemas**:
  - La comprobación `if (radar_count>MAX_RADAR_NUMBER)` está después de
    un `exit()` dentro del bloque de error, así que nunca se ejecuta.
    Además compara número de cadenas (5 por radar) con número de radares.
  - La rama `broadcast` no tiene ninguna comprobación.
  - En `setup_input_network()` el `i++` va antes de
    `if ( i >= MAX_RADAR_NUMBER )`, así que con exactamente 256 radares
    sale con error aunque el array tiene 256 posiciones.
- **Propuesta**: comprobar `radar_count/5 > MAX_RADAR_NUMBER` en
  `parse_config()` para multicast y broadcast (y que `radar_count` sea
  múltiplo de 5), y corregir la comprobación de `setup_input_network()`.

### 11. `EINTR` en `recvfrom()`

- **Dónde**: líneas 1524-1526.
- **Problema**: desde 0.82 hay manejadores de SIGTERM/SIGINT, y cualquier
  error de `recvfrom()` es fatal (`exit(EXIT_FAILURE)`), incluido
  `EINTR`. Es poco probable porque `select()` ya indicó que hay datos,
  pero si ocurre se sale sin cierre ordenado (no se comprime ni se sube la
  última ventana). El caso de `select()` ya se corrigió en 0.82.
- **Propuesta**: tratar `EINTR` (y `EAGAIN` si se pasa a `MSG_DONTWAIT`,
  punto 5) como "no hay paquete" y seguir.

### 12. En modo no continuo, el corte por `timed` puede tardar hasta 20 s con la fuente en silencio

- **Dónde**: condición del `while` principal (línea 1436).
- **Problema**: la condición usa `timed_t_current`, que se actualiza al
  principio de la vuelta **anterior**, antes de un `select()` que puede
  bloquear 10 s. Sin tráfico, el proceso tarda hasta
  2 × `SELECT_TIMEOUT` en terminar tras cumplirse `timed`. Es un
  comportamiento anterior a 0.82; con tráfico real la diferencia es de
  milisegundos. No afecta a la rotación de `mode_continuous`, que usa un
  valor recién leído.
- **Propuesta**: actualizar `timed_t_current` justo después del
  `select()` (o calcular el timeout del `select()` como el mínimo entre
  `SELECT_TIMEOUT` y lo que falte para `timed`).

### 13. `midnight_t` depende de la zona horaria del sistema

- **Dónde**: `setup_time()`, `midnight_t = mktime(t2)` (línea 941), con
  `t2` obtenido de `gmtime()`.
- **Problema**: `mktime()` interpreta la fecha como hora **local**, así
  que `midnight_t` solo es la medianoche UTC si el sistema está en UTC.
  Afecta al fechado de los ficheros gps (`current_time_today`) y a la
  alineación de las rotaciones de `mode_continuous`. Probablemente todas
  las instalaciones están en UTC, pero no está garantizado.
- **Propuesta**: calcular `midnight_t = tv.tv_sec - (tv.tv_sec % 86400)`
  (medianoche UTC, portable, sin depender de `TZ`). Comprobar antes que
  ninguna instalación dependa del comportamiento actual.

### 14. Sin límite de hijos de compresión/FTP en `mode_continuous`

- **Dónde**: bloque de rotación (línea 1453) y `send_output_file()`.
- **Problema**: si el servidor FTP está caído mucho tiempo, cada rotación
  lanza un hijo que reintenta hasta 10 veces con timeouts de hasta 7200 s,
  y pueden acumularse varios en paralelo. Se aceptó así en 0.82 como
  primera versión.
- **Propuesta**: limitar el número de hijos vivos (contándolos con el
  `waitpid(WNOHANG)` que ya existe) o pasar a un único proceso de subida
  con cola de ficheros pendientes.

---

## Prueba de carga (`tests/load/`)

### 15. El escenario `noise` no llega a provocar pérdidas

- **Problema**: el fallo del `break` corregido en 0.83 solo produce
  pérdidas cuando el ruido de una IP no configurada supera lo que el
  bucle puede procesar. El emisor en Python no genera tanto tráfico en
  una máquina de 2 núcleos, así que el escenario solo verifica el
  contador `ignored`.
- **Propuesta**: emisor en C (o varios procesos Python en paralelo) para
  poder saturar, y medir también la latencia por flujo, no solo pérdidas.

### 16. Escenarios para los puntos 1 y 3

- Añadir un escenario de flujo que se corta y vuelve (punto 1) y otro con
  un grupo:puerto repetido en entradas no consecutivas (punto 3), para
  poder verificar esas correcciones cuando se hagan.
