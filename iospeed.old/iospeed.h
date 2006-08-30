#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>

#ifdef FFIO_MACHINE
#include "ffio.h"
#endif

#include "gopt.h"

#define KB 1024
#define MB KB*KB

/* different UNIX write modes */
#define	UNIX_BASIC	0	/* basic write */
#define	UNIX_DIRECTIO	1	/* Direct IO */
#define	UNIX_NONBLOCK	2	/* Non-blocking IO */
