#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define FFIO_MACHINE

#ifdef FFIO_MACHINE
#include "ffio.h"
#endif

#include "gopt.h"

#define KB 1024
#define MB KB*KB

