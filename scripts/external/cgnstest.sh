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
  PROC=`/usr/bin/uname -p`
  OSTYPE=`/usr/bin/uname -s`
fi
if [ -x /usr/local/bin/uname ]
then
  UNAME=`/usr/local/bin/uname -n`
  PROC=`/usr/local/bin/uname -p`
  OSTYPE=`/usr/local/bin/uname -s`
fi
if [ -x /bin/uname ]
then
  UNAME=`/bin/uname -n`
  PROC=`/bin/uname -p`
  OSTYPE=`/bin/uname -s`
fi

# set the default C FLAGS
CFLAGS="-g"
FCFLAGS="-g"


# lower case OSTYPE
OSTYPE=`echo $OSTYPE | tr '[:upper:]' '[:lower:]'`

# Remove the domain name if present
UNAME=`echo $UNAME | sed 's;\..*;;'`

BASEDIR=/mnt/scr1/SnapTest/snapshots-cgns

# READ COMMAND LINE FOR THE TEST TO RUN
NARGS=$#
TEST_NO=$1
shift
if [[ $NARGS == 2 ]];then
  TEST_COMPILER=$1
  shift
fi

make_bin="make"
cmake_bin="cmake"
if [[ $UNAME == "ostrich" ]];then
   cmake_bin="/mnt/hdf/packages/cmake/3.3.1/ppc64/bin/cmake"
fi

HDF_DIR="0"


# Set odd/even days of the week
day=$(( $(date +%u) % 2 ))
day=0
if [ $day -eq 0 ]; then #even day tests

   SHARED_STATUS="--disable-shared"
   CGNS_SHARED_STATUS="-D CGNS_BUILD_SHARED:BOOL=OFF -D CGNS_USE_SHARED:BOOL=OFF"

else #odd day tests

   SHARED_STATUS="--enable-shared"
   CGNS_SHARED_STATUS="-D CGNS_BUILD_SHARED:BOOL=ON -D CGNS_USE_SHARED:BOOL=ON"
fi


# COMPILER TESTS: LINKED TO THE CORRESPONDING COMPILED HDF5 LIBRARY

export FLIBS="-Wl,--no-as-needed -ldl"
export LIBS="-Wl,--no-as-needed -ldl"
CMAKE_EXE_LINKER_FLAGS='-Wl,--no-as-needed -ldl'
DASH="-"

export FCFLAGS=""
export CFLAGS=""

if [[ $TEST_COMPILER == "" ]]; then # System default compiler, exclude dash in the path name
    if [[ $OSTYPE == "sunos" ]];then
	make_bin="gmake"
	export CC="cc"
	export FC="f90"
	export FLIBS="-lm"
	export LIBS="-lm"
	CMAKE_EXE_LINKER_FLAGS=""
        export LD_LIBRARY_PATH="/opt/solarisstudio/lib:$LD_LIBRARY_PATH"
    else
	export CC="gcc"
	export FC="gfortran"

	
	if [[ $SHARED_STATUS == "--enable-shared" ]]; then 
	    FCFLAGS="$FCFLAGS -fPIC"
	    CFLAGS="$CFLAGS -fPIC"
	fi

	if [[ $OSTYPE == "darwin"* ]];then
	    export FLIBS=""
	    export LIBS=""
	    CMAKE_EXE_LINKER_FLAGS=""
	    
	    # CURRENTLY DOES NOT SUPPORT SHARED BUILDS ON MAC, CGNS-66
	    SHARED_STATUS="--disable-shared"
	    CGNS_SHARED_STATUS="-D CGNS_BUILD_SHARED:BOOL=OFF -D CGNS_USE_SHARED:BOOL=OFF"
	fi
    fi
    DASH=""

