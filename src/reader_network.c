/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2019 Diego Torres <diego dot torres at gmail dot com>

This file is part of the reader_network utils.

reader_network is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

reader_network is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with reader_network. If not, see <http://www.gnu.org/licenses/>.
*/

#include "includes.h"

extern unsigned char full_tod[MAX_RADAR_NUMBER*TTOD_WIDTH]; /* 2 sacsic, 1 null, 3 full_tod, 2 max_ttod */

float current_time_today = 0.0;
struct sockaddr_in cliaddr,srvaddr;

bool enabled = false;
long dest_free_space = -1;
bool dest_file_gps = false, source_file_gps = false;
bool dest_file_compress = false;
bool dest_file_timestamp = false;
bool dest_localhost = false;
bool dest_screen_crc = false;
char **dest_filter_selection = NULL; // parsed from config
int dest_filter_count = 0;
int dest_filter_flags = FILTER_NONE; // config translated
bool mode_daemon = false;
bool mode_scrm = false;
long timed = 0;
long timed_stats_interval = 0;
time_t midnight_t; //segundos desde el 1-1-1970 hasta las 00:00:00 del dia actual
char *region_name = NULL; //not used in reader_networ, but in reader_rrd3 & asterix
char *source, *dest_file = NULL, *dest_file_region = NULL,
    *dest_file_extension = NULL,
    *dest_file_final_ast = NULL, *dest_file_final_gps = NULL,
    **dest_ftp_uri = NULL,
    *source_file = NULL, **radar_definition = NULL;
int *radar_counter = NULL; // plots recibidos por cada flujo
int *radar_counter_bytes = NULL; // bytes recibidos por cada flujo
int dest_file_format = DEST_FILE_FORMAT_AST;
int dest_ftp_count = 0; // número de entrads en el array de ftp de destino de los ficheros.
int radar_count = 0; // numero de entradas en el array de definicion de radares.
    // para saber el numero de radares, hay que dividir entre 5! (5 columnas por radar)
int socket_count = 0, s_output_multicast = -1, 
    offset = 0; // post bytes que van detrás de cada datablock (por ejemplo, cuando el asterix va fechado con gps
int *s_reader = NULL; // listado de sockets para leer en el caso de red (multicast, etc)
int fd_in = -1, fd_out_ast = -1, fd_out_gps = -1;
long source_file_gps_version=3;
rb_red_blk_tree* tree = NULL;
char **asterix_versions = NULL;
int asterix_versions_count = 0;
char *digest_hex = NULL;

struct Queue {
    rb_red_blk_node **node;
    //[SCRM_MAX_QUEUE_SIZE];
    int front, rear;
    int count;
};

struct Queue q;

struct radar_destination_s {
    int socket; // socket descriptor
    char dest_ip[256]; // destination multicast address
};

struct radar_destination_s radar_destination[MAX_RADAR_NUMBER];

void setup_asterix_versions() {
    int read_size = 0, i;
    FILE *fd = NULL;
    struct stat sb;
    unsigned char *buf = NULL;
    MD5_CTX ctx;
    unsigned char digest[16];
    char *env = NULL;
/*
    FILE* fstab = setmntent("/etc/mtab", "r");
    struct mntent *e;
    const char *devname = NULL;
    while ((e = getmntent(fstab))) {
        if (strcmp("/", e->mnt_dir) == 0) {
            devname = e->mnt_fsname;
            break;
        }
    }
    printf("root devname is %s\n", devname);
    endmntent(fstab);
*/
// "/var/lib/dbus/machine-id"
// "/sys/class/dmi/id/board_serial"

    if ( ((env = getenv("asterix_versions")) != NULL) && strlen(env) == 32 ) {
        digest_hex = (char *) mem_alloc(16*2 + 1); //md5 length in ascii + null terminator3
        strncpy(digest_hex, env, 32);
        digest_hex[32] = '\0';
        return;
    }

    if ( stat("/var/lib/dbus/machine-id", &sb) == -1 ) { /* To obtain file size */
	log_printf(LOG_VERBOSE, "asterix_versions fstat error (%s)\n", strerror(errno)); return; // exit(EXIT_FAILURE);
    }
    if (!S_ISREG(sb.st_mode)) {
	log_printf(LOG_VERBOSE, "asterix_versions file error (not a file)\n"); return; // exit(EXIT_FAILURE);
    }
    buf = mem_alloc(sb.st_size + 1);

    if ( (fd = fopen("/var/lib/dbus/machine-id", "r")) == NULL ) {
	log_printf(LOG_VERBOSE, "asterix_versions open error (%s)\n", strerror(errno)); free(buf); return; // exit(EXIT_FAILURE);
    }
    if ( (read_size = fread(buf, 1, sb.st_size, fd)) != sb.st_size ) {
	if ( !feof(fd) ) {
	    log_printf(LOG_VERBOSE, "asterix_versions fread error (%s)\n", strerror(errno));
	    mem_free(buf); fclose(fd); return; // exit(EXIT_FAILURE);
	}
    }
    buf[read_size] = '\0';
    MD5_Init(&ctx);
    MD5_Update(&ctx, buf, read_size);
    MD5_Final(digest, &ctx);
    mem_free(buf);
    //sprintf(digest_hex, "%s", digest);
    digest_hex = (char *) mem_alloc(16*2 + 1); //md5 length in ascii + null terminator3

    for(i=0; i<16; i++)
	sprintf(digest_hex + i*2, "%02x", digest[i]);
    digest_hex[16*2] = '\0';

    if ( fclose(fd) != 0 ) {
	log_printf(LOG_ERROR, "asterix_versions close error\n"); exit(EXIT_FAILURE);
    }
    return;
}

