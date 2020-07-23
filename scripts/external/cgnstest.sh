#!/bin/bash

NO_COLOR="\033[0m"
OK_COLOR="\033[32;01m"
WARN_COLOR="\033[33;01m"
ERROR_COLOR="\033[31;01m"
NOTE_COLOR="\033[36;01m"

#
#
# Platypus (64 bit CentOS6)
# Ostrich (64 bit RHEL6-PPC, big-endian)
# Emu (Solaris 11, big-endian)
# OSX1010DEV Mac OS X 10.10.x
# WREN Mac OS X 10.9.x
# KITE Mac OS X 10.8.x
TIMEOUT="timeout 15m"
UNAME="unknown"
if [ -x /usr/bin/uname ]
then
  UNAME=$(/usr/bin/uname -n)
  PROC=$(/usr/bin/uname -p)
  OSTYPE=$(/usr/bin/uname -s)
fi
if [ -x /usr/local/bin/uname ]
then
  UNAME=$(/usr/local/bin/uname -n)
  PROC=$(/usr/local/bin/uname -p)
  OSTYPE=$(/usr/local/bin/uname -s)
fi
if [ -x /bin/uname ]
then
  UNAME=$(/bin/uname -n)
  PROC=$(/bin/uname -p)
  OSTYPE=$(/bin/uname -s)
fi

# set the default C FLAGS
CFLAGS="-g"
FCFLAGS="-g"

BASEDIR=/mnt/scr1/SnapTest/snapshots-cgns

## USED FOR TESING LOCAL COPY
CGNS_SRC=$BASEDIR/current/CGNS
## USED FOR TESTING REPO (DOES NOT WORK ON OLDER MACHINES)
#CGNS_SRC=https://github.com/CGNS/CGNS.git

# set to change to testing a different branch (default is develop)
#BRANCH="master"
#BRANCH="Experimental_X4-X8_hdf5"
BRANCH="CGNS-179"

# lower case OSTYPE
OSTYPE=$(echo "$OSTYPE" | tr '[:upper:]' '[:lower:]')

# Remove the domain name if present
UNAME=$(echo "$UNAME" | sed 's;\..*;;')


# READ COMMAND LINE FOR THE TEST TO RUN
NARGS=$#
TEST_NO=$1
shift
if [[ $NARGS == 2 ]];then
  TEST_COMPILER=$1
  shift
fi

make_bin="make"
make_opt="-j1"
cmake_bin="cmake"

HDF_DIR="0"

# Set odd/even days of the week
day_date="$(date +%-d)"
day=$(( $(date +%-d) % 2 ))

SHARED_STATUS="--disable-shared"
CGNS_SHARED_STATUS="-D CGNS_BUILD_SHARED:BOOL=OFF -D CGNS_USE_SHARED:BOOL=OFF"
if [ $day -eq 0 ]; then #even day tests

   echo "an even day"

else #odd day tests

   SHARED_STATUS="--enable-shared"
   CGNS_SHARED_STATUS="-D CGNS_BUILD_SHARED:BOOL=ON -D CGNS_USE_SHARED:BOOL=ON"
fi

# Check different versions of HDF5 

HDF_VERSION="vdev"
if [ $(( day_date % 7 )) -eq 0 ]; then
  echo "every 7th day"
  HDF_VERSION="v110"
  if [ $(( day_date % 14 )) -eq 0 ]; then
      echo "every 14th day"
      HDF_VERSION="v18"
  fi
fi
TEST_DIR="test.$TEST_NO"

# COMPILER TESTS: LINKED TO THE CORRESPONDING COMPILED HDF5 LIBRARY

export FLIBS="-Wl,--no-as-needed -ldl"
export LIBS="-Wl,--no-as-needed -ldl"
CMAKE_EXE_LINKER_FLAGS='-Wl,--no-as-needed -ldl'

DASH="-"
HDF_DIR="/mnt/scr1/pre-release/hdf5/$HDF_VERSION/$UNAME$DASH$TEST_COMPILER" # default, but changed in "" case below.

if [[ $TEST_COMPILER == "" ]]; then # System default compiler, exclude dash in the path name

    DASH=""
    HDF_DIR="/mnt/scr1/pre-release/hdf5/$HDF_VERSION/$UNAME$DASH$TEST_COMPILER"

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
	    	  
	    export DYLD_LIBRARY_PATH="$PWD/$TEST_DIR/CGNS/src/lib" 
	    export LDFLAGS="$HDF_DIR/lib/libhdf5.dylib"
            TIMEOUT=""
	    # CURRENTLY DOES NOT SUPPORT SHARED BUILDS ON MAC, CGNS-66
	    SHARED_STATUS="--disable-shared"
	    CGNS_SHARED_STATUS="-D CGNS_BUILD_SHARED:BOOL=OFF -D CGNS_USE_SHARED:BOOL=OFF"
	fi
    fi