elif [[ $TEST_COMPILER == gcc* ]]; then

    if [[ $TEST_COMPILER == "gcc52" ]]; then
	export CC="/usr/hdf/bin/gcc52/gcc"
	export FC="/usr/hdf/bin/gcc52/gfortran"
    elif [[ $TEST_COMPILER == "gcc49" ]]; then
	export CC="/usr/hdf/bin/gcc49/gcc"
	export FC="/usr/hdf/bin/gcc49/gfortran"
    elif [[ $TEST_COMPILER == "gcc48" ]]; then
	export CC="/usr/hdf/bin/gcc48/gcc"
	export FC="/usr/hdf/bin/gcc48/gfortran"
    else
	echo " *** TESTING SCRIPT ERROR ***"
	echo "   - Unknown gcc compiler specified: $TEST_COMPILER"
	exit 1
    fi
    if [[ $OSTYPE == "darwin"* ]];then
	export FLIBS=""
	export LIBS=""
	CMAKE_EXE_LINKER_FLAGS=""
    fi

elif [[ $TEST_COMPILER == "intel" ]]; then
    export CC="icc"
    export FC="ifort"
    export FLIBS=""
    export LIBS=""
    CMAKE_EXE_LINKER_FLAGS=""
elif [[ $TEST_COMPILER == "pgi" ]]; then
    export CC="pgcc"
    export FC="pgf90"
elif [[ $TEST_COMPILER == "pp" ]]; then
    MPI="/mnt/hdf/packages/mpich/3.1.4_gnu4.9.2/$PROC/bin"
    export CC="$MPI/mpicc"
    export FC="$MPI/mpif90"
    export FFLAGS=""
elif [[ $TEST_COMPILER == "xl" ]]; then
    export CC="/opt/xl/xlf15.1_xlc13.1/xlc"
    export FC="/opt/xl/xlf15.1_xlc13.1/xlf"
    export FLIBS=""
    export LIBS=""
    CMAKE_EXE_LINKER_FLAGS=""
elif [[ $TEST_COMPILER == "emu64" ]]; then
    make_bin="gmake"
    export CC="cc"
    export FC="f90"
    FCFLAGS="$FCFLAGS -O2 -m64"
    CFLAGS="$CFLAGS -O2 -m64"
    export FCFLAGS="$FCFLAGS"
    export CFLAGS="$CFLAGS"
    export FLIBS="-lm"
    export LIBS="-lm"
    CMAKE_EXE_LINKER_FLAGS=""
    export LD_LIBRARY_PATH_64="/opt/solarisstudio/lib/v9"
else
    echo " *** TESTING SCRIPT ERROR ***"
    echo "   - Unknown compiler specified: $TEST_COMPILER"
    exit 1
fi

# See Document CGNS_TestingSpecs.docx in the CGNS source git repository

#AUTOCONF
WITH_FORTRAN=""
ENABLE_PARALLEL=""
WITH_HDF5=""
ENABLE_64BIT=""
ENABLE_LEGACY=""
ENABLE_SCOPE=""
ENABLE_LFS=""
ENABLE_DEBUG=""
ENABLE_SZIP=""
#CMAKE
CGNS_ENABLE_64BIT="-D CGNS_ENABLE_64BIT:BOOL=OFF"
CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=OFF"
CGNS_ENABLE_HDF5="-D CGNS_ENABLE_HDF5:BOOL=OFF"
CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=OFF"
CGNS_ENABLE_PARALLEL="-D CGNS_ENABLE_PARALLEL:BOOL=OFF"
CGNS_ENABLE_SZIP="OFF"

HDF_DIR="/mnt/scr1/pre-release/hdf5/vdev/$UNAME$DASH$TEST_COMPILER" 

#Check to make sure directory exists
if [ ! -d "$HDF_DIR" ]; then
    echo " *** TESTING SCRIPT ERROR ***"
    echo "   - HDF5 directory does not exists: $HDF_DIR"
    exit 1
fi

#export HDF5_ROOT="$HDF_DIR"
#export HDF5_DIR="$HDF_DIR"
#export CMAKE_PREFIX_PATH="$HDF_DIR"

TEST_SZIP=`grep -iq "szip" $HDF_DIR/bin/h5*cc;echo $?`
if [[ $TEST_SZIP == 0 ]]; then
    SZIP=`cat $HDF_DIR/bin/h5*cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/'`
    ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
    CGNS_ENABLE_SZIP="ON -D SZIP_LIBRARY:PATH=$SZIP/libsz.a"
