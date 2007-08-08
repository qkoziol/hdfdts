#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#ifdef HAVE_FFIO
#include "ffio.h"
#endif

#include "gopt.h"

#define KB 1024
#define MB (KB*KB)
#define GB (MB*KB)

/* different IO modes */
#define	UNIX_BASIC	1	/* basic write */
#define	Posix_IO	2	/* basic write */
#define	UNIX_DIRECTIO	3	/* Direct IO */
#define	FFIO		4	/* basic write */
#define	UNIX_NONBLOCK	5	/* Non-blocking IO */
#define	MMAP_IO		6	/* mmap IO */
#define	DIRECT_MMAP_IO	7	/* Direct mmap IO */
#define UNIX_SYNC	8	/* Synchronous IO */

/* Different test types */
#define READWRITE_TEST 0
#define WRITE_TEST  1
#define READ_TEST   2

/* different memory modes */
#define MEM_none	0	/* memory that has nothing special */
#define MEM_aligned	1	/* memory that is aligned */

/* IO Error Code */
#define IOERR		-1	/* IO error occured */
#define IOSLOW		-2	/* IO speed too slow */

#define IOSPEED_MIN (0.5*MB)	/* Default minimum acceptable io speed (.5MB/sec) */
#define DEFAULT_FILE_NAME "./test_temp_file.dat"
#define FBSIZE	(4*KB)

#define NUMOPTIONS	7	/* number of options */
