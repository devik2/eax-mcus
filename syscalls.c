/* Support files for GNU libc.  Files in the system namespace go here.
   Files in the C namespace (ie those that do not start with an
   underscore) go in .c.  */

#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <reent.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "stm_util.h"

/* Forward prototypes.  */
int     _system     _PARAMS ((const char *));
int     _rename     _PARAMS ((const char *, const char *));
int     _isatty		_PARAMS ((int));
clock_t _times		_PARAMS ((struct tms *));
int     _gettimeofday	_PARAMS ((struct timeval *, void *));
void    _raise 		_PARAMS ((void));
int     _unlink		_PARAMS ((const char *));
int     _link 		_PARAMS ((void));
int     _stat 		_PARAMS ((const char *, struct stat *));
int     _fstat 		_PARAMS ((int, struct stat *));
caddr_t _sbrk		_PARAMS ((int));
int     _getpid		_PARAMS ((int));
int     _kill		_PARAMS ((int, int));
void    _exit		_PARAMS ((int));
int     _close		_PARAMS ((int));
int     _swiclose	_PARAMS ((int));
int     _open		_PARAMS ((const char *, int, ...));
int     _swiopen	_PARAMS ((const char *, int));
int     _write 		_PARAMS ((int, char *, int));
int     _swiwrite	_PARAMS ((int, char *, int));
int     _lseek		_PARAMS ((int, int, int));
int     _swilseek	_PARAMS ((int, int, int));
int     _read		_PARAMS ((int, char *, int));
int     _swiread	_PARAMS ((int, char *, int));

/* Register name faking - works in collusion with the linker.  */
register char * stack_ptr asm ("sp");


/* following is copied from libc/stdio/local.h to check std streams */
extern void   _EXFUN(__sinit,(struct _reent *));
#define CHECK_INIT(ptr) \
  do						\
    {						\
      if ((ptr) && !(ptr)->__sdidinit)		\
	__sinit (ptr);				\
    }						\
  while (0)

/* Adjust our internal handles to stay away from std* handles.  */
#define FILE_HANDLE_OFFSET (0x20)

static int monitor_stdin;
static int monitor_stdout;
static int monitor_stderr;

/* Struct used to keep track of the file position, just so we
   can implement fseek(fh,x,SEEK_CUR).  */
typedef struct
{
  const struct fs_item *fi; // null for unused
  int pos;
}
poslog;

#define MAX_OPEN_FILES 20
static poslog openfiles [MAX_OPEN_FILES];

static int find_free_slot ()
{
  int i;
  for (i = 3; i < MAX_OPEN_FILES; i ++) if (!openfiles[i].fi) return i;
  return -1;
}

int
_read (int file,
       char * ptr,
       int len)
{
  errno = EBADF;
#if 0
  if (file>=MAX_OPEN_FILES || !openfiles[file].fi) return -1;
  errno = 0;
  const struct fs_item *fi = openfiles[file].fi;
  if (len<0 || len>fi->len-openfiles[file].pos)
	  len = fi->len-openfiles[file].pos;
  memcpy(ptr,fi->data+openfiles[file].pos,len);
  openfiles[file].pos += len;
  printf("read len %d file %d\r\n",len,file);
#else
  return -1;
#endif
  return len;
}

int
_lseek (int file,
	int ptr,
	int dir)
{
  errno = EBADF;
#if 0
  if (file>=MAX_OPEN_FILES || !openfiles[file].fi) return -1;
  errno = 0;
  if (dir == 1) openfiles[file].pos += ptr;
  if (dir == 0) openfiles[file].pos = ptr;
  if (dir < 2)
	  return openfiles[file].pos;
  printf("seek %d to %d %d\r\n",file,ptr,dir);
  errno = ENOSYS;
#endif
  return -1;
}

void usart_tx_1(unsigned char c) {}
void usart_tx(const char *c) 
{
	while (*c) usart_tx_1(*(c++));
}

int
_write (int    file,
	char * ptr,
	int    len)
{
  if (file<=2) {
	  for (;len;len--,ptr++) usart_tx_1(*ptr);
	  return len;
  }
  errno = EBADF;
  if (file>=MAX_OPEN_FILES || !openfiles[file].fi) return -1;
  errno = ENOSYS;
  return -1;
}

