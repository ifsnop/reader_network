
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include <debug/memory.h>
#include <debug/log.h>
#include <debug/hex.h>

#define SIZE 8192

static void *getfile (int fd,size_t *length)
{
   size_t size = SIZE;
   void *buf;
   ssize_t result;

   if ((buf = mem_alloc (size)) == NULL)
	 {
		log_printf (LOG_ERROR,"failed to allocate memory: %m\n");
		return (NULL);
	 }

   *length = 0;

   while ((result = read (fd,buf + *length,size - *length)) > 0)
	 {
		*length += result;

		if (*length == size)
		  {
			 void *ptr;

			 size += SIZE;

			 if ((ptr = mem_realloc (buf,size)) == NULL)
			   {
				  log_printf (LOG_ERROR,"failed to allocate memory: %m\n");
				  mem_free (buf);
				  return (NULL);
			   }

			 buf = ptr;
		  }
	 }

   if (result < 0)
	 {
		log_printf (LOG_ERROR,"read: %m\n");
		mem_free (buf);
		return (NULL);
	 }

   return (buf);
}

int main (int argc,char *argv[])
{
   void *buf;
   size_t length;
   int fd;

   mem_open (NULL);
   log_open (NULL,LOG_NORMAL,0);
   atexit (mem_close);
   atexit (log_close);

   if (argc != 2)
	 {
		const char *progname;

		(progname = strrchr (argv[0],'/')) != NULL ? progname++ : (progname = argv[0]);
		log_printf (LOG_ERROR,"usage: %s <filename>\n",progname);
		exit (EXIT_FAILURE);
	 }

   if ((fd = open (argv[1],O_RDONLY)) < 0)
	 {
		log_printf (LOG_ERROR,"open %s: %m\n",argv[1]);
		exit (EXIT_FAILURE);
	 }

   if ((buf = getfile (fd,&length)) == NULL)
	 {
		close (fd);
		exit (EXIT_FAILURE);
	 }

   hexdump (LOG_NORMAL,buf,length);

   mem_free (buf);
   close (fd);

   exit (EXIT_SUCCESS);
}