fi

if [[ $TEST_NO == 1 ]]; then
    WITH_FORTRAN="--with-fortran=yes"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=ON"
    ENABLE_64BIT="--enable-64bit"
    CGNS_ENABLE_64BIT="-D CGNS_ENABLE_64BIT:BOOL=ON"
    ENABLE_LEGACY="--enable-legacy"
    ENABLE_LFS="--enable-lfs"
    CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=ON"
    ENABLE_DEBUG="--enable-debug"
elif [[ $TEST_NO == 2 ]]; then
    WITH_FORTRAN="--with-fortran=yes"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=ON"
    ENABLE_DEBUG="--enable-debug"
elif [[ $TEST_NO == 3 ]]; then
    WITH_FORTRAN="--with-fortran=no"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=OFF"
    WITH_HDF5="$ENABLE_SZIP --with-zlib --with-hdf5=$HDF_DIR"
    ENABLE_64BIT="--enable-64bit"
    CGNS_ENABLE_64BIT="-D CGNS_ENABLE_64BIT:BOOL=ON"
    ENABLE_LEGACY="--enable-legacy"
    ENABLE_LFS="--enable-lfs"
    CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=ON"
    ENABLE_DEBUG="--enable-debug"
    CGNS_ENABLE_HDF5="-D CGNS_ENABLE_HDF5:BOOL=ON -D CMAKE_PREFIX_PATH=$HDF_DIR -D HDF5_NEED_ZLIB:BOOL=ON -D HDF5_NEED_SZIP:BOOL=$CGNS_ENABLE_SZIP"
elif [[ $TEST_NO == 4 ]]; then
    WITH_FORTRAN="--with-fortran=yes"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=ON"
    WITH_HDF5="$ENABLE_SZIP --with-zlib --with-hdf5=$HDF_DIR"
    ENABLE_LFS="--enable-lfs"
    CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=ON"
    CGNS_ENABLE_HDF5="-D CGNS_ENABLE_HDF5:BOOL=ON -D CMAKE_PREFIX_PATH=$HDF_DIR -D HDF5_NEED_ZLIB:BOOL=ON -D HDF5_NEED_SZIP:BOOL=$CGNS_ENABLE_SZIP"
elif [[ $TEST_NO == 5 ]]; then
    WITH_FORTRAN="--with-fortran=yes"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=ON"
    WITH_HDF5="$ENABLE_SZIP --with-zlib --with-hdf5=$HDF_DIR"
    ENABLE_PARALLEL="--with-mpi=$MPI --enable-parallel"
    ENABLE_64BIT="--enable-64bit"
    CGNS_ENABLE_64BIT="-D CGNS_ENABLE_64BIT:BOOL=ON"
    ENABLE_LFS="--enable-lfs"
    CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=ON"
    ENABLE_DEBUG="--enable-debug"
    CGNS_ENABLE_HDF5="-D CGNS_ENABLE_HDF5:BOOL=ON -D CMAKE_PREFIX_PATH=$HDF_DIR -D HDF5_NEED_ZLIB:BOOL=ON -D HDF5_NEED_SZIP:BOOL=$CGNS_ENABLE_SZIP"
    echo "$CGNS_ENABLE_HDF5"
    CGNS_ENABLE_PARALLEL="-D CGNS_ENABLE_PARALLEL:BOOL=ON -D HDF5_NEED_MPI:BOOL=ON"
elif [[ $TEST_NO == 6 ]]; then
    WITH_FORTRAN="--with-fortran=yes"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=ON"
    ENABLE_PARALLEL="--with-mpi=$MPI --enable-parallel"
    WITH_HDF5="$ENABLE_SZIP --with-zlib --with-hdf5=$HDF_DIR"
    ENABLE_LEGACY="--enable-legacy"
    CGNS_ENABLE_HDF5="-D CGNS_ENABLE_HDF5:BOOL=ON -D CMAKE_PREFIX_PATH=$HDF_DIR -D HDF5_NEED_ZLIB:BOOL=ON -D HDF5_NEED_SZIP:BOOL=$CGNS_ENABLE_SZIP"
    CGNS_ENABLE_PARALLEL="-D CGNS_ENABLE_PARALLEL:BOOL=ON -D HDF5_NEED_MPI:BOOL=ON"