void *fs_find(const char*p);

int
_open (const char * path,
       int          flags,
       ...)
{
#if 1
  usart_tx("_open called !\r\n");
  return -1;
#endif
  jprintf("OPEN %s\r\n",path);
  const struct fs_item *f = fs_find(path);
  if (!f) {
	  errno = ENOENT;
	  return -1;
  }
  int n = find_free_slot();
  if (n<0) {
	  errno = EMFILE;
	  return -1;
  }
  openfiles[n].pos = 0;
  openfiles[n].fi = f;
  errno = 0;
  return n;
}

int
_close (int file)
{
  errno = EBADF;
#if 1
  return -1;
#endif
  if (file<3 || file>=MAX_OPEN_FILES) return -1;
  if (!openfiles[file].fi) return -1;
  openfiles[file].fi = NULL;
  errno = 0;
  return 0;
}

int
_kill (int pid, int sig)
{
  usart_tx("_kill called !\r\n");
  errno = ENOSYS;
  return -1;
}

void
_exit (int status)
{
  errno = ENOSYS;
  usart_tx("_exit called !\r\n");
  for(;;);
}

int
_getpid (int n)
{
  return 1;
  n = n;
}

char * heap_end;
caddr_t
_sbrk (int incr)
{
  extern char   end asm ("end");	/* Defined by the linker.  */
  char *        prev_heap_end;

  if (heap_end == NULL)
    heap_end = (& end)+64;

  prev_heap_end = heap_end;
#if 1
  jprintf("sbrk %d %X",incr,heap_end);
#endif
#if 1
  if (heap_end + incr > stack_ptr)
    {
      /* Some of the libstdc++-v3 tests rely upon detecting
	 out of memory errors, so do not abort here.  */
#if 0
      extern void abort (void);

      _write (1, "_sbrk: Heap and stack collision\n", 32);
      
      abort ();
#else
      jprintf("MEM ERROR"); 
      errno = ENOMEM;
      return (caddr_t) -1;
#endif
    }
#endif 
  heap_end += incr;

  return (caddr_t) prev_heap_end;
}


int
_fstat (int file, struct stat * st)
{
  memset (st, 0, sizeof (* st));
  st->st_blksize = 1024;
  if (file<3) {
  	st->st_mode = S_IFCHR;
	return 0;
  }
  errno = EBADF;
#if 1
  return -1;
#else
  if (file>=MAX_OPEN_FILES || !openfiles[file].fi) return -1;
  const struct fs_item *fi = openfiles[file].fi;
  st->st_mode = S_IFREG;
  st->st_size = fi->len;
#endif
  errno = 0;
  return 0;
}

int _stat (const char *fname, struct stat *st)
{
 jprintf("STAT %s\r\n",fname);
  errno = ENOSYS;
  return -1;
}

int
_link (void)
{
  errno = ENOSYS;
  return -1;
}

int
_unlink (const char *path)
{
  errno = ENOSYS;
  return -1;
}

void
_raise (void)
{
  usart_tx("RAISE ERROR\r\n"); 
  return;
}

int
_gettimeofday (struct timeval * tp, void * tzvp)
{
  struct timezone *tzp = tzvp;
  if (tp)
    {
      tp->tv_sec = 0;
      tp->tv_usec = 0;
    }

  /* Return fixed data for the timezone.  */
  if (tzp)
    {
      tzp->tz_minuteswest = 0;
      tzp->tz_dsttime = 0;
    }

  return 0;
}

/* Return a clock that ticks at 100Hz.  */
clock_t 
_times (struct tms * tp)
{
  clock_t timeval=0;

  if (tp)
    {
      tp->tms_utime  = timeval;	/* user time */
      tp->tms_stime  = 0;	/* system time */
      tp->tms_cutime = 0;	/* user time, children */
      tp->tms_cstime = 0;	/* system time, children */
    }
  
  return timeval;
};


int
_isatty (int fd)
{
  return fd<3;
}

int
_system (const char *s)
{
  errno = ENOSYS;
  return -1;
}

int
_rename (const char * oldpath, const char * newpath)
{
  errno = ENOSYS;
  return -1;
}
