# Makefile to build the HUGS IO Kernel benchmark.

# edit this with where your HDF5 library is.
H5HOME=/scratch/users/acheng/HDF5/vdev/co/hdf5

CC=$(H5HOME)/bin/h5cc

iokernel: iokernel.c
	$(CC) iokernel.c -o $@

clean:
	rm -f iokernel
