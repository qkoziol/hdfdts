# Makefile to build the IO speed measurement tool.

CC=icc -g

# Uncomment these if you want to use FFIO.
#FFIO_CPPFLAGS=-DFFIO_MACHINE
#FFIO_LIBS=-lffio -lrt

# Uncomment these if you want to use gcc.
#CC=gcc

# If errors, such as integer overflow, occur for file size larger than 2GB,
# you may want to uncomment the GCC_CPPFLAGS here.
#GCC_CPPFLAGS=-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

/* need _XOPEN_SOURCE=600 to include the definition of posix_memalign().*/
CPPFLAGS=-D_XOPEN_SOURCE=600 $(FFIO_CPPFLAGS) $(GCC_CPPFLAGS)
CFLAGS=-DNDEBUG $(CPPFLAGS)

INCLUDES= \
	-I/u/ncsa/ymuqun/hugs/hdf5-2006-10-12/hdf5/include \
	-I/u/ncsa/ymuqun/szipbin/include \

LIBDIRS= \
	-L/u/ncsa/ymuqun/hugs/hdf5-2006-10-12/hdf5/lib \
	-L/u/ncsa/ymuqun/szipbin/lib \
        -L/usr/lib

LIBS= \
	-lhdf5 \
	-lsz \
	-lz


iokernel: iokernel.c
	$(CC) $(CFLAGS) $(INCLUDES) iokernel.c $(LIBDIRS) $(LIBS) -o $@

clean:
	rm -f iokernel *~