elif [[ $TEST_COMPILER == gcc* ]]; then

    if [[ $SHARED_STATUS == "--enable-shared" ]]; then 
	FCFLAGS="$FCFLAGS -fPIC"
	CFLAGS="$CFLAGS -fPIC"
    fi

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
        TIMEOUT=""
    fi

elif [[ $TEST_COMPILER == "intel" ]]; then

    if [[ $OSTYPE == "darwin"* ]];then
       HOSTNAME=$(hostname -s)
       if [[ $HOSTNAME == "osx1011test" ]];then
          source /opt/intel/compilers_and_libraries_2016.2.146/mac/bin/compilervars.sh -arch intel64 -platform mac
       fi
       # CURRENTLY DOES NOT SUPPORT SHARED BUILDS ON MAC, CGNS-66
       SHARED_STATUS="--disable-shared"
       TIMEOUT=""
    fi

    export CC="icc"
    export FC="ifort"
    export FLIBS=""
    export LIBS=""
    CMAKE_EXE_LINKER_FLAGS=""
    if [[ $SHARED_STATUS == "--enable-shared" ]]; then 
	FCFLAGS="$FCFLAGS -fPIC"
	CFLAGS="$CFLAGS -fPIC"
        if [[ $OSTYPE == "darwin"* ]];then # needed for mac
	  export DYLD_LIBRARY_PATH="$PWD/$TEST_DIR/CGNS/src/lib" 
	  export LDFLAGS="$HDF_DIR/lib/libhdf5.dylib"
          TIMEOUT=""
        fi
    fi
elif [[ $TEST_COMPILER == "pgi" ]]; then
    export CC="pgcc"
    export FC="pgf90"
##    export LIBS="-ldl"
elif [[ $TEST_COMPILER == "nag" ]]; then
    export CC="gcc"
    export FC="nagfor"
    export FLIBS="-ldl"
    if [[ $SHARED_STATUS == "--enable-shared" ]]; then
        FCFLAGS="$FCFLAGS -PIC"
    fi
    CMAKE_EXE_LINKER_FLAGS='-ldl'
elif [[ $TEST_COMPILER == "intel-nag" ]]; then
    export CC="icc"
    export FC="nagfor"
    export LIBS="-ldl "
    export FLIBS="-ldl"
    CMAKE_EXE_LINKER_FLAGS='-ldl'
elif [[ $TEST_COMPILER == "pp"* || $TEST_COMPILER == "openmpi"* ]]; then
    HDF_DIR="/mnt/scr1/pre-release/hdf5/$HDF_VERSION/$UNAME$DASH$TEST_COMPILER"
    export CC="mpicc"
    export FC="mpif90"
    if [[ $SHARED_STATUS == "--enable-shared" ]]; then 
	FCFLAGS="$FCFLAGS -fPIC"
	CFLAGS="$CFLAGS -fPIC"
    fi
elif [[ $TEST_COMPILER == "xl" ]]; then
    export CC="/opt/xl/xlf15.1_xlc13.1/xlc"
    export FC="/opt/xl/xlf15.1_xlc13.1/xlf"
    export FLIBS=""
    export LIBS=""
    CMAKE_EXE_LINKER_FLAGS=""
elif [[ $TEST_COMPILER == "xl64" ]]; then
    export CC="/opt/xl/xlf15.1_xlc13.1/xlc"
    export FC="/opt/xl/xlf15.1_xlc13.1/xlf"
    FCFLAGS="-q64 $FCFLAGS"
    CFLAGS="-q64 $CFLAGS"
    export FLIBS=""
    export LIBS=""
    CMAKE_EXE_LINKER_FLAGS=""
elif [[ $TEST_COMPILER == "emu64" ]]; then
    make_bin="gmake"
    make_opt=""
    export CC="cc"
    export FC="f90"
    FCFLAGS="$FCFLAGS -O2 -m64"
    CFLAGS="$CFLAGS -O2 -m64"
    export FLIBS="-lm"
    export LIBS="-lm"
    export SYSCFLAGS="-m64"
    CMAKE_EXE_LINKER_FLAGS=""
    export LD_LIBRARY_PATH_64="/opt/solarisstudio/lib/v9"
else
    echo " *** TESTING SCRIPT ERROR ***"
    echo "   - Unknown compiler specified: $TEST_COMPILER"
    exit 1
