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

/* IO Error Code */
#define IOERR		-1	/* IO error occured */
#define IOSLOW		-2	/* IO speed too slow */

#define NUMOPTIONS	7	/* number of options */
