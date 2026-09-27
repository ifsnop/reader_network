reader_network 0.82 - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

[![Build Status](https://travis-ci.org/ifsnop/reader_network.svg?branch=master)](https://travis-ci.org/ifsnop/reader_network)
[![Coverity Analysis](https://scan.coverity.com/projects/2418/badge.svg)](https://scan.coverity.com/projects/2418?tab=overview)

Work in progress. Although I use this software everyday (in a realtime quality
monitorization environment), this software is EXPERIMENTAL.

Copyright (C) 2002-2026 Diego Torres <diego dot torres at gmail dot com>

See `bin/conf/example.conf` as a working example of the reader_network
config file; the reference below explains what each option does.

## Modos de funcionamiento (referencia rápida)

Nota: pensado como referencia para quien ya conoce el proyecto, no como
manual paso a paso. El detalle completo de cada clave está comentado en
`bin/conf/example.conf`.

**Entrada** (`source`):
- `file`: procesa un fichero de una vez (`source_file`,
  `source_file_gps_version`) y termina; ignora `timed`.
- `multicast` / `broadcast`: lee de red en bucle (`select()` + `recvfrom()`)
  hasta que corta por `timed` o, en `mode_continuous`, hasta SIGTERM/SIGINT.

**Duración / rotación** (`timed`, `mode_continuous`):
- `mode_continuous = false` (por defecto): `timed` es la duración total en
  segundos; al cumplirse, el proceso comprime, sube por FTP si aplica, y
  termina. Pensado para relanzarse periódicamente desde `cron`.
- `mode_continuous = true`: `timed` pasa a ser el **intervalo de
  rotación**, alineado a hora absoluta del reloj (p.ej. `timed=14400`
  rota exactamente a las 00/04/08/12/16/20h, no "cada 4h desde que
  arrancó"). El proceso no termina solo; se para con SIGTERM/SIGINT.
  Cada rotación comprime y sube el fichero recién cerrado en un proceso
  hijo en background, sin interrumpir la captura. Requiere
  `dest_file_timestamp = true` y `source` multicast/broadcast. Sustituye
  al patrón cron + solape + recorte posterior (`filtertime_s`/`joingps_s`)
  para grabaciones 24/7 sin huecos ni duplicados.

**Otros modos**:
- `mode_daemon`: si es `true`, hace fork a segundo plano (`daemon(1,0)`
  en Linux) y cierra la salida estándar.
- `mode_scrm`: descarta paquetes duplicados (mismo CRC32+tamaño,
  recibidos por rutas distintas) dentro de una ventana de unos segundos;
  es estado independiente del fichero de salida, funciona igual con o
  sin `mode_continuous`.
- `dest_localhost`: además de grabar, reenvía los plots ya decodificados
  a un grupo multicast en localhost para que otras utilidades (`client`,
  `client_time`) los consuman en tiempo real.

## Fichero de salida

- `dest_file`: ruta base, o directorio si `dest_file_timestamp = true`.
- `dest_file_timestamp`: nombra con fecha/hora
  (`MM/DD/YYMMDD-region-HHMMSS`); requerido por `mode_continuous`.
- `dest_file_nodirectory`: nombre con fecha pero sin subdirectorios.
- `dest_file_region`: prefijo/etiqueta opcional en el nombre.
- `dest_file_format` (`ast` | `gps`) y `dest_file_extension`: `ast` graba
  el ASTERIX crudo; `gps` añade 10 bytes de timestamp a cada datablock
  (ver el código fuente para el formato exacto).
- `dest_file_compress`: comprime con `bzip2` al cerrar el fichero.
- `dest_free_space`: aborta si el espacio libre baja de N MB (solo con
  `dest_file_timestamp`).
- `dest_ftp_uri`: lista de URIs FTP donde subir cada fichero ya cerrado
  (usuario:contraseña en la URI, o anónimo si no se indica).

## Radares / red y control de versión

- `radar_definition`: 5 campos por radar — nombre, grupo multicast,
  puerto, ip de origen (`0.0.0.0` para aceptar cualquiera), ip de la
  interfaz local por la que escuchar.
- `asterix_versions`: lista de hashes MD5 permitidos para arrancar,
  calculado normalmente desde `/var/lib/dbus/machine-id` (o forzado con
  la variable de entorno `asterix_versions`); `reader_network64 -r`
  imprime el hash de la máquina actual.

## Otras claves

- `timed_stats_interval`: cada cuántos segundos se vuelcan estadísticas
  por pantalla (modo no-daemon).
- `dest_screen_crc`: vuelca los CRC32 de cada paquete por pantalla
  (depuración, requiere `mode_scrm`).
- `dest_filter_selection` (p.ej. `FILTER_GROUND`): filtra plots; solo
  disponible con `source = "file"`.

## Utilidades relacionadas (`src/utils/`)

- `filtertime_s`: recorta un fichero `.gps` a un rango horario.
- `joingps` / `joingps_s`: fusionan cronológicamente dos ficheros `.gps`.
- `cleanast` / `cleanast_s`: quitan el envoltorio GPS, dejan ASTERIX puro.
- `filtercat*` / `filtersacsic*`: filtran por categoría ASTERIX o SAC/SIC.

---

If you want to understand what ASTERIX is and how it is used, you
should check the following documents from EUROCONTROL and the
ASTERIX EUROCONTORL website

http://www.eurocontrol.int/asterix/public/subsite_homepage/homepage.html
http://www.eurocontrol.int/asterix/public/standard_page/documents.html

EUROCONTROL STANDARD DOCUMENT FOR RADAR DATA EXCHANGE Part 2a Transmission of Monoradar Data Target Reports
SUR.ET1.ST05.2000-STD-02a-01 1.1 August 2002
http://www.eurocontrol.int/asterix/gallery/content/public/documents/astx2a12.pdf

EUROCONTROL STANDARD DOCUMENT FOR SURVEILLANCE DATA EXCHANGE Part 2b Transmission of Monoradar Service Messages
SUR.ET1.ST05.2000-STD-02b-01 1.26 November 2000
http://www.eurocontrol.int/asterix/gallery/content/public/documents/astx2b1.pdf

What is ASTERIX ?

ASTERIX is the EUROCONTROL Standard for the exchange of Surveillance related data.
The acronym stands for "All Purpose STructured Eurocontrol SuRveillance Information EXchange".

ASTERIX provides a structured approach to a message format to be applied in the exchange of surveillance related information for various applications. Developed by the SuRveillance Data Exchange Task Force (RDE-TF) with its multinational participation, it ensures a common data representation, thereby facilitating the exchange of surveillance data in an international context.

Having started as a EUROCONTROL development (see the "history" section for further details), ASTERIX is now applied worldwide.

ASTERIX defines a structured approach to the encoding of surveillance data. Categories group the information related to a specific application. They consist of a number of data items which are defined in the ASTERIX documents down to the bit-level.

DOCUMENTS

EUROCONTROL STANDARD DOCUMENT FOR RADAR SURVEILLANCE IN EN-ROUTE AIRSPACE AND MAJOR TERMINAL AREAS
SUR.ET1.ST01.1000-STD-01-01 1.0 March 1997

EUROCONTORL RADAR SENSOR PERFORMANCE ANALYSIS
SUR.ET1.ST03.1000-STD-01-01 0.1 June 1997

DRAFT IMPLEMENTING RULE ON SURVEILLANCE PERFORMANCE AND INTEROPERABILITY REQUIREMENTS EUROCONTROL. European mode S Station Surveillance Coordination Interface Control Document. SUR/MODES/EMS/ICD-01.

European mode S Station Functional Specification. SUR/MODES/EMS/SPE-01. Version 3.11

Specification for ATM Surveillance System Performance EUROCONTROL-SPEC-0147 0.35 01/09/2011