fi

export FCFLAGS="$FCFLAGS"
export CFLAGS="$CFLAGS"

#Check to make sure directory exists
if [ ! -d "$HDF_DIR" ]; then
    echo " *** TESTING SCRIPT ERROR ***"
    echo "   - HDF5 directory does not exists: $HDF_DIR"
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
CGNS_ENABLE_SCOPING="-D CGNS_ENABLE_SCOPING:BOOL=OFF"
CGNS_ENABLE_SZIP="OFF"

#export HDF5_ROOT="$HDF_DIR"
#export HDF5_DIR="$HDF_DIR"
#export CMAKE_PREFIX_PATH="$HDF_DIR"

if [[ "$H5CC" = "" ]]; then 
    H5CC="$HDF_DIR"/bin/h5*cc
fi

#TEST_SZIP=$(grep -iq "szip" $HDF_DIR/bin/h5*cc;echo $?)
TEST_SZIP=$(grep -iq "szip" $H5CC;echo $?)
if [[ $TEST_SZIP == 0 ]]; then
    SZIP=$(cat $H5CC | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/')
#    SZIP=$(cat $HDF_DIR/bin/h5*cc | grep "H5BLD_LDFLAGS=" | sed -e 's/.*H5BLD_LDFLAGS=" -L\(.*\) ".*/\1/')
#    ENABLE_SZIP="--with-szip=$SZIP/libsz.a"
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SZIP"
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
    WITH_HDF5="--with-hdf5=$HDF_DIR"
    ENABLE_64BIT="--enable-64bit"
    CGNS_ENABLE_64BIT="-D CGNS_ENABLE_64BIT:BOOL=ON"
    ENABLE_LEGACY="--enable-legacy"
    ENABLE_LFS="--enable-lfs"
    CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=ON"
    ENABLE_DEBUG="--enable-debug"
    CGNS_ENABLE_HDF5="-D CGNS_ENABLE_HDF5:BOOL=ON -D CMAKE_PREFIX_PATH=$HDF_DIR -D HDF5_NEED_ZLIB:BOOL=ON -D HDF5_NEED_SZIP:BOOL=$CGNS_ENABLE_SZIP"
    ENABLE_SCOPE="--enable-scope"
    CGNS_ENABLE_SCOPING="-D CGNS_ENABLE_SCOPING:BOOL=ON"
elif [[ $TEST_NO == 4 ]]; then
    WITH_FORTRAN="--with-fortran=yes"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=ON"
    WITH_HDF5="--with-hdf5=$HDF_DIR"
    ENABLE_LFS="--enable-lfs"
    CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=ON"
    CGNS_ENABLE_HDF5="-D CGNS_ENABLE_HDF5:BOOL=ON -D CMAKE_PREFIX_PATH=$HDF_DIR -D HDF5_NEED_ZLIB:BOOL=ON -D HDF5_NEED_SZIP:BOOL=$CGNS_ENABLE_SZIP"
elif [[ $TEST_NO == 5 ]]; then
    WITH_FORTRAN="--with-fortran=yes"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=ON"
    WITH_HDF5="--with-hdf5=$HDF_DIR"
    ENABLE_PARALLEL="--enable-parallel"
    ENABLE_64BIT="--enable-64bit"
    CGNS_ENABLE_64BIT="-D CGNS_ENABLE_64BIT:BOOL=ON"
    ENABLE_LFS="--enable-lfs"
    CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=ON"
    ENABLE_DEBUG="--enable-debug"
    CGNS_ENABLE_HDF5="-D CGNS_ENABLE_HDF5:BOOL=ON -D CMAKE_PREFIX_PATH=$HDF_DIR -D HDF5_NEED_ZLIB:BOOL=ON -D HDF5_NEED_SZIP:BOOL=$CGNS_ENABLE_SZIP"
    CGNS_ENABLE_PARALLEL="-D CGNS_ENABLE_PARALLEL:BOOL=ON -D HDF5_NEED_MPI:BOOL=ON"
elif [[ $TEST_NO == 6 ]]; then
    WITH_FORTRAN="--with-fortran=yes"
    CGNS_ENABLE_FORTRAN="-D CGNS_ENABLE_FORTRAN:BOOL=ON"
    ENABLE_PARALLEL="--enable-parallel"
    WITH_HDF5="--with-hdf5=$HDF_DIR"
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
#    else
#	do_test=0
#    fi

autotools_status=0
if [[ $do_test != 0 ]]; then  

    if [ -d "$TEST_DIR" ]; then
        rm -fr "$TEST_DIR"
    fi

    mkdir "$TEST_DIR"
    cd "$TEST_DIR" || exit
#    if [[ $HDF_DIR == 0 ]]; then
#	mkdir trunk; cd trunk
#	HDF_DIR="$PWD/hdf5"
#	/home/brtnfld/hdf5/trunk/configure $ENABLE_PARALLEL --disable-fortran --disable-hl; $make_bin -j 8 > result.txt 2>&1; $make_bin install
#	cd ..
#    fi

    if ! git clone $CGNS_SRC; then
	echo " *** TESTING SCRIPT ERROR ***"
	echo "   - FAILED COMMAND: git clone $CGNS_SRC"
	exit 1
    fi

    if [[ "$BRANCH" != "" ]]; then
        printf "%b ********************************************************************************* \n" "$WARN_COLOR"
        printf "  ____  ____      _    _   _  ____ _   _   _____ _____ ____ _____ ___ _   _  ____\n"
        printf " | __ )|  _ \    / \  | \ | |/ ___| | | | |_   _| ____/ ___|_   _|_ _| \ | |/ ___|\n"
        printf " |  _ \| |_) |  / _ \ |  \| | |   | |_| |   | | |  _| \___ \ | |  | ||  \| | |  _\n"
        printf " | |_) |  _ <  / ___ \| |\  | |___|  _  |   | | | |___ ___) || |  | || |\  | |_| |\n"
        printf " |____/|_| \_\/_/   \_\_| \_|\____|_| |_|   |_| |_____|____/ |_| |___|_| \_|\____|\n\n"
        printf " *********************************************************************************\n"
        printf " TESTING BRANCH ... %b %b \n" "$BRANCH" "$NO_COLOR"
        echo ""
        cd CGNS || exit
        git checkout -b "$BRANCH" origin/"$BRANCH"
        cd ..
    fi

    CONFIG_CMD="./configure"
    CONFIG_CMD="$CONFIG_CMD $WITH_HDF5 $WITH_FORTRAN $ENABLE_PARALLEL $ENABLE_64BIT"
    CONFIG_CMD="$CONFIG_CMD $ENABLE_LEGACY $ENABLE_SCOPE $ENABLE_LFS $ENABLE_DEBUG"
    CONFIG_CMD="$CONFIG_CMD --prefix=$PWD/cgns_build $SHARED_STATUS --disable-cgnstools"
    cd CGNS/src || exit
    
    printf "%b" "$NOTE_COLOR"
    printf "       ___   __  ____________  __________  ____  __   _____\n"
    printf "      /   | / / / /_  __/ __ \/_  __/ __ \/ __ \/ /  / ___/\n"
    printf "     / /| |/ / / / / / / / / / / / / / / / / / / /   \__ \ \n"
    printf "    / ___ / /_/ / / / / /_/ / / / / /_/ / /_/ / /______/ / \n"
    printf "   /_/  |_\____/ /_/  \____/ /_/  \____/\____/_____/____/  \n\n"

    printf "$CONFIG_CMD%b\n\n" "$NO_COLOR"

    if ! $CONFIG_CMD ; then
	echo "CGNS CONFIGURE #FAILED"
	autotools_status=$status
    fi
    if ! $make_bin; then
	echo "CGNS BUILD #FAILED"
	autotools_status=$status
    fi
    if ! $make_bin install; then
	echo "CGNS INSTALL #FAILED"
	autotools_status=$status
    fi

	if ! $TIMEOUT $make_bin test; then
	    echo "CGNS CMAKE TESTING #FAILED"
	    cmake_status=$?
	fi

    $TIMEOUT $make_bin test &> ../../../results.$TEST_NO.txt

    timeout_status=$?
    if [[ $timeout_status -eq 124 ]]; then
        echo "CGNS AUTOOLS TESTING #FAILED"
        autotools_status=$timeout_status
    fi
        
    cat ../../../results.$TEST_NO.txt
    status=$?
    if [[ $status != 0 ]]; then
	printf "%bCGNS TESTING #FAILED%b" "$ERROR_COLOR" "$NO_COLOR"
	autotools_status=$status
    fi

    ## date format
    NOW=$(date +"%F")
    FILE="/mnt/scr1/SnapTest/benchmarks/cgns/$NOW"
    ## Save timing data
    if [ -f tests/CGNS_timing.txt ]; then
	echo "# $HDF_VERSION $TEST_NO" >> "$FILE"
	cat tests/CGNS_timing.txt >> "$FILE"
	echo "" >> "$FILE"
    fi

    cd ../../../
fi

do_test=1

# ******* COMMENTED OUT FAILED TESTS ******
# shared test fail with Fortran because cmake tries to remove a .mod file that is not there.
# Building C object src/CMakeFiles/cgns_shared.dir/cgnslib.c.o
# Fatal Error: Can't delete temporary module file 'cgns.mod0': No such file or directory
# gmake[2]: *** [src/CMakeFiles/cgns_shared.dir/cgns_f.F90.o] Error 1

if [[ $SHARED_STATUS == "--enable-shared" && $WITH_FORTRAN == "--with-fortran=yes" ]]; then
    if [[ $UNAME == "ostrich" || $UNAME == "kituo" || $UNAME == "mayll" || $UNAME == "moohan" || $UNAME == "platypus" ]];then
	do_test=0
        printf "%b" "$WARN_COLOR"
        echo "WARNING: Disabling CMake testing for ostrich, kituo, moohan and platypus"
        echo "for shared tests and Fortran because cmake tries to remove a .mod "
        echo "file that is not there."
        printf "%b" "$NO_COLOR"
    fi
fi

cmake_status=0
CGNS_ENABLE_LFS="-D CGNS_ENABLE_LFS:BOOL=OFF"
printf "%b" "$WARN_COLOR"
echo "WARNING: Disabling LFS, CGNS_ENABLE_LFS='-D CGNS_ENABLE_LFS:BOOL=OFF'"
printf "%b" "$NO_COLOR"

if [ -d "$TEST_DIR" ]; then
    cd "$TEST_DIR" || exit
    
    if [[ $do_test != 0 ]]; then

        printf "%b" "$NOTE_COLOR"
        echo "       ________  ______    __ __ ______"
        echo "      / ____/  |/  /   |  / //_// ____/"
        echo "     / /   / /|_/ / /| | / ,<  / __/   "
        echo "    / /___/ /  / / ___ |/ /| |/ /___   "
        echo "    \____/_/  /_/_/  |_/_/ |_/_____/   "
        echo ""
        printf "%b" "$NO_COLOR"

	if ! git clone $CGNS_SRC CGNS_SRC; then
	    echo " *** TESTING SCRIPT ERROR ***"
	    echo "   - FAILED COMMAND: git clone $CGNS_SRC CGNS_SRC"
	    exit 1
	fi
	if [[ "$BRANCH" != "" ]]; then
          cd CGNS_SRC || exit
          git checkout "$BRANCH"
          cd ..
        fi

	CGNS="$PWD/CGNS_SRC"

	mkdir CGNS_build
	cd CGNS_build || exit
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
	    -D CMAKE_INSTALL_PREFIX:PATH="./" \
	    -D CMAKE_EXE_LINKER_FLAGS:STRING="$CMAKE_EXE_LINKER_FLAGS" $CGNS_ENABLE_PARALLEL \
             $CGNS_ENABLE_LFS $CGNS_ENABLE_HDF5 $CGNS_ENABLE_FORTRAN $CGNS_ENABLE_64BIT \
             $CGNS_ENABLE_SCOPING \
             "$CGNS"

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
	    -D CMAKE_INSTALL_PREFIX:PATH="./" \
	    -D CMAKE_EXE_LINKER_FLAGS:STRING="$CMAKE_EXE_LINKER_FLAGS" $CGNS_ENABLE_PARALLEL \
             $CGNS_ENABLE_LFS $CGNS_ENABLE_HDF5 $CGNS_ENABLE_FORTRAN $CGNS_ENABLE_64BIT \
             $CGNS_ENABLE_SCOPING \
             "$CGNS"
	    
        status=$?
        if [[ $status != 0 ]]; then
	   echo "CGNS CMAKE CONFIGURE #FAILED"
	   cmake_status=$status
        fi
	
	if ! $make_bin $make_opt; then
	    echo "CGNS CMAKE BUILD #FAILED"
	    cmake_status=$?
	fi
	$TIMEOUT $make_bin test
        timeout_status=$?
        if [[ $timeout_status -eq 124 ]]; then
            echo "CGNS CMAKE TESTING #FAILED"
	    cmake_status=$timeout_status
        fi
    fi
fi

if [[ $autotools_status != 0 || $cmake_status != 0 ]]; then
    printf "%bCGNS #FAILED%b" "$ERROR_COLOR" "$NO_COLOR"
    exit 1
fi
exit 0

#     -D MPIEXEC:STRING=$MPI/mpiexec \
#     -D MPI_C_COMPILER:STRING=$MPI/mpicc \
#     -D MPI_Fortran_COMPILER:STRING=$MPI/mpif90 \
#  -D HDF5_NEED_MPI:BOOL=ON \
