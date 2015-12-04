#!/bin/bash
#
#
# Platypus (64 bit CentOS6)
# Ostrich (64 bit RHEL6-PPC, big-endian)
# Emu (Solaris 11, big-endian)
# OSX1010DEV Mac OS X 10.10.x
# WREN Mac OS X 10.9.x
# KITE Mac OS X 10.8.x

UNAME="unknown"
if [ -x /usr/bin/uname ]
then
  UNAME=`/usr/bin/uname -n`
fi
if [ -x /usr/local/bin/uname ]
then
  UNAME=`/usr/local/bin/uname -n`
fi
if [ -x /bin/uname ]
then
  UNAME=`/bin/uname -n`
fi

make_bin="make"
for x in 1; do

  ENABLE_64BIT=""
  ENABLE_PARALLEL=""
  ENABLE_SZIP=""
  HDF_DIR="0"
  if [[ $UNAME ==  "jam" ]]; then
#jam gnu 5.2, serial
    if [[ $x == 1 ]]; then
      export CC="/usr/hdf/bin/gcc52/gcc"
      export FC="/usr/hdf/bin/gcc52/gfortran"
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      ENABLE_64BIT="--enable-64bit"
#jam intel, serial
    elif [[ $x == 2 ]]; then
      export CC="icc"
      export FC="ifort"
      export FLIBS=""
      export LIBS=""
      #ENABLE_64BIT="--enable-64bit"
#jam pgi, serial
    elif [[ $x == 3 ]]; then
      export CC="pgcc"
      export FC="pgf90"
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      #ENABLE_64BIT="--enable-64bit"
#jam gnu 5.2, serial, -fdefault-real-8 -fdefault-integer-8
#  elif [[ $x == 4 ]]; then
#    export CC="/usr/hdf/bin/gcc52/gcc"
#    export FC="/usr/hdf/bin/gcc52/gfortran"
#    export FFLAGS="-fdefault-real-8 -fdefault-integer-8"
#    export FLIBS="-Wl,--no-as-needed -ldl"
#    export LIBS="-Wl,--no-as-needed -ldl"
#    ENABLE_64BIT="--enable-64bit"
    elif [[ $x == 4 ]]; then
      MPI="/mnt/hdf/packages/mpich/3.1.4_gnu4.9.2/i686/bin"
      export CC="$MPI/mpicc"
      export FC="$MPI/mpif90"
      export FFLAGS=""
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      ENABLE_64BIT="--enable-64bit"
      ENABLE_PARALLEL="--enable-parallel"
      WITH_MPI="--with-mpi=$MPI"
    else 
      break
    fi
###########################################
#   _   _   _   _   _   _   _   _  
#  / \ / \ / \ / \ / \ / \ / \ / \ 
# ( P | L | A | T | Y | P | U | S )
#  \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ 
#
###########################################
  elif [[ $UNAME == "platypus" ]]; then
#jam gnu , serial
    if [[ $x == 1 ]]; then
     # export CC="/usr/hdf/bin/gcc52/gcc"
    #  export FC="/usr/hdf/bin/gcc52/gfortran"
      export CC="gcc"
      export FC="gfortran"
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/platypus" 
      SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
      ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
      #ENABLE_64BIT="--enable-64bit"
#jam intel, serial
    elif [[ $x == 2 ]]; then
      export CC="icc"
      export FC="ifort"
      export FLIBS=""
      export LIBS=""
      #ENABLE_64BIT="--enable-64bit"
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/platypus-intel" 
      SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
      ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
#jam pgi, serial
    elif [[ $x == 3 ]]; then
      export CC="pgcc"
      export FC="pgf90"
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/platypus-pgi"
      #ENABLE_64BIT="--enable-64bit"
#jam gnu 5.2, serial, -fdefault-real-8 -fdefault-integer-8
#  elif [[ $x == 4 ]]; then
#    export CC="/usr/hdf/bin/gcc52/gcc"
#    export FC="/usr/hdf/bin/gcc52/gfortran"
#    export FFLAGS="-fdefault-real-8 -fdefault-integer-8"
#    export FLIBS="-Wl,--no-as-needed -ldl"
#    export LIBS="-Wl,--no-as-needed -ldl"
#    ENABLE_64BIT="--enable-64bit"
    elif [[ $x == 4 ]]; then
      MPI="/mnt/hdf/packages/mpich/3.1.4_gnu4.9.2/x86_64/bin"
      export CC="$MPI/mpicc"
      export FC="$MPI/mpif90"
      export FFLAGS=""
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      ENABLE_64BIT="--enable-64bit"
      ENABLE_PARALLEL="--enable-parallel"
      WITH_MPI="--with-mpi=$MPI"
    elif [[ $x == 5 ]]; then
      MPI="/mnt/hdf/packages/mpich/3.1.4_intel15.0.3/x86_64/bin"
      export CC="$MPI/mpicc"
      export FC="$MPI/mpif90"
      export FFLAGS=""
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      ENABLE_64BIT="--enable-64bit"
      ENABLE_PARALLEL="--enable-parallel"
      WITH_MPI="--with-mpi=$MPI"

    else 
      break
    fi