void parse_config(char *conf_file) {
char *dest_file_format_string = NULL;

    if (!cfg_open(conf_file)) {
	log_printf(LOG_ERROR, "please create reader_network.conf or check for duplicate entry\n");
	exit(EXIT_FAILURE);
    }
    cfg_get_bool(&enabled, "enabled");
    if (enabled == false) {
	log_printf(LOG_ERROR, "please modify your config.file\n");
	exit(EXIT_FAILURE);
    }

    cfg_get_bool(&mode_daemon, "mode_daemon");
    cfg_get_int(&timed_stats_interval, "timed_stats_interval");
    if (mode_daemon) {
#if defined(__sun)
	long retcode; int fd = 0;
#endif
        log_printf(LOG_VERBOSE, "going to daemon mode\n");
	if (timed_stats_interval>0) {
	    log_printf(LOG_VERBOSE, "advanced stats disabled in daemon mode\n"); 
	    timed_stats_interval = 0;
	}
	log_flush();
	log_close();
#if defined(__linux)
	if (daemon(1,0) == -1) {
	    log_printf(LOG_ERROR, "ERROR daemon: %s\n",strerror(errno));
	    exit(EXIT_FAILURE);
	}
#endif
#if defined(__sun)
	if(getppid()==1) {
	    log_printf(LOG_ERROR, "ERROR getppid (already a daemon): %s\n", strerror(errno)); exit(EXIT_FAILURE);
	}
	if ( (retcode = fork())<0 ) {
	    log_printf(LOG_ERROR, "ERROR fork: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	} else {
	    if (retcode>0) {
		log_printf(LOG_VERBOSE, "parent exiting\n"); exit(EXIT_SUCCESS);
	    }
	}
	// child (daemon) continues
	setsid(); // obtain a new process group
	// close all open filehandlers
	if ( (retcode = sysconf(_SC_OPEN_MAX)) <0 ) {
	    log_printf(LOG_ERROR, "ERROR sysconf: %s\n", strerror(errno)); exit(EXIT_FAILURE);
	}
	while (fd < retcode) close(fd++);
	retcode = open("/dev/null",O_RDWR); dup(retcode); dup(retcode); // handle standart I/O
	umask(027); // set newly created file permissions -> 750
	 // chdir("/"); // change running directory
#endif
    } else {
	log_printf(LOG_VERBOSE, "interactive version\n");
	if (timed_stats_interval>0)
	    log_printf(LOG_VERBOSE, "displaying advanced stats every %ld secs\n", timed_stats_interval);
    }

    if (cfg_get_bool(&mode_scrm, "mode_scrm")) {
	if (mode_scrm) {
	    log_printf(LOG_VERBOSE, "enable scrm crc/high availability data\n");
	} else {
	    log_printf(LOG_VERBOSE, "normal recording mode (no scrm/ha)\n");
	}
    } else {
	log_printf(LOG_VERBOSE, "normal recording mode (no scrm/ha)\n");
    }

    if (cfg_get_bool(&dest_screen_crc, "dest_screen_crc")) {
	if (dest_screen_crc) {
	    log_printf(LOG_VERBOSE, "outputing pkt crc\n");
	} else {
	    log_printf(LOG_VERBOSE, "not displaying pkt crc\n");
	}
    } else {
	log_printf(LOG_VERBOSE, "not displaying pkt crc\n");
    }

    cfg_get_str_array(&asterix_versions, &asterix_versions_count, "asterix_versions");
    if (0 == asterix_versions_count) {
        log_printf(LOG_ERROR, "asterix_versions entry missing\n");
        exit(EXIT_FAILURE);
    }
    if (NULL == digest_hex) {
        log_printf(LOG_VERBOSE, "asterix_versions not parsed\n");
    } else {
        int i;
        bool equal = false;
        for ( i=0; i<asterix_versions_count; i++) {
	    int j = 0;
	    if ( strlen(asterix_versions[i]) != 32 ) {
	        log_printf(LOG_ERROR, "asterix_versions corrupted\n");
	        exit(EXIT_FAILURE);
            }
            equal = true;
	    for( j = 0; j<32; j++)
	    if ( digest_hex[j] != asterix_versions[i][j] )
	        equal = false;
            if (equal)
                break;
        }
        if (equal) {
	    log_printf(LOG_VERBOSE, "asterix_versions match\n");
	} else {
	log_printf(LOG_ERROR, "asterix_versions mismatch\n");
	exit(EXIT_FAILURE);
	}
    }
    
    if (!cfg_get_str(&source, "source")) {
	log_printf(LOG_VERBOSE, "source must be file, multicast or broadcast\n");
	exit(EXIT_FAILURE);
    }
    if (!strncasecmp(source, "file", 4)) {
	if (!cfg_get_str(&source_file, "source_file")) {
	    log_printf(LOG_ERROR, "source_file entry missing\n");
	    exit(EXIT_FAILURE);
	} else {
	    log_printf(LOG_VERBOSE, "input data from file: %s\n", source_file);
	    if (!strcasecmp(source_file + strlen(source_file) - 3, "gps")) {
		source_file_gps = true;
		if (!cfg_get_int(&source_file_gps_version, "source_file_gps_version")) {
		    log_printf(LOG_ERROR, "source_file_gps_version entry missing\n");
		    exit(EXIT_FAILURE);
		} else {
                    if (source_file_gps_version == 0) {
		        log_printf(LOG_VERBOSE, "AutoGPS input activated\n");
		        offset = 0; //later
		    } else if (source_file_gps_version == 1) {
			log_printf(LOG_VERBOSE, "GPSv1 input activated\n");
			offset = 10;
		    } else if (source_file_gps_version == 2) {
			log_printf(LOG_VERBOSE, "GPSv2 input activated\n");
			offset = 4;
		    } else {
			log_printf(LOG_ERROR, "GPS version not supported\n");
			exit(EXIT_FAILURE);
		    }
		}
	    } else {
		log_printf(LOG_VERBOSE, "AST input activated\n");
	    }
	}
    } else if (!strncasecmp(source, "mult", 4)) {
	if (!cfg_get_str_array(&radar_definition, &radar_count, "radar_definition")) {
	    log_printf(LOG_ERROR, "radar_definition entry missing\n");
	    exit(EXIT_FAILURE);
	    if (radar_count>MAX_RADAR_NUMBER) {
		log_printf(LOG_ERROR, "maximum number of entries in radar_definition (%d > %d)\n", radar_count, MAX_RADAR_NUMBER);
		exit(EXIT_FAILURE);
	    }
	}
        log_printf(LOG_VERBOSE, "reading from multicast\n");
        memset(radar_destination, 0, sizeof(struct radar_destination_s)*MAX_RADAR_NUMBER);
    } else if (!strncasecmp(source, "broa", 4)) {
	if (!cfg_get_str_array(&radar_definition, &radar_count, "radar_definition")) {
	    log_printf(LOG_ERROR, "radar_definition entry missing\n");
	    exit(EXIT_FAILURE);
	}
        log_printf(LOG_VERBOSE, "reading from broadcast\n");
    }
    if (cfg_get_int(&timed, "timed")) {
        if (timed == 0) 
	    log_printf(LOG_VERBOSE, "recording forever (user interrupt or input file interrupt)\n");
	else
	    log_printf(LOG_VERBOSE, "recording for %ld secs only\n", timed);
    }
    if ( cfg_get_int(&dest_free_space, "dest_free_space") ) {
	log_printf(LOG_VERBOSE, "minimum free space configured at %ld Mb\n", dest_free_space);
    }

    if (cfg_get_bool(&dest_localhost, "dest_localhost")) {
	if (dest_localhost) {
	    log_printf(LOG_VERBOSE, "enable localhost decoding\n");
	} else {
	    log_printf(LOG_VERBOSE, "not enabling localhost decoding\n");
	}
    }

    if (cfg_get_str(&dest_file_region, "dest_file_region")) {
	log_printf(LOG_VERBOSE, "output file region: %s\n", dest_file_region);
    } else {
	log_printf(LOG_VERBOSE, "no region prefix\n");
    }

    if (cfg_get_str(&dest_file_extension, "dest_file_extension")) {
	log_printf(LOG_VERBOSE, "output file extension: %s\n", dest_file_extension);
    } else {
	log_printf(LOG_VERBOSE, "no extension\n");
    }

    if (cfg_get_str(&dest_file, "dest_file")) {
	log_printf(LOG_VERBOSE, "output data to file (1):%s\n", dest_file);
	if (cfg_get_bool(&dest_file_timestamp, "dest_file_timestamp")) {
	    if (dest_file_timestamp) log_printf(LOG_VERBOSE, "appending date+time to output file\n");
	}
	if (cfg_get_bool(&dest_file_compress, "dest_file_compress")) {
	    if (dest_file_compress) log_printf(LOG_VERBOSE, "compress output file at end of recording\n");
	}
	if (cfg_get_str(&dest_file_format_string, "dest_file_format")) {
	    log_printf(LOG_VERBOSE, "parsing output...");
	    if (!strcasecmp(dest_file_format_string, "gps")) {
		dest_file_format = DEST_FILE_FORMAT_GPS;
		log_printf(LOG_VERBOSE, "GPS output activated");
	    } else if (!strcasecmp(dest_file_format_string, "ast")) {
		dest_file_format = DEST_FILE_FORMAT_AST;
		log_printf(LOG_VERBOSE, "AST output activated");
	    } else {
		    dest_file_format = DEST_FILE_FORMAT_AST;
		    log_printf(LOG_VERBOSE, "AST output activated (default)");
	    }
	    log_printf(LOG_VERBOSE, "\n");
	    mem_free(dest_file_format_string);
	}
	if (cfg_get_str_array(&dest_ftp_uri, &dest_ftp_count, "dest_ftp_uri")) {
	    log_printf(LOG_VERBOSE, "ftp upload enabled\n");
	}
    } else {
	log_printf(LOG_VERBOSE, "no output selected\n");
    }

    if (cfg_get_str_array(&dest_filter_selection, &dest_filter_count, "dest_filter_selection")) {
        int i;
        if (strncasecmp(source, "file", 4)) { // not equal
            log_printf(LOG_ERROR, "dest_filter_selection is only available when source = \"file\"\n");
            exit(EXIT_FAILURE);
        }
        for (i=0; i<dest_filter_count; i++) {
            if (!strncasecmp(dest_filter_selection[i], "FILTER_GROUND", 13)) {
                dest_filter_flags |= FILTER_GROUND;
                log_printf(LOG_VERBOSE, "filtering ground plots\n");
            }
        }
    }


    if (dest_screen_crc && timed_stats_interval != 0) {
	log_printf(LOG_VERBOSE, "disabling timed_stats_interval (dest_screen_crc set)\n");
	timed_stats_interval = 0;
    }

    cfg_close();

    return;
}

void setup_output_file(void) {
char *gpsheader = NULL, *dest_file_region_parsed = NULL;
struct timeval t;
struct tm *t2 = NULL;

    gpsheader = (char *) mem_alloc(2200);
    gpsheader = memset(gpsheader, 0xcd, 2200);

    if (dest_file != NULL) {
	if ( dest_file_timestamp == true && dest_free_space != -1 ) {
	    double result = 0;
	    struct statvfs sfs;
	    if ( statvfs(dest_file, &sfs) == -1 ) {
		log_printf(LOG_ERROR, "ERROR statvfs: %s -> %s\n", strerror(errno), dest_file); exit(EXIT_FAILURE);
	    } else
		result = (double)((unsigned long long)sfs.f_bsize * sfs.f_bfree)/(1024*1024);
	    if (result > 0) {
		if ( result < dest_free_space ) {
		    log_printf(LOG_ERROR, "not enough free space %.2f Mb / %ld Mb\n", result, dest_free_space);
		    exit(EXIT_FAILURE);
		}
	    }
	}

	if ( (dest_file_region != NULL) && (strlen(dest_file_region)>0) ) {
	    dest_file_region_parsed = (char *) mem_alloc(strlen(dest_file_region)+1+2);
	    snprintf(dest_file_region_parsed, (strlen(dest_file_region) + 4), "-%s-", dest_file_region);
	}

	if (dest_file_timestamp) {
	    if (gettimeofday(&t, NULL) !=0 ) {
		log_printf(LOG_ERROR, "ERROR gettimeofday: %s\n", strerror(errno)); exit(EXIT_FAILURE);
	    }
	    if ((t2 = gmtime(&t.tv_sec))==NULL) {
		log_printf(LOG_ERROR, "ERROR gmtime: %s\n", strerror(errno)); exit(EXIT_FAILURE);
	    }
	}
	if ((dest_file_format & DEST_FILE_FORMAT_AST ) == DEST_FILE_FORMAT_AST) {
	    dest_file_final_ast = (char *) mem_alloc(512 + strlen(dest_file));
	    if (dest_file_timestamp) {
		int res = 0;
		//sprintf(dest_file_final_ast, "%s_%04d%02d%02d%02d%02d%02d.ast", dest_file, t2->tm_year+1900, t2->tm_mon+1, t2->tm_mday,
		//    t2->tm_hour, t2->tm_min, t2->tm_sec);
		snprintf(dest_file_final_ast, 512, "/bin/mkdir -p %s/%02d/%02d/", dest_file, t2->tm_mon+1, t2->tm_mday);
		if ( ((res = system(dest_file_final_ast)) == -1) || (WEXITSTATUS(res) != 0) ) {
		    log_printf(LOG_ERROR, "ERROR makdir %s\n", dest_file_final_ast); exit(EXIT_FAILURE);
		}
		snprintf(dest_file_final_ast, 512, "%s/%02d/%02d/%02d%02d%02d%s%02d%02d%02d.%s", dest_file, t2->tm_mon+1, t2->tm_mday,
		    t2->tm_year % 100, t2->tm_mon+1, t2->tm_mday,
		    (dest_file_region_parsed != NULL ? dest_file_region_parsed : "-"),
		    t2->tm_hour, t2->tm_min, t2->tm_sec,
		    (dest_file_extension != NULL ? dest_file_extension : "ast"));
	    } else {
		sprintf(dest_file_final_ast, "%s.%s", dest_file, (dest_file_extension != NULL ? dest_file_extension : "ast") );
	    }
	    log_printf(LOG_NORMAL, "output data to file (2):%s\n", dest_file_final_ast);
	    if ( (fd_out_ast = open(dest_file_final_ast, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR 
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
		log_printf(LOG_ERROR, "ERROR open(%s): %s\n", dest_file_final_ast, strerror(errno)); exit(EXIT_FAILURE);
	    }
	}
	if ((dest_file_format & DEST_FILE_FORMAT_GPS) == DEST_FILE_FORMAT_GPS) {
	    dest_file_final_gps = (char *) mem_alloc(512 + strlen(dest_file));
	    if (dest_file_timestamp) {
		int res = 0;
		snprintf(dest_file_final_gps, 512, "/bin/mkdir -p %s/%02d/%02d/", dest_file, t2->tm_mon+1, t2->tm_mday);
		if ( ((res = system(dest_file_final_gps)) == -1) || (WEXITSTATUS(res) != 0) ) {
		    log_printf(LOG_ERROR, "ERROR mkdir: %s\n", dest_file_final_gps); exit(EXIT_FAILURE);
		}
		snprintf(dest_file_final_gps, 512, "%s/%02d/%02d/%02d%02d%02d%s%02d%02d%02d.%s", dest_file, t2->tm_mon+1, t2->tm_mday,
		    t2->tm_year % 100, t2->tm_mon+1, t2->tm_mday,
		    (dest_file_region_parsed != NULL ? dest_file_region_parsed : "-"),
		    t2->tm_hour, t2->tm_min, t2->tm_sec,
		    (dest_file_extension != NULL ? dest_file_extension : "gps"));
		//sprintf(dest_file_final_gps, "%s/%02d/%02d/%02d%02d%02d.gps", dest_file, t2->tm_mon+1, t2->tm_mday,
		//    t2->tm_hour, t2->tm_min, t2->tm_sec);
	    } else {
		sprintf(dest_file_final_gps, "%s.%s", dest_file, (dest_file_extension != NULL ? dest_file_extension : "gps") );
	    }
	    log_printf(LOG_NORMAL, "output data to file (2):%s\n", dest_file_final_gps);
	    if ( (fd_out_gps = open(dest_file_final_gps, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR 
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
		log_printf(LOG_ERROR, "ERROR open(%s): %s\n", dest_file_final_gps, strerror(errno)); exit(EXIT_FAILURE);
	    }
	    if (write(fd_out_gps, gpsheader, 2200)!=2200) {
		log_printf(LOG_ERROR, "ERROR write gps file init: %s\n", strerror(errno)); exit(EXIT_FAILURE);
	    }
	}
    }
    if (dest_file_region_parsed != NULL)
	mem_free(dest_file_region_parsed);
    mem_free(gpsheader);
    return;
}

void setup_priority(int i) {
int ret = 0;
int pid = 0;
    log_printf(LOG_VERBOSE, "checking root...");
    if (getuid() == 0) {
	log_printf(LOG_VERBOSE, "we are root...changing priority");
	errno = 0; ret = 0; pid = getpid();
	if ( ((ret = getpriority(PRIO_PROCESS, pid)) == -1) && (errno != 0) ) {
	    log_printf(LOG_VERBOSE, "\nERROR getpriority: %s\n", strerror(errno));
	} else {
	    log_printf(LOG_VERBOSE, " %d => %d", ret, i);
	    if ( setpriority(PRIO_PROCESS, pid , i) == -1 ) {
		log_printf(LOG_VERBOSE, "\nERROR setpriority: %s\n", strerror(errno));
	    } else {
		log_printf(LOG_VERBOSE, "...done!\n");
	    }
	}
    } else {
	log_printf(LOG_VERBOSE, "we are NOT root!\n");
    }
    return;
}

void close_output_file() {
char tmp[1024];
int res=0;

    if (dest_file_compress) {
	setup_priority(0);
	if ((dest_file_format & DEST_FILE_FORMAT_AST) == DEST_FILE_FORMAT_AST) {
	    close(fd_out_ast);
	    snprintf(tmp, 1023, "bzip2 -f %s", dest_file_final_ast);
	    if ((res = system(tmp)) == -1) {
		log_printf(LOG_ERROR, "ERROR compressing %s\n", dest_file_final_ast);
		exit(EXIT_FAILURE);
	    } else {
		log_printf(LOG_VERBOSE, "output file %s compressed succesfully (%d)\n", dest_file_final_ast, res);
	    }
	}
	if ((dest_file_format & DEST_FILE_FORMAT_GPS) == DEST_FILE_FORMAT_GPS) {
	    close(fd_out_gps);
	    snprintf(tmp, 1023, "bzip2 -f %s", dest_file_final_gps);
	    if ((res = system(tmp)) == -1) {
		log_printf(LOG_ERROR, "ERROR compressing %s\n", dest_file_final_gps);
		exit(EXIT_FAILURE);
	    } else {
		log_printf(LOG_VERBOSE, "output file %s compressed succesfully (%d)\n", dest_file_final_gps, res);
	    }
	}
    }
    return;
}

void send_output_file() {
    CURL *ch = NULL;
    CURLcode res = 0;
    FILE *fh = NULL;
    char file[1024];
    char buff_1[1024];
    char buff_2[CURL_ERROR_SIZE];
    const char buff_ftpactive[] = "-";
    struct stat file_info;
    curl_off_t fsize;
    char noproxy_host[1024];
    int slash_pos = 6, i = 0, j = 0;

    memset(file, 0, 1024);

    if ((dest_file_format & DEST_FILE_FORMAT_AST) == DEST_FILE_FORMAT_AST) {
	snprintf(file, 1023, "%s%s", dest_file_final_ast, (dest_file_compress ? ".bz2" : "") );
    } else if ((dest_file_format & DEST_FILE_FORMAT_GPS) == DEST_FILE_FORMAT_GPS) {
	snprintf(file, 1023, "%s%s", dest_file_final_gps, (dest_file_compress ? ".bz2" : "") );
    }

    memset(&file_info, 0, sizeof(struct stat));

    if (stat(file, &file_info)) {
        log_printf(LOG_ERROR, "ERROR stat '%s': %s\n", file, strerror(errno));
        exit(EXIT_FAILURE);
    }
    fsize = (curl_off_t)file_info.st_size;
    if ( unsetenv("http_proxy")==-1 ) log_printf(LOG_ERROR, "ERROR unsetenv http_proxy\n");
    if ( unsetenv("ftp_proxy")==-1 ) log_printf(LOG_ERROR, "ERROR unsetenv ftp_proxy\n");

    for(i=0;i<dest_ftp_count;i++) {
	if ( dest_ftp_uri[i][strlen(dest_ftp_uri[i])-1]!='/' )
	    snprintf(buff_1, 1023, "%s/%s", dest_ftp_uri[i], basename(file));
	else
	    snprintf(buff_1, 1023, "%s%s", dest_ftp_uri[i], basename(file));
	log_printf(LOG_VERBOSE, "uploading %s\n", buff_1);

        if ( (fh = fopen(file, "rb")) == NULL) {
	    log_printf(LOG_ERROR, "ERROR open(%s): %s\n", file, strerror(errno));
	    exit(EXIT_FAILURE);
	}

	while ( (dest_ftp_uri[i][slash_pos] != '/') && (slash_pos<strlen(dest_ftp_uri[i])) ) slash_pos++;
	snprintf(noproxy_host, slash_pos - 5, "%s", dest_ftp_uri[i] + 6);

	if ( setenv("NO_PROXY", noproxy_host,1)==-1 ) log_printf(LOG_ERROR, "ERROR setenv NO_PROXY\n");
        //system("echo poniendo noproxy\n"); system("echo _${NO_PROXY}_\n"); exit(EXIT_FAILURE);

	curl_global_init(CURL_GLOBAL_ALL);
	ch = curl_easy_init();
	if(ch) {
	    curl_easy_setopt(ch, CURLOPT_VERBOSE, 0L);

	    /* enable uploading */
	    curl_easy_setopt(ch, CURLOPT_UPLOAD, 1L);

	    /* specify target */
	    curl_easy_setopt(ch, CURLOPT_URL, buff_1);
	    curl_easy_setopt(ch, CURLOPT_HTTPPROXYTUNNEL, 0L);

	    /* disable PASSIVE transfers http://curl.haxx.se/libcurl/c/CURLOPT_FTPPORT.html */
	    curl_easy_setopt(ch, CURLOPT_FTPPORT, buff_ftpactive);

            /* only one CWD https://curl.haxx.se/libcurl/c/CURLOPT_FTP_FILEMETHOD.html */
	    curl_easy_setopt(ch, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_SINGLECWD);

	    /* https://curl.haxx.se/libcurl/c/CURLOPT_FTP_USE_EPRT.html */
	    /* If the value is 1, it tells curl to use the EPRT command when doing active FTP downloads (which is enabled by CURLOPT_FTPPORT). */
	    curl_easy_setopt(ch, CURLOPT_FTP_USE_EPRT, 0L);

	    /* now specify which file to upload */
	    curl_easy_setopt(ch, CURLOPT_READDATA, fh);

	    /* Set the size of the file to upload (optional).  If you give a *_LARGE
	    option you MUST make sure that the type of the passed-in argument is a
	    curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
	    make sure that to pass in a type 'long' argument. */
	    curl_easy_setopt(ch, CURLOPT_INFILESIZE, (long)(curl_off_t)fsize);
	    curl_easy_setopt(ch, CURLOPT_ERRORBUFFER, buff_2);

	    /* Now run off and do what you've been told! */

            for(j = 0; j<10; j++) {
                log_printf(LOG_VERBOSE, "sending ftp file, curl try number(%d)\n", j+1);
                res = curl_easy_perform(ch);
	        if (res == CURLE_OK) {
	            break;
	        }
                log_printf(LOG_ERROR, "ERROR curl(1): %s\nERROR curl(2): %s\n", curl_easy_strerror(res), buff_2);
                sleep(2);
            }

	    /* always cleanup */
	    curl_easy_cleanup(ch);
	}

	fclose(fh); /* close the local file */
	curl_global_cleanup();
    }

    return;
}

void setup_output_multicast(void) {
struct hostent *h;
int loop=1;

    if ( (h = gethostbyname(MULTICAST_PLOTS_GROUP)) == NULL ) {
	log_printf(LOG_ERROR, "ERROR gethostbyname");
	exit(EXIT_FAILURE);
    }
    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
    memset(&srvaddr, 0, sizeof(struct sockaddr_in));
    srvaddr.sin_family = h->h_addrtype;
    memcpy((char*) &srvaddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    srvaddr.sin_port = htons(MULTICAST_PLOTS_PORT);
    if ( (s_output_multicast = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	log_printf(LOG_ERROR, "ERROR socket: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //htonl(INADDR_ANY);
    cliaddr.sin_port = htons(0);
    if (  bind(s_output_multicast, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) <0 ) {
	log_printf(LOG_ERROR, "ERROR socket: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    if ( setsockopt(s_output_multicast, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0 ) {
	log_printf(LOG_ERROR, "ERROR setsockopt: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return;
}

void setup_input_network(void) {
    struct sockaddr_in cast_group;
    struct ip_mreq mreq;
    int i = 0, yes = 1;

    socket_count = 0;

	    while (i<(radar_count/5)) {
		if (i>0 && 								     // si
		    !strcasecmp(radar_definition[(i*5)+1], radar_definition[((i-1)*5)+1]) && // mismo grupo mcast
		    !strcasecmp(radar_definition[(i*5)+2], radar_definition[((i-1)*5)+2])) { // y mismo puerto

//		    strncpy(radar_destination[i], radar_definition[(i*5)+1, 255);
		    log_printf(LOG_VERBOSE, "%d] desc(%s) dest(%s:%s) src(%s) ifaz(%s)\n", i,
			radar_definition[i*5], radar_definition[(i*5)+1],
			radar_definition[(i*5)+2], radar_definition[(i*5)+3],
			radar_definition[(i*5)+4]);

		    radar_destination[i].socket = s_reader[socket_count-1];
		    strncpy(radar_destination[i].dest_ip, radar_definition[(i*5)+1], 255);
											     // no te suscribas 
		    //(para distintas ips de origien pero mismos grupos multicast y puertos, no hace falta volver a suscribirse)
		}  else {									     // else nos suscribimos
		    if ( (s_reader[socket_count] = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {        
			log_printf(LOG_ERROR, "socket reader (%d) %s\n", socket_count, strerror(errno));
			exit(EXIT_FAILURE);
		    }
		    log_printf(LOG_VERBOSE, "%d]*desc(%s) dest(%s:%s) src(%s) ifaz(%s) socket(%d)\n", i,
			radar_definition[i*5], radar_definition[(i*5)+1],
			radar_definition[(i*5)+2], radar_definition[(i*5)+3],
			radar_definition[(i*5)+4], s_reader[socket_count]);

		    //strncpy(radar_destination[s_reader[socket_count]].dest_ip, radar_definition[(i*5)+1], 255);
		    radar_destination[i].socket = s_reader[socket_count];
		    strncpy(radar_destination[i].dest_ip, radar_definition[(i*5)+1], 255);

		    //log_printf(LOG_VERBOSE, "0>%s<\n", radar_destination[i].dest_ip);
		    //log_printf(LOG_VERBOSE, "1>%s<\n", radar_definition[(i*5)+1]);

		    if (!strncasecmp(source, "mult", 4)) {
			unsigned char ttl = 32;
			if ( setsockopt(s_reader[socket_count], SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
			    log_printf(LOG_ERROR, "reuseaddr setsockopt reader %s\n", strerror(errno));
			    exit(EXIT_FAILURE);
			}
			if ( setsockopt(s_reader[socket_count], IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
			    log_printf(LOG_ERROR, "ip_multicast_ttl setsockopt reader %s\n", strerror(errno));
			    exit(EXIT_FAILURE);
			}

			memset(&cast_group, 0, sizeof(cast_group));
			cast_group.sin_family = AF_INET;
			// segun documentacion, si es SOLARIS cast_group.sin_addr.s_addr = htonl(INADDR_ANY);
			// resto cast_group.sin_addr.s_addr = inet_addr(radar_definition[i*5+1]);
			//cast_group.sin_addr.s_addr = inet_addr(radar_definition[i*5 + 1]); //multicast group ip
			cast_group.sin_addr.s_addr = inet_addr(radar_definition[i*5+1]); //htonl(INADDR_ANY);
			cast_group.sin_port = htons((unsigned short int)strtol(radar_definition[i*5 + 2], NULL, 0)); //multicast group port
			if ( bind(s_reader[socket_count], (struct sockaddr *) &cast_group, sizeof(cast_group)) < 0) {
			    log_printf(LOG_ERROR, "bind reader %s\n", strerror(errno));
			    exit(EXIT_FAILURE);
			}

			mreq.imr_interface.s_addr = inet_addr(radar_definition[i*5 + 4]); //htonl(INADDR_ANY); 
			mreq.imr_multiaddr.s_addr = inet_addr(radar_definition[i*5 + 1]); //multicast group ip
			if (setsockopt(s_reader[socket_count], IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
			    log_printf(LOG_ERROR, "add_membership setsockopt reader: %s\n", strerror(errno));
			    exit(EXIT_FAILURE);
			}
		    } else if (!strncasecmp(source, "broa", 4)) {
			if ( setsockopt(s_reader[socket_count], SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
			    log_printf(LOG_ERROR, "reuseaddr setsockopt reader: %s\n", strerror(errno));
			    exit(EXIT_FAILURE);
			}
			cast_group.sin_family = AF_INET;
			cast_group.sin_addr.s_addr = htonl(INADDR_ANY);
			cast_group.sin_port = htons((unsigned short int)strtol(radar_definition[i*5 + 2], NULL, 0)); //broadcast group port
			memset(&cast_group.sin_zero, 0, sizeof(cast_group.sin_zero));
			if ( bind(s_reader[socket_count], (struct sockaddr *) &cast_group, sizeof(cast_group)) < 0) {
			    // error happens when suscribing broadcast addresses
			    // suscribing 214.25.250.255 for 214.25.250.10 &
			    //            214.25.250.255 for 214.25.250.11
			    // returns an "Address already in use", so comment this code.
			    // problem is that SO_BROADCAST ignores SO_REUSEADDR
			    //log_printf(LOG_ERROR, "bind reader %s\n", strerror(errno));
			    //exit(EXIT_FAILURE);
			}
		    }
		    // log_printf(LOG_VERBOSE, "[%d] %d/%d/%d %s:%s %s\n", s_reader[socket_count], socket_count, i, radar_count/5, 
		    // radar_definition[i*5 + 1], radar_definition[i*5 + 2], radar_definition[i*5+3] );
		    socket_count++;
		}
		i++;
	    }

    return;
}

void setup_time(void) {
struct timeval t;
struct tm *t2;

    if (gettimeofday(&t, NULL)==-1) {
	log_printf(LOG_ERROR, "ERROR gettimeofday (setup_time): %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    if ((t2 = gmtime(&t.tv_sec)) == NULL) {
	log_printf(LOG_ERROR, "ERROR gmtime (setup_time): %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    t2->tm_sec = 0; t2->tm_min = 0; t2->tm_hour = 0;
    if ((midnight_t = mktime(t2))==-1) { //segundos a las 00:00:00 de hoy
	log_printf(LOG_ERROR, "ERROR mktime (setup_time): %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    return;
}

ssize_t setup_input_file(void) {
ssize_t size;

    if ( (fd_in = open(source_file, O_RDONLY)) == -1) {
        log_printf(LOG_ERROR, "ERROR open(%s): %s\n", source_file, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if ( (size = lseek(fd_in, 0, SEEK_END)) == -1) {
        log_printf(LOG_ERROR, "ERROR lseek (seek_end): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if ( lseek(fd_in, 0, SEEK_SET) == -1) {
        log_printf(LOG_ERROR, "ERROR lseek (seek_set): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return size;
}

void AddQueue(void* a) {
    // printf("add addr:%x crc:%x\n", (unsigned int)a, ((rb_red_blk_node*)a)->crc32);
    if (q.count != SCRM_MAX_QUEUE_SIZE) {
	if (q.count > 0)
	    q.rear = (q.rear + 1) % SCRM_MAX_QUEUE_SIZE;
	q.node[q.rear] = (rb_red_blk_node *) a;
	q.count++;
    }
    return;
}

void DeleteQueue(void *a) {
    // log_printf(LOG_ERROR, "1>deleting addr:%p crc:%08x\n", a, ((rb_red_blk_node*)a)->crc32);
    // item = queue.node[queue.front];
    if (q.count > 0) {
	if (q.count > 1) {
	    // log_printf(LOG_ERROR, "2>deleting addr:%p crc:%08x\n", q.node[q.front], q.node[q.front]->crc32);
	    q.node[q.front] = NULL;
	    q.front = (q.front + 1) % SCRM_MAX_QUEUE_SIZE;
	}
	q.count--;
    }
}

int UIntComp(unsigned int a, unsigned int b) {
    if (a>b) return (1);
    if (a<b) return (-1);
    return 0;
}

void free_config(void) {
    int i;

    if (dest_file_final_ast != NULL) mem_free(dest_file_final_ast);
    if (dest_file_final_gps != NULL) mem_free(dest_file_final_gps);
    if (source != NULL) mem_free(source);
    if (dest_file_region != NULL) mem_free(dest_file_region);
    if (dest_file_extension != NULL) mem_free(dest_file_extension);
    if (source_file != NULL) mem_free(source_file);
    if (dest_file != NULL) mem_free(dest_file);
    for(i=0; i<radar_count; i++)
	mem_free(radar_definition[i]);
    if (radar_definition != NULL) mem_free(radar_definition);
    for(i=0; i<dest_ftp_count; i++)
	mem_free(dest_ftp_uri[i]);
    if (dest_ftp_uri != NULL) mem_free(dest_ftp_uri);
    if (digest_hex != NULL) mem_free(digest_hex);
    for(i=0; i<dest_filter_count; i++)
        mem_free(dest_filter_selection[i]);
    if (dest_filter_selection != NULL) mem_free(dest_filter_selection);
    for(i=0; i<asterix_versions_count; i++)
        mem_free(asterix_versions[i]);
    if (asterix_versions != NULL) mem_free(asterix_versions);
    return;
}


int main(int argc, char *argv[]) {

ssize_t ast_size_total;
ssize_t ast_pos = 0;
ssize_t ast_size_tmp;
int ast_size_datablock;
unsigned char *ast_ptr_raw;
struct timeval t;
struct timeval timed_t_start; // tiempo inicial para las grabaciones temporizadas
struct timeval timed_t_current; // tiempo actual de la recepcion del paquete
struct timeval timed_t_Xsecs; // tiempo inicial de la vuelta actual (para el display de stats)
unsigned long count2_plot_ignored = 0;
unsigned long count2_plot_processed = 0;
unsigned long count2_plot_unique = 0;
unsigned long count2_plot_duped = 0;
unsigned long count2_udp_received = 0;
unsigned long count2_plot_filtered = 0;

    printf("reader_network_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
    startup();
    memset(full_tod, 0x00, MAX_RADAR_NUMBER*TTOD_WIDTH);
    if ( argc!=2 ) {
	log_printf(LOG_ERROR, "usage: %s <config_file>\n\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    setup_asterix_versions();
    if ( !strncmp(argv[1], "-r", 2) ) {
	if (digest_hex)
	    log_printf(LOG_ERROR, "%s\n", digest_hex);
	exit(EXIT_SUCCESS);
    }
    parse_config(argv[1]);
    log_printf(LOG_NORMAL, "init...\n");
    setup_time();
    if (dest_localhost)
	setup_output_multicast();
    setup_output_file();
    setup_crc32_table();
    if (mode_scrm) {
	tree = RBTreeCreate(UIntComp,AddQueue,DeleteQueue);
	q.node = (rb_red_blk_node **) mem_alloc(sizeof(rb_red_blk_node *) * SCRM_MAX_QUEUE_SIZE);
	q.rear = q.front = q.count = 0;
	// q.node =  mem_alloc(rb_red_blk_node);
    }

    gettimeofday(&timed_t_start, NULL);

    if (!strncasecmp(source, "file", 4)) {
/*
        if ( (fd_out_ast != -1) || (fd_out_gps == -1) ) {
	    log_printf(LOG_ERROR, "ERROR reader_network: output should be GPS when reading file\n");
	    exit(EXIT_FAILURE);
        }
*/
        //if ( (!source_file_gps && (fd_out_ast == -1)) ||
        if ( !source_file_gps && (fd_out_gps != -1) ) {
            log_printf(LOG_ERROR, "ERROR input & output mismatch source_file_gps(%d) fd_out_ast(%d) fd_out_gps(%d)\n",
                source_file_gps, fd_out_ast, fd_out_gps);
            log_printf(LOG_ERROR, "it is only possible to write AST->AST, GPS->GPS, GPS->AST\n");
            exit(EXIT_FAILURE);
        }

	ast_size_total = setup_input_file();
	ast_ptr_raw = (unsigned char *) mem_alloc(ast_size_total);
	if ( (ast_size_tmp = read(fd_in, ast_ptr_raw, ast_size_total)) != ast_size_total) {
	    log_printf(LOG_ERROR, "ERROR read: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	log_printf(LOG_NORMAL, "readed %ld bytes\n", (unsigned long) ast_size_total);
        if (source_file_gps_version == 0) {
            if ( !source_file_gps ) { // acaba en ast, es un archivo asterix
                offset = 0; ast_pos = 0;
            } else {
	        unsigned char *memcmp1;
	        memcmp1 = (unsigned char *) mem_alloc(20);
	        memcmp1 = memset(memcmp1, 0xCD, 20);
	        if (!memcmp(memcmp1, ast_ptr_raw+20, 20)) {
	            offset = 10; ast_pos += 2200; source_file_gps_version = 1;
	            log_printf(LOG_VERBOSE, "GPSv1 input auto-activated\n");
	        } else {
	            offset = 4; ast_pos = 0; source_file_gps_version = 2;
	            log_printf(LOG_VERBOSE, "GPSv2 input auto-activated\n");
	        }
	        mem_free(memcmp1);
	    }
        } else {
            if (source_file_gps_version == 1) {
	        ast_pos += 2200; offset = 10;
            }
	}
/*
        if (source_file_gps_version != 1) {
            mem_free(ast_ptr_raw);
	    log_printf(LOG_ERROR, "ERROR reader_network: input should be GPSv1 when reading file\n");
	    exit(EXIT_FAILURE);
        }
*/
	while (ast_pos < ast_size_total) {
            bool volcar = true;
	    gettimeofday(&t, NULL);
	    ast_size_datablock = (ast_ptr_raw[ast_pos + 1]<<8) + ast_ptr_raw[ast_pos + 2];
	    count2_udp_received++;

	    if (source_file_gps) {
		if (source_file_gps_version == 1) {
		    current_time_today = ((ast_ptr_raw[ast_pos + ast_size_datablock + 6]<<16 ) +
			(ast_ptr_raw[ast_pos + ast_size_datablock + 7] << 8) +
			(ast_ptr_raw[ast_pos + ast_size_datablock + 8]) ) / 128.0;
		} else if (source_file_gps_version == 2) {
		    current_time_today = ((ast_ptr_raw[ast_pos + ast_size_datablock] ) +
			(ast_ptr_raw[ast_pos + ast_size_datablock + 1] << 8) +
			(ast_ptr_raw[ast_pos + ast_size_datablock + 2] << 16) +
			(ast_ptr_raw[ast_pos + ast_size_datablock + 3] << 24) ) / 1000.0;
		}
	    } else {
		current_time_today = (t.tv_sec - midnight_t) + t.tv_usec / 1000000.0;
	    }

            filter_struct fs;
            fs.size_datablock = 0;
            fs.ptr_raw = NULL;
            fs.filter_type = dest_filter_flags;

            if ( dest_localhost || dest_filter_flags ) {
                //log_printf(LOG_NORMAL, "\n==================================================================\n");
		//ast_output_datablock(ast_ptr_raw + ast_pos, ast_size_datablock, count2_plot_processed, 0);

		if (ast_ptr_raw[ast_pos] == '\x01') {
		    count2_plot_processed++;
		    ast_procesarCAT01(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		} else if (ast_ptr_raw[ast_pos] == '\x02') {
		    count2_plot_processed++;
		    ast_procesarCAT02(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		} else if (ast_ptr_raw[ast_pos] == '\x08') {
		    count2_plot_processed++;
		    ast_procesarCAT08(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		} else if (ast_ptr_raw[ast_pos] == '\x0a') {
		    count2_plot_processed++;
		    ast_procesarCAT10(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		} else if (ast_ptr_raw[ast_pos] == '\x13') {
		    count2_plot_processed++;
		    ast_procesarCAT19(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		} else if (ast_ptr_raw[ast_pos] == '\x14') {
		    count2_plot_processed++;
		    ast_procesarCAT20(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		} else if (ast_ptr_raw[ast_pos] == '\x15') {
		    // count2_plot_processed++;
		    // ast_procesarCAT21(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		    count2_plot_ignored++;
		} else if (ast_ptr_raw[ast_pos] == '\x22') {
		    count2_plot_processed++;
		    ast_procesarCAT34(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		} else if (ast_ptr_raw[ast_pos] == '\x30') {
                    count2_plot_processed++;
		    ast_procesarCAT48F(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost, &fs);
                    if (fs.filter_type != FILTER_NONE && fs.size_datablock == 0) {
                        volcar = false;
                        count2_plot_filtered++;
                    } else if ( fs.size_datablock == ast_size_datablock ) {
                        mem_free(fs.ptr_raw);
                        fs.size_datablock = 0;
                    }
		} else if (ast_ptr_raw[ast_pos] == '\x3e') {
		    count2_plot_processed++;
		    ast_procesarCAT62(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost);
		} else {
		    count2_plot_ignored++;
		}
	    }
            if (volcar) {
                  if (fs.size_datablock > 0) { // en el caso de que ast_procesarCATXXF soportase filtros y se hubiera activado,
                                               // hay que escribir en disco el datablock una vez filtrado
                    count2_plot_filtered++;
                    //ast_output_datablock(fs.ptr_raw, fs.size_datablock, count2_plot_processed, 0);
                    if ( fd_out_gps != -1 ) {
                        if ( write(fd_out_gps, fs.ptr_raw, fs.size_datablock) == -1 ) {
                            log_printf(LOG_ERROR, "ERROR writev1->fd_out_gps (fs): %s\n", strerror(errno));
                        }
                        // si el origen es gps, hay que escribir también el offset.
                        if ( offset != 0 ) {
                            if ( write(fd_out_gps, ast_ptr_raw + ast_pos + ast_size_datablock, offset) == -1 ) {
                                log_printf(LOG_ERROR, "ERROR writev1->fd_out_gps (gps): %s\n", strerror(errno));
                            }
                        }
                        // nunca se puede dar la condición de que el original sea gps y el destino asterix.
                    }
                    if ( fd_out_ast != -1 ) {
                        // no hay offset que escribir, el origen es asterix, el destino debe ser asterix
                        if ( write(fd_out_ast, fs.ptr_raw, fs.size_datablock) == -1 ) {
                            log_printf(LOG_ERROR, "ERROR write (fs): %s\n", strerror(errno));
                        }
                    }
                    mem_free(fs.ptr_raw);
                } else {
                    //ast_output_datablock(ast_ptr_raw + ast_pos, ast_size_datablock + offset, count2_plot_processed, 0);
                    if ( fd_out_gps != -1 ) {
                        if ( write(fd_out_gps, ast_ptr_raw + ast_pos, ast_size_datablock + offset) == -1 ) {
                            log_printf(LOG_ERROR, "ERROR writev2->fd_out_gps (ast_ptr_raw): %s\n", strerror(errno));
                        }
                    }
                    if ( fd_out_ast != -1 ) {
                        if ( write(fd_out_ast, ast_ptr_raw + ast_pos, ast_size_datablock) == -1 ) {
                            log_printf(LOG_ERROR, "ERROR writev2->fd_out_ast (ast_ptr_raw): %s\n", strerror(errno));
                        }
                    }
                }
            }

            ast_pos += ast_size_datablock + offset;

/*
	    if (fd_out_ast != -1) { // ultima modificacion! permitir salida ast/gps en input de fichero.
		if (dest_file_gps) {
		    if (source_file_gps && (source_file_gps_version == 2) ) {
			if ( (write (fd_out_gps, ast_ptr_raw + ast_pos, ast_size_datablock + offset)) == -1 ) {
			    log_printf(LOG_ERROR, "ERROR: %s\n", strerror(errno));
			}
		    } else {
			unsigned long sec;
			unsigned char byte;
			unsigned char *datablock_tmp;
			datablock_tmp = (unsigned char *) mem_alloc(ast_size_datablock+4);
			memcpy(datablock_tmp, ast_ptr_raw + ast_pos, ast_size_datablock);
			sec = (t.tv_sec - t3) * 1000 + (t.tv_usec/1000);
			byte = sec & 0xFF;
			memcpy(datablock_tmp + ast_size_datablock, &byte, 1);
			byte = (sec >> 8) & 0xFF;
			memcpy(datablock_tmp + ast_size_datablock + 1, &byte, 1);
			byte = (sec >> 16) & 0xFF;
			memcpy(datablock_tmp + ast_size_datablock + 2, &byte, 1);
			byte = (sec >> 24) & 0xFF;
			memcpy(datablock_tmp + ast_size_datablock + 3, &byte, 1); //para fechar con hora gpsv2
										  //SALIDA NO VALIDADA!!
			if ( (write(fd_out, datablock_tmp, ast_size_datablock + 4) ) == -1) {
		    	    log_printf(LOG_ERROR, "ERROR: %s\n", strerror(errno));
			}
			mem_free(datablock_tmp);
		    }
		} else {
//IFSNOP
		    int volcar=1;

		    if ((ast_size_datablock == 9) && (ast_ptr_raw[0+ast_pos] == '\x02')) {
			volcar=0;
		    }

		    if (volcar==1) {
			if (ast_ptr_raw[0+ast_pos] == '\x01') {
			    char *p;
			    do {
//				log_printf(LOG_VERBOSE, "1 vamos a ver [%x] [%x]\n", ast_ptr_raw+ast_pos, p);
				p = (char *) memmem(ast_ptr_raw+ast_pos, ast_size_datablock, "\x0FD\x44\x14\x0C9\x0A0",5);
//				log_printf(LOG_VERBOSE, "3 vamos a ver [%x] [%x]\n", ast_ptr_raw+ast_pos, p);				
				if (p!=NULL) {
//					log_printf(LOG_VERBOSE, ">>>><<<<\n");
//					ast_output_datablock(ast_ptr_raw+ast_pos,ast_size_datablock+offset,0,0);

					*(p + 4) = '\x0A2';
					*(p + 3) = '\x0CA';

//					ast_output_datablock(ast_ptr_raw+ast_pos,ast_size_datablock+offset,0,0);
//					log_printf(LOG_VERBOSE, "vamos a ver [%x] [%x]\n", ast_ptr_raw+ast_pos, p);

//					log_printf(LOG_VERBOSE, ">>>><<<<\n\n");
				}
			    } while (p!=NULL);
			}

			if ( (write(fd_out, ast_ptr_raw + ast_pos, ast_size_datablock) ) == -1) {
			    log_printf(LOG_ERROR, "ERROR: %s\n", strerror(errno));
			}
		    } else {
			count_plot_ignored++;
		    }
		}
	    }
*/
/*
	    if (dest_localhost) {
//		ast_output_datablock(ast_ptr_raw + ast_pos, ast_size_datablock + offset, 0, 0);
		if (ast_ptr_raw[ast_pos] == '\x01') {
		    count2_plot_processed++;
		    ast_procesarCAT01(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else if (ast_ptr_raw[ast_pos] == '\x02') {
		    count2_plot_processed++;
		    ast_procesarCAT02(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else if (ast_ptr_raw[ast_pos] == '\x08') {
		    count2_plot_processed++;
		    ast_procesarCAT08(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else if (ast_ptr_raw[ast_pos] == '\x0a') {
		    count2_plot_processed++;
		    ast_procesarCAT10(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else if (ast_ptr_raw[ast_pos] == '\x13') {
		    count2_plot_processed++;
		    ast_procesarCAT19(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else if (ast_ptr_raw[ast_pos] == '\x14') {
		    count2_plot_processed++;
		    ast_procesarCAT20(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else if (ast_ptr_raw[ast_pos] == '\x15') {
		    count2_plot_processed++;
		    ast_procesarCAT21(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else if (ast_ptr_raw[ast_pos] == '\x22') {
		    count2_plot_processed++;
		    ast_procesarCAT34(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else if (ast_ptr_raw[ast_pos] == '\x30') {
		    count2_plot_processed++;
		    ast_procesarCAT48(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true, dest_filter_flags);
		} else if (ast_ptr_raw[ast_pos] == '\x3e') {
		    count2_plot_processed++;
		    ast_procesarCAT62(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, true);
		} else {
		    count2_plot_ignored++;
		}
	    }
	    ast_pos += ast_size_datablock + offset;
//	    usleep(100);
*/
	}
	mem_free(ast_ptr_raw);

    } else if (!strncasecmp(source, "mult", 4) || !strncasecmp(source, "broa", 4)) {
	int i=0,j=0;
	setup_priority(-20);

	s_reader = mem_alloc((radar_count/5)*sizeof(int));
	radar_counter = mem_alloc((radar_count/5)*sizeof(int));
	radar_counter_bytes = mem_alloc((radar_count/5)*sizeof(int));

	for(i=0; i<(radar_count/5); i++) {
	    s_reader[i] = -1;
	    radar_counter[i] = 0; // plots recibidos por flujo
	    radar_counter_bytes[i] = 0; // bytes recibidos por flujo
	}

	setup_input_network();

	ast_ptr_raw = (unsigned char *) mem_alloc(MAX_PACKET_LENGTH);
	gettimeofday(&timed_t_current, NULL);
	timed_t_Xsecs.tv_sec = timed_t_current.tv_sec;
	timed_t_Xsecs.tv_usec = timed_t_current.tv_usec;

	while ( timed==0 || (timed_t_current.tv_sec <= (timed_t_start.tv_sec + timed))) {
	    struct timeval timeout;
	    int select_count;
	    socklen_t addrlen = sizeof(struct sockaddr_in);
	    double current_timestamp = 0;
	    struct sockaddr_in cast_group;
	    fd_set reader_set;

	    gettimeofday(&timed_t_current, NULL);
	    memset(ast_ptr_raw, 0x00, MAX_PACKET_LENGTH);
	    timeout.tv_sec = SELECT_TIMEOUT; timeout.tv_usec = 0;

	    FD_ZERO(&reader_set);
	    for (i=0; i<socket_count; i++) {
		FD_SET(s_reader[i], &reader_set);
	    }

	    select_count = select(s_reader[socket_count - 1] + 1, &reader_set, NULL, NULL, &timeout);

	    gettimeofday(&t, NULL); // for gps & queue timestamp
	    current_time_today = ((t.tv_sec - midnight_t) % 86400) + t.tv_usec / 1000000.0; // segundos desde las 00:00:00
	    current_timestamp = (t.tv_sec) + (t.tv_usec / 1000000.0); // segundos desde 01-01-1970 a las 00:00:00

	    // PURGE OLD ELEMENTS FROM DUPE QUEUE
	    if (mode_scrm) {
		//log_printf(LOG_ERROR, "=================\ncount: %d, current_timestamp: %3.3f\n", q.count, current_timestamp);
		// RBTreePrint(tree);
		//if (q.count>0) {
		//    log_printf(LOG_ERROR, "q.rear(%08X)\n", q.rear);
		//    log_printf(LOG_ERROR, "q.front(%08X)\n", q.front);
		//    log_printf(LOG_ERROR, "q.node[q.rear]->timestamp(%3.3f)\n", q.node[q.rear]->timestamp);
		//    log_printf(LOG_ERROR, "q.node[q.front]->timestamp(%3.3f)\n", q.node[q.front]->timestamp);
		//    log_printf(LOG_ERROR, "%3.4f < %3.4f = %3.4f\n", q.node[q.front]->timestamp,
		//        (current_timestamp - SCRM_TIMEOUT),
		//        q.node[q.front]->timestamp - (current_timestamp - SCRM_TIMEOUT));
		//}
		while ( (q.count > 0) &&
		    q.node[q.front]->timestamp < (current_timestamp - SCRM_TIMEOUT) ) {
		    // borrar todos los nodos con fecha > SCRM_OLD_TIME
		    rb_red_blk_node *node_old = q.node[q.front];
		    //log_printf(LOG_ERROR, "borrando ahora (%3.3f) nodo que tiene el timestamp (%3.3f) de hace mas de (%d) segundos\n", 
		    //    current_timestamp, node_old->timestamp, SCRM_TIMEOUT);
		    RBDelete(tree, node_old);
		    //log_printf(LOG_ERROR, "despues de borrar, nos quedan (%d) elementos\n", q.count);
		}
	    }

	    if ( select_count > 0 ) {
		i=0;
		while (i<socket_count) {
		    if (FD_ISSET(s_reader[i], &reader_set)) {
			int udp_size=0; bool record = true, is_processed = false;
			memset(&cast_group, 0, sizeof(cast_group));
			addrlen = sizeof(struct sockaddr);
			if ((udp_size = recvfrom(s_reader[i], ast_ptr_raw, MAX_PACKET_LENGTH, 0, (struct sockaddr *) &cast_group, &addrlen)) < 0) {
			    log_printf(LOG_ERROR, "ERROR recvfrom: %s\n", strerror(errno));
			    exit(EXIT_FAILURE);
			}
			if (udp_size > 0)
			    count2_udp_received++;

			for(j=0;(j<radar_count/5); j++) { // se comprueba con la ip de origen
//			    log_printf(LOG_VERBOSE, "%d %d\n",
//			        (s_reader[i]!=radar_destination[j].socket),
//			        (strcasecmp(inet_ntoa(cast_group.sin_addr), radar_definition[j*5+3])!=0));
			    if ( (s_reader[i]==radar_destination[j].socket) &&
			         (!strcasecmp(inet_ntoa(cast_group.sin_addr), radar_definition[j*5+3])) )
			        break;
			}
			
			if (j==(radar_count/5))
			    break; // no ha aparecido ningun blanco que pertenezca a un radar definido en la configuracion

//			while (j<(radar_count/5)) {

//    cast_group.sin_port = htons((unsigned short int)strtol(radar_definition[i*5 + 2], NULL, 0)); //broadcast group port

//    !strcasecmp(radar_definition[(i*5)+1], radar_definition[((i-1)*5)+1]) && // mismo grupo mcast
//    !strcasecmp(radar_definition[(i*5)+2], radar_definition[((i-1)*5)+2])) { // y mismo puerto
//			log_printf(LOG_VERBOSE, "s(%d) %s->%s:%d\n", s_reader[i], 
//			    radar_destination[s_reader[i]].dest_ip, 
//			    inet_ntoa(cast_group.sin_addr), cast_group.sin_port);
			    //if (!strcasecmp(inet_ntoa(cast_group.sin_addr), radar_definition[j*5+3])) { // filtrando por ip origen
			    {
				unsigned char *ast_ptr_raw_tmp = ast_ptr_raw;
				int salir = 0;
				
//				for(k = j; k<socket_count; k++) {
//				    log_printf(LOG_VERBOSE, ">%d\n", k);
//				    if (!strcasecmp(inet_ntoa(cast_group.sin_addr), radar_definition[k*5+3])) {
//					log_printf(LOG_VERBOSE, ">> %s %d\n", radar_definition[k*5+3], s_reader[k]);
//				    }
//				}

//			        log_printf(LOG_VERBOSE, "*%02d) rcv(%s) cfg(%s) counter(%ld)\n",j, inet_ntoa(cast_group.sin_addr), radar_definition[j*5+3], count2_udp_received);

				if (timed_stats_interval) {
				    radar_counter[j]++;
//				    log_printf(LOG_NORMAL, "adding(%d) to radar %d\n\n", udp_size,j);
				    radar_counter_bytes[j] += udp_size;
				}
				if ( dest_localhost ||
				    ((dest_file_format & DEST_FILE_FORMAT_GPS) == DEST_FILE_FORMAT_GPS) ) {
				    // solo si se descodifica tenemos que tomar precauciones con el ast
				    // cat 10 del smr de barajas y con scr mal configurados
				    ast_size_datablock = (ast_ptr_raw[1]<<8) + ast_ptr_raw[2];
				} else {
				    ast_size_datablock = udp_size;
				}
				is_processed = true;

				//log_printf(LOG_VERBOSE,"-----udp(%d) ast_size_datablock(%d)\n", udp_size, ast_size_datablock);
				//ast_output_datablock(ast_ptr_raw, udp_size, 0, 0);
				
				do {
				    unsigned int crc = 0;
				    count2_plot_processed++; // smr barajas mete varios plots que hay que desmontar para fechar
				    record = true;

				    if (mode_scrm || dest_screen_crc) {
					crc = crc32(ast_ptr_raw_tmp, ast_size_datablock);
					if (dest_screen_crc) {
					    ast_output_datablock(ast_ptr_raw_tmp, ast_size_datablock, count2_plot_processed, 0);
					    log_printf(LOG_VERBOSE, "%3.4f [%08X]\n", current_time_today, crc);
					}
				    }

				    if (mode_scrm) {
					rb_red_blk_node *node = RBExactQuery(tree,crc);
					if (!node) { // no existe en arbol
					    count2_plot_unique++;
					    if (tree->count>=SCRM_MAX_QUEUE_SIZE) { // si arbol lleno, borrar nodo mas antiguo
						rb_red_blk_node *node_old = q.node[q.front];
//						if (node_old->access==0) {
//						    log_printf(LOG_ERROR,"borrando sin dupe crc32[%08x] count[%d]\n", node_old->crc32, node_old->access);
//						}
						RBDelete(tree, node_old);
					    }
					    RBTreeInsert(tree, crc, current_timestamp);
					} else {
					    node->access++;
					    record = false; // so dupes are ignored
					    count2_plot_duped++;
					}
					// RBTreePrint(tree);
				    }

				    //ast_output_datablock(ast_ptr_raw_tmp, ast_size_datablock, count2_plot_processed, 0);
				    if (dest_localhost && record) {
					if (ast_ptr_raw_tmp[0] == '\x01')
					    ast_procesarCAT01(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
					if (ast_ptr_raw_tmp[0] == '\x02')
					    ast_procesarCAT02(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
					if (ast_ptr_raw_tmp[0] == '\x08')
					    ast_procesarCAT08(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
                                        // excepción, el antiguo SMR de Terma concatenaba cat-size-datarecord+datarecord+datarecord
                                        // el resto del mundo hace cat-size-datarecords+cat-size-datarecord....
					if (ast_ptr_raw[0] == '\x0a')
					    ast_procesarCAT10(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
					if (ast_ptr_raw_tmp[0] == '\x013')
					    ast_procesarCAT19(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
					if (ast_ptr_raw_tmp[0] == '\x014')
					    ast_procesarCAT20(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
					if (ast_ptr_raw_tmp[0] == '\x15')
					    ast_procesarCAT21(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
					if (ast_ptr_raw_tmp[0] == '\x22')
					    ast_procesarCAT34(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
					if (ast_ptr_raw_tmp[0] == '\x30')
					    //ast_procesarCAT48(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true); //, FILTER_GROUND);
                                            //ast_procesarCAT48F(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost, dest_filter_flags, ast_ptr_raw_new, ast_size_datablock_new);
                                            ast_procesarCAT48F(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count2_plot_processed, dest_localhost, NULL);
					if (ast_ptr_raw_tmp[0] == '\x3e')
					    ast_procesarCAT62(ast_ptr_raw_tmp + 3, ast_size_datablock, count2_plot_processed, true);
				    }
				    if (dest_file != NULL && record) {
					if ((dest_file_format & DEST_FILE_FORMAT_AST) == DEST_FILE_FORMAT_AST) {
					    if ( (write(fd_out_ast, ast_ptr_raw_tmp, ast_size_datablock) ) == -1) {
						log_printf(LOG_ERROR, "ERROR write_ast: %s (%d)\n", strerror(errno), fd_out_ast);
					    }
					}
					if ((dest_file_format & DEST_FILE_FORMAT_GPS) == DEST_FILE_FORMAT_GPS) {
					    unsigned long timegps;
					    unsigned char byte;
					    unsigned char output_ptr[MAX_PACKET_LENGTH];
					    memcpy(output_ptr, ast_ptr_raw_tmp, ast_size_datablock);
                                            memset(output_ptr + ast_size_datablock, 0, 10);

					    byte = (cast_group.sin_addr.s_addr >> 24) & 0xFF;
					    memcpy(output_ptr + ast_size_datablock + 0, &byte, 1);
					    byte = (cast_group.sin_addr.s_addr >> 16) & 0xFF;
					    memcpy(output_ptr + ast_size_datablock + 1, &byte, 1);
					    byte = (cast_group.sin_addr.s_addr >> 8) & 0xFF;
					    memcpy(output_ptr + ast_size_datablock + 2, &byte, 1);
					    byte = (cast_group.sin_addr.s_addr) & 0xFF;
					    memcpy(output_ptr + ast_size_datablock + 3, &byte, 1);

                                            // 4, 5 & 9 are still empty

					    timegps = current_time_today * 128.0;
					    byte = (timegps >> 16) & 0xFF;
					    memcpy(output_ptr + ast_size_datablock + 6, &byte, 1);
					    byte = (timegps >> 8) & 0xFF;
					    memcpy(output_ptr + ast_size_datablock + 7, &byte, 1);
					    byte = (timegps) & 0xFF;
					    memcpy(output_ptr + ast_size_datablock + 8, &byte, 1);
					    if ( (write(fd_out_gps, output_ptr, ast_size_datablock+10) ) != (ast_size_datablock+10)) {
						log_printf(LOG_ERROR, "ERROR write_gps: %s (fd:%d)\n", strerror(errno), fd_out_gps);
					    }
					}
				    }
				    ast_ptr_raw_tmp += ast_size_datablock;
				    // realmente, si ast_size_datablock < udp_size
				    if (ast_ptr_raw_tmp < (ast_ptr_raw+udp_size)) {
					ast_size_datablock = (ast_ptr_raw_tmp[1]<<8) + ast_ptr_raw_tmp[2];
				    } else {
					salir=1;
				    }
				} while (salir==0);
				//log_printf(LOG_NORMAL, "\n==================================================================\n");
//                                j=radar_count/5; // no seguir buscando, ya ha sido procesado
//			    } else {
//			        log_printf(LOG_VERBOSE, "%02d) rcv(%s) cfg(%s) counter(%ld)\n",j, inet_ntoa(cast_group.sin_addr), radar_definition[j*5+3], count2_udp_received);
			    }
//			    j++;
//			}
			if (!is_processed) count2_plot_ignored++;
		    }
		    i++;
		}
	    } else if (select_count == 0) {
		log_printf(LOG_VERBOSE, "%d sec(s) warning\n", SELECT_TIMEOUT);
		if (dest_localhost)
		    setup_output_multicast();
		for(i=0; i<socket_count; i++) {
		    close(s_reader[i]);
		}
		socket_count = 0; setup_input_network();
	    } else {
		log_printf(LOG_ERROR, "socket error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	    }

	    // PRINT STATS
	    if ( (timed_stats_interval > 0) && (
		(t.tv_sec+t.tv_usec/1000000.0) > (timed_t_Xsecs.tv_sec + timed_t_Xsecs.tv_usec/1000000.0 + timed_stats_interval)
		) ) {
		float secs = t.tv_sec + t.tv_usec/1000000.0 - // tiempo del ultimo dato recibido
		  (timed_t_Xsecs.tv_sec + timed_t_Xsecs.tv_usec / 1000000.0); // tiempo del ultimo stat volcado 
		float total_bw = 0.0; int total_bytes = 0, total_packets = 0;

		log_printf(LOG_VERBOSE," # name\t\t\tcount\tbw(b/s)\tbytes\tsecs(%3.2f)\n", secs);

		for(i=0;i<radar_count/5;i++) {
		    log_printf(LOG_VERBOSE,"%02i]%s\t\t%d\t%03.1f\t%d          \n", i, radar_definition[i*5],
			radar_counter[i], ((float)radar_counter_bytes[i]) / secs, radar_counter_bytes[i]);
		    total_bw += ((float)radar_counter_bytes[i])/secs;
		    total_bytes += radar_counter_bytes[i];
		    total_packets += radar_counter[i];
		    radar_counter[i] = 0;
		    radar_counter_bytes[i] = 0;
		}
		log_printf(LOG_VERBOSE, "XX]TOTAL\t\t%d\t%03.1f\t%d              \n", total_packets, total_bw, total_bytes);
		//for(i=0;i<(radar_count/5)+2;i++) { printf("\033[1A"); }
		timed_t_Xsecs.tv_sec = t.tv_sec;
		timed_t_Xsecs.tv_usec = t.tv_usec;
	     }
/*
	{ // delay statistics
	    struct timeval delay_t;
	    double delay;
	    gettimeofday(&delay_t, NULL); // for gps & queue timestamp
	    delay = (delay_t.tv_sec + (delay_t.tv_usec / 1000000.0)) - (t.tv_sec + (t.tv_usec / 1000000.0));
	    if (delay>0.001)
		log_printf(LOG_VERBOSE,"proc. delay: %3.5f\n", delay);
	}
*/	
	}
	mem_free(radar_counter_bytes);
	mem_free(radar_counter);
	mem_free(ast_ptr_raw);
	for(i=0; i<socket_count; i++)
	    close(s_reader[i]);
	mem_free(s_reader);
    }

    //{
	//int i;
	//if ( (timed_stats_interval > 0) && (count2_plot_processed>0) )
	//    for(i=0;i<(radar_count/5)+1;i++)
	//	printf("\n");
    //}

    log_printf(LOG_NORMAL, "stats received[%ld] processed[%ld]/ignored[%ld]/filtered[%ld] = unique[%ld]+duped[%ld]\n",
        count2_udp_received, count2_plot_processed, count2_plot_ignored,
        count2_plot_filtered, count2_plot_unique, count2_plot_duped);

    if (dest_localhost) { // if sending decoded asterix, tell clients that we are closing!
	struct datablock_plot dbp;
	memset(&dbp, 0, sizeof(struct datablock_plot));
	dbp.cat = CAT_255;
	if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
	    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
	}
    }

//    log_flush();
    if (mode_scrm) {
	mem_free(q.node);
	RBTreeDestroy(tree);
    }
    if (dest_localhost)
	close(s_output_multicast);
    if (!strncasecmp(source, "file", 4))
	close(fd_in);
    close_output_file();
    if (dest_ftp_count > 0)
	send_output_file();

    free_config();

    return 0;
}