else
    echo " *** TESTING SCRIPT ERROR ***"
    echo "   - Unknown test id specified: $TEST_NO"
    exit 2
fi

do_test=1 # default is to perform the tests

#if [[ $UNAME ==  "jam" ]]; then

#    if [[ $TEST_NO == 1 ]]; then
#	export FLIBS="-Wl,--no-as-needed -ldl"
#	export LIBS="-Wl,--no-as-needed -ldl"
#	ENABLE_64BIT="--enable-64bit"
#jam intel, serial
#    elif [[ $TEST_NO == 2 ]]; then
#	export FLIBS=""
#	export LIBS=""
      #ENABLE_64BIT="--enable-64bit"
#jam pgi, serial
#    elif [[ $TEST_NO == 3 ]]; then
#	export FLIBS="-Wl,--no-as-needed -ldl"
#	export LIBS="-Wl,--no-as-needed -ldl"
      #ENABLE_64BIT="--enable-64bit"
#jam gnu 5.2, serial, -fdefault-real-8 -fdefault-integer-8
#  elif [[ $TEST_NO == 4 ]]; then
#    export CC="/usr/hdf/bin/gcc52/gcc"
#    export FC="/usr/hdf/bin/gcc52/gfortran"
#    export FFLAGS="-fdefault-real-8 -fdefault-integer-8"
#    export FLIBS="-Wl,--no-as-needed -ldl"
#    export LIBS="-Wl,--no-as-needed -ldl"
#    ENABLE_64BIT="--enable-64bit"
#    elif [[ $TEST_NO == 4 ]]; then
#	export FLIBS="-Wl,--no-as-needed -ldl"
#	export LIBS="-Wl,--no-as-needed -ldl"
#	ENABLE_64BIT="--enable-64bit"
#	ENABLE_PARALLEL="--enable-parallel"
#	WITH_MPI="--with-mpi=$MPI"
#    else
#	do_test=0
#    fi
if [[ $do_test != 0 ]]; then  

    mkdir test.$TEST_NO
    cd test.$TEST_NO
#    if [[ $HDF_DIR == 0 ]]; then
#	mkdir trunk; cd trunk
#	HDF_DIR="$PWD/hdf5"
#	/home/brtnfld/hdf5/trunk/configure $ENABLE_PARALLEL --disable-fortran --disable-hl; $make_bin -j 8 > result.txt 2>&1; $make_bin install
#	cd ..
#    fi
    #git clone https://github.com/CGNS/CGNS.git
    git clone $BASEDIR/current/CGNS
    
    if [[ $? != 0 ]]; then
	echo " *** TESTING SCRIPT ERROR ***"
	echo "   - FAILED COMMAND: git clone $BASEDIR/current/CGNS"
	exit 1
    fi

    cd CGNS/src
    ./configure \
	$WITH_HDF5 $WITH_FORTRAN $ENABLE_PARALLEL $ENABLE_64BIT $ENABLE_LFS \
 	$ENABLE_LEGACY $ENABLE_SCOPE $ENABLE_LFS $ENABLE_SZIP $ENABLE_DEBUG \
        --prefix=$PWD/cgns_build \
	$SHARED_STATUS \
	--disable-cgnstools \
	--disable-x

    status=$?
    if [[ $status != 0 ]]; then
	echo "CGNS CONFIGURE #FAILED"
	exit $status
    fi
    $make_bin
    status=$?
    if [[ $status != 0 ]]; then
	echo "CGNS BUILD #FAILED"
	exit $status
    fi
    $make_bin install
    status=$?
    if [[ $status != 0 ]]; then
	echo "CGNS INSTALL #FAILED"
	exit $status
    fi
    $make_bin test &> ../../../results.$TEST_NO.txt
    status=$?
    if [[ $status != 0 ]]; then
	echo "CGNS TESTING #FAILED"
	exit $status
    fi
    
    cd ../../../
fi