###########################################
#   _   _   _   _   _   _
#  / \ / \ / \ / \ / \ / \ 
# ( M | O | O | H | A | N |
#  \_/ \_/ \_/ \_/ \_/ \_/
#
###########################################

  elif [[ $UNAME == "moohan.ad.hdfgroup.org" ]]; then
#jam gnu , serial
    if [[ $x == 1 ]]; then
     # export CC="/usr/hdf/bin/gcc52/gcc"
    #  export FC="/usr/hdf/bin/gcc52/gfortran"
      export CC="gcc"
      export FC="gfortran"
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/moohan"
      TEST_SZIP=`grep -iq "szip" $HDF_DIR/bin/h5cc;echo $?`
      if [[ $TEST_SZIP == 0 ]]; then
        SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
        ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
      fi
      #SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
      #TEST_SZIP=`grep -i "szip" $HDF_DIR/bin/h5cc`
      #if [[ $TEST_SZIP == 0 ]]; then
      #  SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
      #  ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
      #fi
      #ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
      #ENABLE_64BIT="--enable-64bit"
    else
      break
    fi
##########################################
#   _   _   _   _   _   _   _  
#  / \ / \ / \ / \ / \ / \ / \ 
# ( O | S | T | R | I | C | H )
#  \_/ \_/ \_/ \_/ \_/ \_/ \_/ 
#
##########################################

  elif [[ $UNAME ==  "ostrich" ]]; then
    #gnu 4.4.7, serial
    if [[ $x == 1 ]]; then
      export CC="gcc"
      export FC="gfortran"
      export FLIBS="-Wl,--no-as-needed -ldl"
      export LIBS="-Wl,--no-as-needed -ldl"
      ENABLE_64BIT="--enable-64bit"
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/ostrich"
      SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
      ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
    #xlf, serial
    elif [[ $x == 2 ]]; then
      export CC="/opt/xl/xlf15.1_xlc13.1/xlc"
      export FC="/opt/xl/xlf15.1_xlc13.1/xlf"
      export FLIBS=""
      export LIBS=""
      ENABLE_64BIT="--enable-64bit"
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/ostrich-xl"
      TEST_SZIP=`grep -iq "szip" $HDF_DIR/bin/h5cc;echo $?`
      if [[ $TEST_SZIP == 0 ]]; then
        SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
        ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
      fi
    elif [[ $x == 3 ]]; then
      export CC="/opt/xl/xlf15.1_xlc13.1/xlc"
      export FC="/opt/xl/xlf15.1_xlc13.1/xlf"
      export FLIBS=""
      export LIBS=""
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/ostrich-xl"
      TEST_SZIP=`grep -iq "szip" $HDF_DIR/bin/h5cc;echo $?`
      if [[ $TEST_SZIP == 0 ]]; then
        SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
        ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
      fi
    else 
      break
    fi

##########################################
#   _   _   _  
#  / \ / \ / \ 
# ( E | M | U )
#  \_/ \_/ \_/ 
#
##########################################
  elif [[ $UNAME ==  "emu" ]]; then
    make_bin="gmake"
    HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/emu64"
    SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
    ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
    export CC="cc"
    export FC="f90"
    export FLIBS=""
    export FCFLAGS="-m64"
    export CFLAGS="-m64"
    export LIBS=""
    if [[ $x == 1 ]]; then
      ENABLE_64BIT=""
    elif [[ $x == 2 ]]; then
      ENABLE_64BIT="--enable-64bit"
    else 
      break
    fi

