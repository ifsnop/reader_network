               :
               :#include "includes.h"
               :
               :float current_time=0.0;
               :struct sockaddr_in cliaddr,srvaddr;
               :
               :bool enabled = false;
               :bool dest_file_gps = false, source_file_gps = false;
               :time_t t3;
               :char *source, *dest_file = NULL, *source_file = NULL, **radar_definition;
               :int radar_count = 0, socket_count = 0, s, offset = 0;
               :int fd_out=-1, fd_in=-1;
               :long source_file_gps_version=0;
               :unsigned long count_plot_processed = 0;
               :unsigned long count_plot_ignored = 0;
               :	    
               :
               :void parse_config(void) {
               :
               :    if (!cfg_open("reader_network.conf")) {
               :        log_printf(LOG_ERROR, "please create reader_network.conf or check for duplicate entry\n");
               :        exit(EXIT_FAILURE);
               :    }
               :
               :    cfg_get_bool(&enabled, "enabled");
               :    if (enabled == false) {
               :        log_printf(LOG_ERROR, "please modify your config.file\n");
               :        exit(EXIT_FAILURE);
               :    }
               :
               :    if (!cfg_get_str(&source, "source")) {
               :        log_printf(LOG_VERBOSE, "source must be file, multicast or broadcast\n");
               :        exit(EXIT_FAILURE);
               :    }
               :
               :    if (!strncasecmp(source, "file", 4)) {
               :	if (!cfg_get_str(&source_file, "source_file")) {
               :    	    log_printf(LOG_ERROR, "source_file entry missing\n");
               :    	    exit(EXIT_FAILURE);
               :	} else {
               :    	    log_printf(LOG_VERBOSE, "input data from file: %s\n", source_file);
               :	    if (!strcasecmp(source_file + strlen(source_file) - 3, "gps")) {
               :		source_file_gps = true;
               :		if (!cfg_get_int(&source_file_gps_version, "source_file_gps_version")) {
               :    		    log_printf(LOG_ERROR, "source_file entry missing\n");
               :    	    	    exit(EXIT_FAILURE);
               :		} else {
               :		    if (source_file_gps_version == 1) {			
               :			log_printf(LOG_VERBOSE, "GPSv1 input activated\n");
               :			offset = 10;
               :		    } else if (source_file_gps_version == 2) {
               :		    	log_printf(LOG_VERBOSE, "GPSv2 input activated\n");
               :			offset = 4;
               :		    } else {
               :			log_printf(LOG_ERROR, "GPS version not supported\n");
               :    	    		exit(EXIT_FAILURE);
               :		    }
               :		}
               :	    } else {
               :		log_printf(LOG_VERBOSE, "AST input activated\n");
               :	    }
               :	}
               :    } else if (!strncasecmp(source, "mult", 4)) {
               :	if (!cfg_get_str_array(&radar_definition, &radar_count, "radar_definition")) {
               :    	    log_printf(LOG_ERROR, "radar_definition entry missing\n");
               :    	    exit(EXIT_FAILURE);
               :	}
               :        log_printf(LOG_VERBOSE, "reading from multicast\n");
               :    } else if (!strncasecmp(source, "broa", 4)) {
               :	if (!cfg_get_str_array(&radar_definition, &radar_count, "radar_definition")) {
               :    	    log_printf(LOG_ERROR, "radar_definition entry missing\n");
               :    	    exit(EXIT_FAILURE);
               :	}
               :        log_printf(LOG_VERBOSE, "reading from broadcast\n");
               :    }
               :
               :    if (cfg_get_str(&dest_file, "dest_file")) {
               :        log_printf(LOG_VERBOSE, "output data to file: %s\n", dest_file);
               :	if (!strcasecmp(dest_file + strlen(dest_file) - 3, "gps")) {
               :	    dest_file_gps = true;
               :	    log_printf(LOG_VERBOSE, "GPS output activated\n");
               :	} else {
               :	    log_printf(LOG_VERBOSE, "AST output activated\n");
               :	}
               :    }
               :
               :    return;
               :}
               :
               :void setup_output_file(void) {
               :
               :    if (dest_file != NULL) {
               :        if ( (fd_out = open(dest_file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR 
               :	    | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
               :    	    log_printf(LOG_ERROR, "ERROR open: %s\n", strerror(errno));
               :    	    exit(EXIT_FAILURE);
               :	}
               :    }
               :    return;
               :}
               :
               :void setup_output_multicast(void) {
               :struct hostent *h;
               :int loop=1;
               :
               :    if ( (h = gethostbyname(MULTICAST_PLOTS_GROUP)) == NULL ) {
               :        log_printf(LOG_ERROR, "ERROR gethostbyname");
               :        exit(EXIT_FAILURE);
               :    }
               :    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
               :    memset(&srvaddr, 0, sizeof(struct sockaddr_in));
               :    srvaddr.sin_family = h->h_addrtype;
               :    memcpy((char*) &srvaddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
               :    srvaddr.sin_port = htons(MULTICAST_PLOTS_PORT);
               :    if ( (s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
               :        log_printf(LOG_ERROR, "ERROR socket: %s\n", strerror(errno));
               :        exit(EXIT_FAILURE);
               :    }
               :    cliaddr.sin_family = AF_INET;
               :    cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //htonl(INADDR_ANY);
               :    cliaddr.sin_port = htons(0);
               :    if (  bind(s, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) <0 ) {
               :        log_printf(LOG_ERROR, "ERROR socket: %s\n", strerror(errno));
               :        exit(EXIT_FAILURE);
               :    }
               :    setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
               :
               :    return;
               :}
               :
               :void setup_time(void) {
               :struct timeval t;
               :struct tm *t2;
               :    
               :    gettimeofday(&t, NULL);
               :    t2 = gmtime(&t.tv_sec);
               :    t2->tm_sec = 0; t2->tm_min = 0; t2->tm_hour = 0;
               :    t3 = mktime(t2); //segundos a las 00:00:00 de hoy
               :    return;
               :}
               :
               :ssize_t setup_input_file(void) {
               :ssize_t size;
               :    if ( (fd_in = open(source_file, O_RDONLY)) == -1) {
               :        log_printf(LOG_ERROR, "ERROR open: %s\n", strerror(errno));
               :        exit(EXIT_FAILURE);
               :    }        
               :    if ( (size = lseek(fd_in, 0, SEEK_END)) == -1) {
               :        log_printf(LOG_ERROR, "ERROR lseek (seek_end): %s\n", strerror(errno));
               :        exit(EXIT_FAILURE);
               :    }
               :    if ( lseek(fd_in, 0, SEEK_SET) == -1) {
               :        log_printf(LOG_ERROR, "ERROR lseek (seek_set): %s\n", strerror(errno));
               :        exit(EXIT_FAILURE);
               :    }
               :    return size;
               :}
               :
               :int main(int argc, char *argv[]) {
               :
               :ssize_t ast_size_total;
               :ssize_t ast_pos;
               :ssize_t ast_size_tmp;
               :int ast_size_datablock;
               :unsigned char *ast_ptr_raw;
               :struct timeval t;
               :
               :    startup();
               :    bzero(full_tod, MAX_RADAR_NUMBER*TTOD_WIDTH);	
               :    parse_config();
               :    log_printf(LOG_NORMAL, "init...\n");
               :    setup_time();
               :    setup_output_multicast();
               :    setup_output_file();
               :
               :    if (!strncasecmp(source, "file", 4)) { 
               :	ast_size_total = setup_input_file();
               :	ast_ptr_raw = (unsigned char *) mem_alloc(ast_size_total);
               :	if ( (ast_size_tmp = read(fd_in, ast_ptr_raw, ast_size_total)) != ast_size_total) {
               :	    log_printf(LOG_ERROR, "ERROR read: %s\n", strerror(errno));
               :	    exit(EXIT_FAILURE);
               :	}    
               :	log_printf(LOG_NORMAL, "readed %d bytes\n", ast_size_total);
               :        ast_pos = 0;
               :	if (source_file_gps_version == 1) ast_pos += 2200;
               :
     2  0.0248 :	while (ast_pos < ast_size_total) {
    14  0.1733 :	    gettimeofday(&t, NULL);
    11  0.1362 :	    ast_size_datablock = ast_ptr_raw[ast_pos + 1]*256 + ast_ptr_raw[ast_pos + 2];
               :	    
               :    	    if (source_file_gps) {
               :		if (source_file_gps_version == 1) {
               :		    current_time = ((ast_ptr_raw[ast_pos + ast_size_datablock + 6]<<16 ) +
               :			(ast_ptr_raw[ast_pos + ast_size_datablock + 7] << 8) +
               :			(ast_ptr_raw[ast_pos + ast_size_datablock + 8]) ) / 128.0;
               :		
     4  0.0495 :		} else if (source_file_gps_version == 2) {
   141  1.7457 :		    current_time = ((ast_ptr_raw[ast_pos + ast_size_datablock] ) +
               :			(ast_ptr_raw[ast_pos + ast_size_datablock + 1] << 8) +
               :			(ast_ptr_raw[ast_pos + ast_size_datablock + 2] << 16) +
               :			(ast_ptr_raw[ast_pos + ast_size_datablock + 3] << 24) ) / 1000.0;
               :		}
               :	    } else {
               :		current_time = (t.tv_sec - t3) + t.tv_usec / 1000000.0;
               :	    }
     7  0.0867 :	    ast_output_datablock(ast_ptr_raw + ast_pos, ast_size_datablock + offset, count_plot_processed, 0);
    27  0.3343 :	    if (ast_ptr_raw[ast_pos] == '\x01') {
     6  0.0743 :		ast_procesarCAT01(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count_plot_processed);
    28  0.3467 :		count_plot_processed++;
    16  0.1981 :	    } else if (ast_ptr_raw[ast_pos] == '\x02') {
               :		ast_procesarCAT02(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count_plot_processed);
     2  0.0248 :		count_plot_processed++;
     1  0.0124 :	    } else if (ast_ptr_raw[ast_pos] == '\x08') {
               :		ast_procesarCAT08(ast_ptr_raw + ast_pos + 3, ast_size_datablock, count_plot_processed);
               :		count_plot_processed++;
     1  0.0124 :	    } else {
               :		count_plot_ignored++;
               :	    }
    23  0.2848 :	    ast_pos += ast_size_datablock + offset;
               ://	    usleep(50*1000);
     5  0.0619 :	}
               :	mem_free(ast_ptr_raw);
               :
               :    } else if (!strncasecmp(source, "mult", 4) || !strncasecmp(source, "broa", 4)) {
               :	int *s_reader, yes = 1, i=0,j=0;
               :	fd_set reader_set;
               :	struct sockaddr_in cast_group;
               :	struct ip_mreq mreq;
               :
               :	s_reader = mem_alloc((radar_count/3)*sizeof(int));
               :	FD_ZERO(&reader_set);
               :
               :	while (i<(radar_count/3)) {
               :	    if (i>0 && 
               :		    !strcasecmp(radar_definition[i*3], radar_definition[(i-1)*3]) && 
               :		    !strcasecmp(radar_definition[i*3 + 1], radar_definition[(i-1)*3 +1])) {
               :	    }  else {
               :    		if ( (s_reader[socket_count] = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
               :		    log_printf(LOG_ERROR, "socket reader %s\n", strerror(errno));
               :		    exit(EXIT_FAILURE);
               :        	}
               :		if (!strncasecmp(source, "mult", 4)) {
               :		    if ( setsockopt(s_reader[socket_count], SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
               :			log_printf(LOG_ERROR, "reuseaddr setsockopt reader %s\n", strerror(errno));
               :			exit(EXIT_FAILURE);
               :    		    }
               :		    memset(&cast_group, 0, sizeof(cast_group));
               :		    cast_group.sin_family = AF_INET;
               :		    cast_group.sin_addr.s_addr = inet_addr(radar_definition[i*3 + 0]); //multicast group ip
               :		    cast_group.sin_port = htons((unsigned short int)strtol(radar_definition[i*3 + 1], NULL, 0)); //multicast group port
               :    		    if ( bind(s_reader[socket_count], (struct sockaddr *) &cast_group, sizeof(cast_group)) < 0) {
               :			log_printf(LOG_ERROR, "bind reader %s\n", strerror(errno));
               :			exit(EXIT_FAILURE);
               :    		    }
               :		    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
               :		    mreq.imr_multiaddr = cast_group.sin_addr; //multicast group ip
               :    		    if (setsockopt(s_reader[socket_count], IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
               :    	    		log_printf(LOG_ERROR, "add_membership setsockopt reader\n");
               :			exit(EXIT_FAILURE);
               :		    }
               :		} else if (!strncasecmp(source, "broa", 4)) {
               :		    if ( setsockopt(s_reader[socket_count], SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0) {
               :			log_printf(LOG_ERROR, "reuseaddr setsockopt reader %s\n", strerror(errno));
               :			exit(EXIT_FAILURE);
               :    		    }
               :		    cast_group.sin_family = AF_INET;
               :		    cast_group.sin_addr.s_addr = htonl(INADDR_ANY);
               :		    cast_group.sin_port = htons((unsigned short int)strtol(radar_definition[i*3 + 1], NULL, 0)); //broadcast group port
               :    		    if ( bind(s_reader[socket_count], (struct sockaddr *) &cast_group, sizeof(cast_group)) < 0) {
               :			log_printf(LOG_ERROR, "bind reader %s\n", strerror(errno));
               :			exit(EXIT_FAILURE);
               :    		    }
               :		}
               :	
               :		log_printf(LOG_VERBOSE, "%d/%d/%d %s:%s\n", socket_count, i, radar_count/3, 
               :		    radar_definition[i*3 + 0], radar_definition[i*3 + 1] );
               :		
               :		socket_count++;
               :	    }
               :	    i++;
               :	}
               :	
               :	ast_ptr_raw = (unsigned char *) mem_alloc(MAX_PACKET_LENGTH);
               :	while (1) {
               :	    struct timeval timeout;	
               :	    int select_count;
               :	    int addrlen = sizeof(struct sockaddr_in);
               :	    unsigned char byte;
               :	    unsigned long sec;
               :
               :	    bzero(ast_ptr_raw, MAX_PACKET_LENGTH);
               :
               :	    timeout.tv_sec = 1; timeout.tv_usec = 0;
               :	    
               :	    FD_ZERO(&reader_set);
               :	    for (i=0; i<socket_count; i++) 
               :		FD_SET(s_reader[i], &reader_set);
               :
               :	    select_count = select(s_reader[socket_count - 1] + 1, &reader_set, NULL, NULL, &timeout);
               :	    if ( select_count > 0) {
               :    		gettimeofday(&t, NULL);
               :
               :		i=0;
               :		while (i<socket_count) {
               :		    if (FD_ISSET(s_reader[i], &reader_set)) {
               :	    		memset(&cast_group, 0, sizeof(cast_group));
               :		        if (recvfrom(s_reader[i], ast_ptr_raw, MAX_PACKET_LENGTH, 0, (struct sockaddr *) &cast_group, &addrlen) < 0) {
               :	    		    log_printf(LOG_ERROR, "recvfrom reader\n");
               :	    	    	    exit(EXIT_FAILURE);
               :    			}
               :			current_time = (t.tv_sec - t3) + t.tv_usec / 1000000.0;
               :			j=0;
               :			while (j<(radar_count/3)) {
               :			    if (!strcasecmp(inet_ntoa(cast_group.sin_addr), radar_definition[j*3+2])) {
               :				ast_size_datablock = (ast_ptr_raw[1]<<8) + ast_ptr_raw[2];
               :
               :		    	        if (fd_out != -1) {
               :				    if (dest_file_gps) {
               :				        sec = (t.tv_sec - t3) * 1000 + (t.tv_usec/1000);
               ://					sprintf(tmp,"%08lX", sec);
               :					byte = sec & 0xFF;
               :					memcpy(ast_ptr_raw + ast_size_datablock, &byte, 1);
               :					byte = (sec >> 8) & 0xFF;
               :					memcpy(ast_ptr_raw + ast_size_datablock + 1, &byte, 1);
               :					byte = (sec >> 16) & 0xFF;
               :					memcpy(ast_ptr_raw + ast_size_datablock + 2, &byte, 1);
               :					byte = (sec >> 24) & 0xFF;
               :		                	memcpy(ast_ptr_raw + ast_size_datablock + 3, &byte, 1); //para fechar con hora gps
               :					if ( (write(fd_out, ast_ptr_raw, ast_size_datablock + 4) ) == -1) {
               :					    log_printf(LOG_ERROR, "ERROR: %s\n", strerror(errno));
               :					}
               :				    } else {
               :					if ( (write(fd_out, ast_ptr_raw, ast_size_datablock) ) == -1) {
               :					    log_printf(LOG_ERROR, "ERROR: %s\n", strerror(errno));
               :					}
               :				    }
               :				}
               :				ast_output_datablock(ast_ptr_raw, ast_size_datablock, count_plot_processed, 0);
               :				if (ast_ptr_raw[0] == '\x01')
               :				    ast_procesarCAT01(ast_ptr_raw + 3, ast_size_datablock, count_plot_processed);
               :				if (ast_ptr_raw[0] == '\x02')
               :				    ast_procesarCAT02(ast_ptr_raw + 3, ast_size_datablock, count_plot_processed);
               :				if (ast_ptr_raw[0] == '\x08')
               :				    ast_procesarCAT08(ast_ptr_raw + 3, ast_size_datablock, count_plot_processed);
               :			    }
               :			    j++;
               :			}
               :		    }
               :		    i++;
               :		}
               :	    } else if (select_count == 0) {
               :		log_printf(LOG_VERBOSE, "1 sec warning\n");
               :		timeout.tv_sec = 1;
               :		timeout.tv_usec = 0;
               :	    } else {
               :		log_printf(LOG_ERROR, "socket error %s\n", strerror(errno));
               :		exit(EXIT_FAILURE);
               :	    }
               :	}
               :	mem_free(s_reader);
               :	mem_free(ast_ptr_raw);
               :    }
               :
               :    log_printf(LOG_NORMAL, "plot proccessed: %ld\tplot ignored: %ld\n", 
               :	count_plot_processed, count_plot_ignored);
               :
               :    {
               :	struct datablock_plot dbp;
               :	dbp.cat = CAT_255;
               :	if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
               :    	    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
               :	}
               :    }
               :    log_flush();
               :    close(s);
               :    close(fd_in);
               :    close(fd_out);
               :    return 0;
               :
     1  0.0124 :}
/* 
 * Total samples for file : "/root/prog/conversor_asterix2/src/reader_network.c"
 * 
 *    289  3.5781
 */


/* 
 * Command line: opannotate --source --output-dir=anot1 ./reader_network 
 * 
 * Interpretation of command line:
 * Output annotated source file with samples
 * Output all files
 * 
 * CPU: PIII, speed 747.746 MHz (estimated)
 * Counted CPU_CLK_UNHALTED events (clocks processor is not halted) with a unit mask of 0x00 (No unit mask) count 6000
 */
