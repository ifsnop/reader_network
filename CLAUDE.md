# reader_network

Grabador de datos de red por línea de comandos para Linux, escrito en C/C++.
Graba y procesa tráfico radar multicast en formato ASTERIX.

**Requisito clave del proyecto: compatibilidad absoluta con distribuciones
Linux muy antiguas.** Cualquier fix relacionado con el compilador o la
libc debe quedar protegido (`#if defined(__GNUC__) && ...`, etc.) para
que sea un no-op en toolchains antiguos, nunca asumir que el entorno de
compilación va a ser siempre moderno.

## Build

```
./build.sh
```

- Detecta si se puede compilar en 32 bits (`can32bits`, vía un conftest.c
  con `gcc -m32`); si el entorno no tiene multilib instalado, compila
  solo en 64 bits.
- Compila las librerías estáticas propias (`libdebug`, `libconfig`) y de
  terceros (`zlib`, `curl`) a partir de los tarballs en `3rdparty/`.
- **Trampa de cacheo:** `build.sh` solo (re)compila `libs/libdebug64.a` y
  `libs/libconfig64.a` si el `.a` correspondiente **no existe todavía**.
  Si tocas algo en `src/libdebug/*.c` o `src/libconfig/*.c`, hay que
  borrar el `.a` (y sus `.o` en `obj/`) a mano antes de relanzar
  `build.sh`, o el cambio no se recogerá:
  ```
  rm -f libs/libdebug64.a obj/log64.o obj/memory64.o obj/hex64.o
  ./build.sh
  ```
- Los binarios resultantes se dejan en `bin/` (uno por utilidad:
  `reader_network64`, `client64`, `client_time64`, `filter*`, etc).

## Gotchas de compilación en entornos nuevos (glibc/gcc recientes)

Estos dos problemas aparecieron al compilar por primera vez en un entorno
con gcc 13 / glibc 2.39 (Ubuntu 24.04), tras años compilando en máquinas
más antiguas. Ninguno de los dos indica un bug nuevo introducido por el
cambio de entorno; simplemente el compilador/libc modernos son más
estrictos y sacan a la luz comportamiento que antes pasaba desapercibido.

### 1. `facilitynames` undeclared en `src/libdebug/log.c`

`log.c` define `_ISOC99_SOURCE` explícitamente antes de incluir
`<errno.h>`/`<syslog.h>` (para tener `vsnprintf()`). En glibc reciente,
`<features.h>` deja de activar `_DEFAULT_SOURCE` (y por tanto
`__USE_MISC`) automáticamente en cuanto el usuario define **cualquier**
macro de feature-test explícita. `facilitynames[]` en
`<sys/syslog.h>` está protegido por
`#if defined SYSLOG_NAMES && defined __USE_MISC`, así que deja de
declararse → error `'facilitynames' undeclared`.

**Fix:** añadir `#define _DEFAULT_SOURCE` junto a `_ISOC99_SOURCE` al
principio de `log.c`, para no perder `__USE_MISC`. No afecta a
compiladores/libc antiguos (donde `_DEFAULT_SOURCE` es un no-op o ya
estaba implícito).

### 2. `-Wstringop-truncation` en `src/sacsic.c`

`-Wstringop-truncation` forma parte de `-Wall` desde GCC 8; con
compiladores anteriores este aviso ni existía. Salta en
`ast_get_SACSIC()`, rama `GET_SIC_SHORT` para SAC=0x00 (estaciones
españolas), porque varios nombres de estación (`"SMR-TFN"`,
`"SMMS-BCN"`, etc., 7-9 caracteres) se copian con
`strncpy(tmp, "...", TEXT_LENGTH_SHORT)` con `TEXT_LENGTH_SHORT == 5`.

El truncamiento a 5 caracteres es **intencionado** (la salida "corta"
nunca debe superar esa longitud, con independencia de los nombres
configurados) y es seguro en cuanto a memoria: `tmp` se reserva con
`mem_alloc(TEXT_LENGTH_SHORT+1)` y se pone a cero con `memset` justo
antes del `switch`, así que aunque `strncpy` no añada el `\0` cuando
trunca, el último byte del buffer ya está a cero de antemano.

Sí existe una colisión de nombres real (p.ej. `"SMR-BTS"`, `"SMR-BTN"` y
`"SMR-BCN"` truncan todas a `"SMR-B"`), pero solo afecta a los
llamantes de `GET_SIC_SHORT`, que son `src/reader_rrd3.c` (deshabilitado
en `build.sh`) y `src/legacy/*.c` (no se compilan desde `build.sh`). El
binario activo (`reader_network64`, `client64`, `client_time64`, ...)
solo usa `GET_SIC_LONG` para el SIC, así que no le afecta hoy.

**Fix:** silenciado con un pragma de diagnóstico local, protegido para
no tocar compiladores antiguos:

```c
#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
        switch (sic[0]) {
            /* ... strncpy(tmp, "SMR-TFN", TEXT_LENGTH_SHORT); ... */
        }
#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic pop
#endif
```