#rm -fr test.$TEST_NO

#tail -n 100 results.*
do_test=1
CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=OFF"
if [ -d "test.$TEST_NO" ]; then
    cd test.$TEST_NO
    
    if [[ $do_test != 0 ]]; then
        # git clone https://github.com/CGNS/CGNS.git CGNS_SRC
	git clone $BASEDIR/current/CGNS CGNS_SRC
	if [[ $? != 0 ]]; then
	    echo " *** TESTING SCRIPT ERROR ***"
	    echo "   - FAILED COMMAND: git clone $BASEDIR/current/CGNS CGNS_SRC"
	    exit 1
	fi
	CGNS="$PWD/CGNS_SRC"

	mkdir CGNS_build
	cd CGNS_build
	set -x $cmake_bin \
	    -D CMAKE_C_COMPILER:PATH=$CC \
	    -D CMAKE_C_FLAGS:STRING="$CFLAGS" \
	    -D CMAKE_Fortran_FLAGS:STRING="$FCFLAGS" \
	    -D CMAKE_BUILD_TYPE:STRING="Debug" \
	    -D CMAKE_Fortran_COMPILER:PATH=$FC \
	    $CGNS_SHARED_STATUS \
	    -D CGNS_BUILD_SHARED:BOOL=OFF -D CGNS_USE_SHARED:BOOL=OFF \
	    -D CMAKE_STATIC_LINKER_FLAGS:STRING="" \
	    -D CGNS_ENABLE_TESTS:BOOL=ON \
	    -D CGNS_BUILD_CGNSTOOLS:BOOL=OFF \
	    -D CGNS_ENABLE_SCOPING:BOOL=OFF \
	    -D CMAKE_INSTALL_PREFIX:PATH="./" \
	    -D CMAKE_EXE_LINKER_FLAGS:STRING="$CMAKE_EXE_LINKER_FLAGS" $CGNS_ENABLE_PARALLEL \
             $CGNS_ENABLE_LFS $CGNS_ENABLE_HDF5 $CGNS_ENABLE_FORTRAN $CGNS_ENABLE_64BIT $CGNS

	$cmake_bin \
	    -D CMAKE_C_COMPILER:PATH=$CC \
	    -D CMAKE_C_FLAGS:STRING="$CFLAGS" \
	    -D CMAKE_Fortran_FLAGS:STRING="$FCFLAGS" \
	    -D CMAKE_BUILD_TYPE:STRING="Debug" \
	    -D CMAKE_Fortran_COMPILER:PATH=$FC \
	    $CGNS_SHARED_STATUS \
	    -D CMAKE_STATIC_LINKER_FLAGS:STRING="" \
	    -D CGNS_ENABLE_TESTS:BOOL=ON \
	    -D CGNS_BUILD_CGNSTOOLS:BOOL=OFF \
	    -D CGNS_ENABLE_SCOPING:BOOL=OFF \
	    -D CMAKE_INSTALL_PREFIX:PATH="./" \
	    -D CMAKE_EXE_LINKER_FLAGS:STRING="$CMAKE_EXE_LINKER_FLAGS" $CGNS_ENABLE_PARALLEL \
             $CGNS_ENABLE_LFS $CGNS_ENABLE_HDF5 $CGNS_ENABLE_FORTRAN $CGNS_ENABLE_64BIT $CGNS 
	    
        status=$?
        if [[ $status != 0 ]]; then
	   echo "CGNS CONFIGURE #FAILED"
	   exit $status
        fi
	make
	status=$?
	if [[ $status != 0 ]]; then
	    echo "CGNS BUILD #FAILED"
	    exit $status
	fi
	make test
	status=$?
	if [[ $status != 0 ]]; then
	    echo "CGNS TESTING #FAILED"
	    exit $status
	fi
    fi
fi
exit 0

#     -D MPIEXEC:STRING=$MPI/mpiexec \
#     -D MPI_C_COMPILER:STRING=$MPI/mpicc \
#     -D MPI_Fortran_COMPILER:STRING=$MPI/mpif90 \
#  -D HDF5_NEED_MPI:BOOL=ON \