##########################################
#   _   _   _   _   _   _   _  
#  / \ / \ / \ / \ / \ / \ / \ 
# ( O | X | S | 1 | 0 | 1 | 0 )
#  \_/ \_/ \_/ \_/ \_/ \_/ \_/
#
##########################################
  elif [[ $UNAME ==  "osx1010dev.ad.hdfgroup.org" ]]; then
    if [[ $x == 1 ]]; then
      export CC="gcc"
      export FC="gfortran"
      export FLIBS="-Wl,-ldl"
      export LIBS="-Wl,-ldl"
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/osx1010test"
      SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
      ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
    elif [[ $x == 2 ]]; then
      export CC="icc"
      export FC="ifort"
      export FLIBS=""
      export LIBS=""
      HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/osx1010test-intel"
      TEST_SZIP=`grep -iq "szip" $HDF_DIR/bin/h5cc;echo $?`
      if [[ $TEST_SZIP == 0 ]]; then
        SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
        ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
      fi
    else
      break
    fi
  fi
  
   mkdir test.$x
   cd test.$x
   if [[ $HDF_DIR == 0 ]]; then
       mkdir trunk; cd trunk
       HDF_DIR="$PWD/hdf5"
       /home/brtnfld/hdf5/trunk/configure $ENABLE_PARALLEL --disable-fortran --disable-hl; $make_bin -j 8 > result.txt 2>&1; $make_bin install
       cd ..
   fi
   git clone -b develop https://github.com/CGNS/CGNS.git

  cd CGNS/src
  ./configure \
  --with-hdf5=$HDF_DIR \
  --with-fortran=yes $ENABLE_PARALLEL \
  --enable-lfs $ENABLE_64BIT $WITH_MPI $ENABLE_SZIP \
  --disable-shared \
  --enable-debug \
  --with-zlib \
  --disable-cgnstools \
  --disable-x
  $make_bin
  $make_bin test &> ../../../results.$x.txt
  status=$?
  if [[ $status != 0 ]]; then
      echo "CGNS TEST #FAILED"
      exit $status
  fi
   

  cd ../../../
  #rm -fr test.$x

done
tail -n 100 results.*

for x in 1; do

  ENABLE_64BIT=""  
  if [ -d "test.$x" ]; then
  	cd test.$x
	ENABLE_SZIP="OFF"
  	if [[ $UNAME ==  "jam" ]]; then
#jam gnu 5.2, serial
      		CC="/usr/hdf/bin/gcc52/gcc"
      		FC="/usr/hdf/bin/gcc52/gfortran"
      		HDF_DIR="$PWD/trunk/hdf5"
      		CGNS="$PWD/CGNS"
		TEST_SZIP=`grep -iq "szip" $HDF_DIR/bin/h5cc;echo $?`
		if [[ $TEST_SZIP == 0 ]]; then
		    SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
		    ENABLE_SZIP="ON -D SZIP_LIBRARY:PATH=$SZIP/libsz.a"
		fi
        elif [[ $UNAME == "platypus" ]]; then
      		CC="gcc"
      		FC="gfortran"
      		HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/platypus"
      		CGNS="$PWD/CGNS"
		TEST_SZIP=`grep -iq "szip" $HDF_DIR/bin/h5cc;echo $?`
		if [[ $TEST_SZIP == 0 ]]; then
		    SZIP=`cat $HDF_DIR/bin/h5cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
		    ENABLE_SZIP="ON -D SZIP_LIBRARY:PATH=$SZIP/libsz.a"
		fi
    	else 
		break
    	fi
  
   	mkdir CGNS_build
   	cd CGNS_build

   	cmake \
	     -D CMAKE_C_COMPILER:PATH=$CC \
	     -D CMAKE_C_FLAGS:STRING="-g" \
	     -D CMAKE_Fortran_FLAGS:STRING="-g" \
	     -D CMAKE_BUILD_TYPE:STRING="Debug" \
	     -D CMAKE_Fortran_COMPILER:PATH=$FC \
	     -D CGNS_BUILD_SHARED:BOOL=OFF \
	     -D CGNS_USE_SHARED:BOOL=OFF \
	     -D CGNS_ENABLE_64BIT:BOOL=ON \
	     -D HDF5_NEED_ZLIB:BOOL=ON \
	     -D HDF5_NEED_SZIP:BOOL=$ENABLE_SZIP \
	     -D CMAKE_EXE_LINKER_FLAGS:STRING="-Wl,--no-as-needed -ldl" \
	     -D CMAKE_STATIC_LINKER_FLAGS:STRING="" \
	     -D CGNS_ENABLE_HDF5:BOOL=ON \
	     -D CGNS_ENABLE_TESTS:BOOL=ON \
	     -D CGNS_ENABLE_LFS:BOOL=OFF \
	     -D CGNS_BUILD_CGNSTOOLS:BOOL=OFF \
	     -D HDF5_LIBRARY:STRING=$HDF_DIR/lib/libhdf5.a \
	     -D HDF5_INCLUDE_PATH:PATH=$HDF_DIR/include \
	     -D CGNS_ENABLE_SCOPING:BOOL=OFF \
	     -D CGNS_ENABLE_FORTRAN:BOOL=ON \
	     -D CGNS_ENABLE_PARALLEL:BOOL=OFF \
	     -D CMAKE_INSTALL_PREFIX:PATH="./" \
    $CGNS 
    make
    make test
    status=$?
    if [[ $status != 0 ]]; then
	echo "CGNS TEST #FAILED"
	exit $status
    fi
  fi

done

exit $status

#     -D MPIEXEC:STRING=$MPI/mpiexec \
#     -D MPI_C_COMPILER:STRING=$MPI/mpicc \
#     -D MPI_Fortran_COMPILER:STRING=$MPI/mpif90 \
#  -D HDF5_NEED_MPI:BOOL=ON \