`#pragma GCC diagnostic push/pop` existe desde GCC 4.6 y
`-Wstringop-truncation` desde GCC 8, así que el guard `__GNUC__ >= 8`
hace que en compiladores anteriores el preprocesador ni siquiera vea
las directivas `#pragma` — inerte al 100%, sin impacto en la
compatibilidad hacia atrás que exige este proyecto.

**Nota para el futuro:** si se usa `memcpy` en vez de `strncpy` para
"arreglar" este tipo de warning, ojo — solo es seguro cuando el literal
de origen es *más largo* que el límite copiado. Si se aplicara a
literales más cortos que `TEXT_LENGTH_SHORT` (p.ej. `"LOC"`, `"FRA"`),
`memcpy` leería más allá del literal en memoria (que solo ocupa
`strlen+1` bytes) — un acceso fuera de rango real. Por eso, para las
llamadas `strncpy` con literales que ya caben en el límite, no tocar.

## Modo continuo (`mode_continuous`) en `reader_network`

Por defecto, en modo red (`source = "multicast"`/`"broadcast"`),
`reader_network` graba durante `timed` segundos y **termina el proceso**
(`src/reader_network.c`, condición del `while` principal); para grabar
24h seguidas hacía falta relanzarlo desde `cron` cada `timed` segundos,
solapando ventanas y recortando el solape a posteriori con
`src/utils/filtertime_s.c` + `src/utils/joingps_s.c` para no perder ni
duplicar datos en el borde.

Con `mode_continuous = true` en el `.conf` (ver `bin/conf/example.conf`),
el proceso **no termina nunca por tiempo**: `timed` pasa a ser el
intervalo de rotación del fichero de salida, y el proceso sigue leyendo
la red indefinidamente hasta recibir `SIGTERM`/`SIGINT`. Puntos clave:

- **Rotación alineada a hora absoluta, no relativa al arranque.** Con
  `timed=14400` (4h) el corte ocurre exactamente a las 00:00, 04:00,
  08:00, 12:00, 16:00, 20:00 — nunca "4h después de que arrancó el
  proceso". Se calcula desde `midnight_t` (medianoche del día de
  arranque, `setup_time()`), como `midnight_t + k*timed`; tras cada
  rotación se recalcula el siguiente corte desde ese mismo origen fijo
  (no sumando `timed` al anterior), de forma autocorrectiva si el
  proceso se salta algún corte (p.ej. tras una suspensión del sistema).
- **Compresión (`bzip2`) y subida FTP en background**: cada rotación
  hace `fork()`; el hijo se queda con la copia (copy-on-write) del
  fichero recién cerrado y hace `close_output_file()` +
  `send_output_file()` tal cual ya existían, sin bloquear al padre, que
  sigue leyendo `select()`/`recvfrom()` sin interrupción — así no se
  pierden paquetes UDP durante la rotación. Los hijos de rotación ya
  terminados se cosechan con `waitpid(-1, NULL, WNOHANG)` en cada vuelta
  del bucle — **importante**: NO se usa `signal(SIGCHLD, SIG_IGN)` para
  esto, porque `system()` (usado internamente en `setup_output_file()`
  para el `mkdir` y en `close_output_file()` para `bzip2`) necesita hacer
  su propio `wait()` sobre el hijo que lanza para poder devolver el
  código de salida; con `SIGCHLD` a `SIG_IGN` el kernel re-siega ese hijo
  antes de que `system()` lo pueda leer y `system()` devuelve -1 siempre
  (bug real encontrado y corregido durante las pruebas de esta feature:
  todos los `mkdir`/`bzip2` del proceso fallaban con `SIG_IGN` puesto
  globalmente). No hay límite de hijos concurrentes de compresión/FTP si
  el FTP está caído mucho tiempo — se acepta como limitación conocida.
- **Requiere** `dest_file_timestamp = true` (si no, cada rotación pisaría
  el mismo fichero de nombre fijo antes de que el hijo lo procese) y
  `source` sea `multicast`/`broadcast` (no `file`, que no tiene bucle
  temporizado). `parse_config()` valida esto al arrancar y aborta con
  mensaje claro si no se cumple.
- Si `timed` es 0/no está definido con `mode_continuous=true`, se usa
  7200s (2h) por defecto — un fichero infinito no tiene sentido en este
  modo.
- Funciona igual con la fuente en silencio total: el chequeo de rotación
  está antes del `select()` (que tiene timeout fijo de 10s,
  `SELECT_TIMEOUT`), así que el bucle sigue iterando y rotando aunque no
  llegue ningún paquete.
- El único estado nuevo que se reserva por rotación
  (`dest_file_final_ast`/`dest_file_final_gps`, vía `setup_output_file()`)
  se libera explícitamente en el padre antes de cada rotación siguiente,
  para no acumular memoria en un proceso pensado para no morir nunca.
