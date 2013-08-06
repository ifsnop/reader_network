
#include "includes.h"

float current_time=0.0;
int s;
struct sockaddr_in cliaddr,srvaddr;

static void fail (const char *fmt, ...)
{
    va_list ap;
    va_start (ap,fmt);
#ifndef DEBUG_LOG
    if (fmt != NULL) log_vprintf (LOG_ERROR,fmt,ap);
#else   /* #ifndef DEBUG_LOG */
    if (fmt != NULL) {
	vfprintf (stderr,fmt,ap);
	fflush (stderr);
    }
#endif  /* #ifndef DEBUG_LOG */
    va_end (ap);
    exit (EXIT_FAILURE);
}


int main(int argc, char *argv[]) {
int fd;

ssize_t ast_size_total;
ssize_t ast_pos;
ssize_t ast_size_tmp;
int ast_size_datablock;
unsigned char *ast_ptr_raw;
struct timeval t;
struct tm *t2;
time_t t3;
struct hostent *h;
int loop=1;

    mem_open(fail);
    if (log_open(NULL, LOG_VERBOSE, /*LOG_TIMESTAMP |*/ LOG_HAVE_COLORS | LOG_PRINT_FUNCTION | LOG_DEBUG_PREFIX_ONLY)) {
	fprintf(stderr, "log_open failed: %m\n");
	exit (EXIT_FAILURE);
    }
    
    atexit(mem_close);
    atexit(log_close);
    
    bzero(full_tod, MAX_RADAR_NUMBER*TTOD_WIDTH);	

    if (argc != 2) {
	log_printf(LOG_NORMAL,"%s <input file>\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    log_printf(LOG_NORMAL, "init...\n");
    
    gettimeofday(&t, NULL);
    t2 = gmtime(&t.tv_sec);
    t2->tm_sec = 0; t2->tm_min = 0; t2->tm_hour = 0;
    t3 = mktime(t2); //segundos a las 00:00:00 de hoy
    
    if ( (fd = open(argv[1], O_RDONLY)) == -1) {
	log_printf(LOG_ERROR, "ERROR open: %s\n", strerror(errno));
	exit(1);
    }
        
    if ( (ast_size_total = lseek(fd, 0, SEEK_END)) == -1) {
	log_printf(LOG_ERROR, "ERROR lseek (seek_end): %s\n", strerror(errno));
	exit(1);
    }

    if ( lseek(fd, 0, SEEK_SET) == -1) {
	log_printf(LOG_ERROR, "ERROR lseek (seek_set): %s\n", strerror(errno));
	exit(1);
    }

    ast_ptr_raw = (unsigned char *) mem_alloc(ast_size_total);

    if ( (ast_size_tmp = read(fd,ast_ptr_raw,ast_size_total)) != ast_size_total) {
	log_printf(LOG_ERROR, "ERROR read: %s\n", strerror(errno));
	exit(1);
    }
    
    log_printf(LOG_NORMAL, "readed %d bytes\n", ast_size_total);

    if ( (h = gethostbyname(MULTICAST_PLOTS_GROUP)) == NULL ) {
	log_printf(LOG_ERROR, "ERROR gethostbyname");
	exit(1);
    };

    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
    memset(&srvaddr, 0, sizeof(struct sockaddr_in));
    srvaddr.sin_family = h->h_addrtype;
    memcpy((char*) &srvaddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    srvaddr.sin_port = htons(MULTICAST_PLOTS_PORT);

    if ( (s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        log_printf(LOG_ERROR, "ERROR socket: %s\n", strerror(errno));
        exit(1);
    }
    
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //htonl(INADDR_ANY);
    cliaddr.sin_port = htons(0);
    if (  bind(s, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) <0 ) {
        log_printf(LOG_ERROR, "ERROR socket: %s\n", strerror(errno));
        exit(1);
    }

    setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    
    ast_pos = 0;
    while (ast_pos < ast_size_total) {
        gettimeofday(&t, NULL);
	current_time = (t.tv_sec - t3) + t.tv_usec / 1000000.0;
	ast_size_datablock = ast_ptr_raw[ast_pos + 1]*256 + ast_ptr_raw[ast_pos + 2];
//	ast_output_datablock(ast_ptr_raw + ast_pos, ast_size_datablock);
	if (ast_ptr_raw[ast_pos] == 01)
	    ast_procesarCAT01(ast_ptr_raw + ast_pos + 3, ast_size_datablock);
	if (ast_ptr_raw[ast_pos] == 02)
	    ast_procesarCAT02(ast_ptr_raw + ast_pos + 3, ast_size_datablock);
	ast_pos+=ast_size_datablock;
//	usleep(500*1000);
    }
    
    mem_free(ast_ptr_raw);
    log_printf(LOG_NORMAL, "end...\n");
//    log_flush();
    close(fd);
    return 0;

}
