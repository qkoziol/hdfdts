#!/bin/bash -l

NO_COLOR="\033[0m"
OK_COLOR="\033[32;01m"
WARN_COLOR="\033[33;01m"
ERROR_COLOR="\033[31;01m"
NOTICE_COLOR="\033[36;01m"
CLEAR="\e[0m"
UNDERLINE="\e[4m"

IN_DIR=`pwd`

#Defaults

HDF5_VER=""
KNL="false"
ALL_TESTS="false"
HDF5_BRANCH=""
SKIP_TESTS=""
BUILD_TYPE="Release"

# READ COMMAND LINE FOR THE TEST TO RUN
CTEST_OPTS=""
ACCOUNT=""
POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    -v|--version)
    HDF5_VER="$2"
    shift # past argument
    shift # past value
    BRANCH_VERSTR=`echo $HDF5_VER | sed -e s/\\\./_/g`
    HDF5_BRANCH=hdf5_$BRANCH_VERSTR
    ;;
    -a)
    CTEST_OPTS="LOCAL_BATCH_SCRIPT_ARGS=$2,$CTEST_OPTS"
    shift # past argument
    shift # past value
    ;;
    -p)
    ACCOUNT=$2
    shift # past argument
    shift # past value
    ;;
    -knl)
    KNL="true"
    CTEST_OPTS="KNL=$KNL,$CTEST_OPTS"
    shift # past argument
    ;;
    -alltests)
    ALL_TESTS="true"
    shift # past argument
    ;;
    -debug)
    BUILD_TYPE="Debug"
    shift # past argument
    ;;
    -h*|--h*)
    printf "USAGE: nnsa.sh [${UNDERLINE}OPTION${CLEAR}]...

    where:

       -v,--version x.x.x    hdf5 version to test 
                             [default is develop; do not specify version]
       -knl                  compile for KNL [default: no]
       -a --account [${UNDERLINE}id${CLEAR}]     specify job account on the batch command line
       -p [${UNDERLINE}id${CLEAR}]               specify job account in the batch script
       -alltests             don't skip any problematic tests [default: skip those tests]
       -debug                build type is debug mode
       -h,--help             show this help text \n"
    exit 0
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

HOSTNAME=`hostname -d`
if [[ $HOSTNAME == *alcf* ]]; then
  HOSTNAME=`hostname`
fi

# Get the host name
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

# Remove the domain name if present
UNAME=`echo $UNAME | sed 's;\..*;;'`

#save this directory for copying binary to $BASEDIR/hdf5_install
BASEDIR=`pwd`

mkdir -p CI; cd CI
rm -rf HDF5-* build hdf5.log* Failed* slurm-*.out
sleep 1

HDF5_SRC_UPDATED=""
HDF5_CLONED=""
HDF5_SRCDIR=""
HDF5_BRANCH_NAME='N/A'
LAST_COMMIT_PREV=0
LAST_COMMIT=1
#GIT_DATE="yesterday"
GIT_DATE="3 days ago"
if [ -d hdf5* ]; then
  HDF5_SRCDIR=`ls -d hdf5*`
  cd ${HDF5_SRCDIR}
  if [[ $HDF5_BRANCH == "" ]]; then
    git checkout develop
  else
    git checkout $HDF5_BRANCH
  fi
  LAST_COMMIT_PREV=`git log -p -1 --pretty=format:"%H"`
  if ! git pull; then
    cd ..
    rm -rf ${HDF5_SRCDIR}
    if [[ $HDF5_BRANCH == "" ]]; then
      git clone https://git@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git hdf5
    else
      git clone https://git@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git -b $HDF5_BRANCH hdf5
      HDF5_BRANCH_NAME=$HDF5_BRANCH
    fi
    HDF5_CLONED="true"
    cd hdf5
  fi
  sleep 5

  HDF5_VER=`bin/h5vers`
  LAST_COMMIT=`git log -p -1 --pretty=format:"%H"`
  if [[ $(git log --since="$GIT_DATE") ]]; then
    HDF5_SRC_UPDATED="true"
  fi
  cd ..
  if [[ "${HDF5_SRC_UPDATED}" == "true" ]]; then
    if [[ $HDF5_CLONED == "" ]] && [[ $LAST_COMMIT_PREV != $LAST_COMMIT ]]; then
      echo "Delete directory $HDF5_SRCDIR and do a fresh clone of HDF5 $HDF5_BRANCH source."
      rm -rf ${HDF5_SRCDIR}
      echo "Test new HDF5 clone."
    fi
  else
    printf "$NOTICE_COLOR\n"
    echo "HDF5 $HDF5_BRANCH source in $HDF5_SRCDIR has not changed since $GIT_DATE. No testing today."
    printf "$NO_COLOR\n"
    exit 0
  fi

fi

if [[ $HDF5_CLONED == "" ]] && [[ $LAST_COMMIT_PREV != $LAST_COMMIT ]]; then
  if [[ $HDF5_BRANCH == "" ]]; then
    git clone https://git@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git hdf5
  else
    git clone https://git@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git -b $HDF5_BRANCH hdf5
    HDF5_BRANCH_NAME=$HDF5_BRANCH
  fi
  cd hdf5
  HDF5_VER=`bin/h5vers`
  cd ..
  mv hdf5 hdf5-$HDF5_VER
fi

# Summary of command line inputs
printf "$NOTICE_COLOR\n"
echo "HDF5_VER.......$HDF5_VER"
echo "MISC OPTIONS...$CTEST_OPTS"
echo "HDF5_BRANCH....$HDF5_BRANCH_NAME"
printf "$NO_COLOR\n"

sleep 1
rm -f CTestScript.cmake HDF5config.cmake HDF5options.cmake
sleep 1
ln -s hdf5-$HDF5_VER/config/cmake/scripts/CTestScript.cmake .
ln -s hdf5-$HDF5_VER/config/cmake/scripts/HDF5config.cmake .
ln -s hdf5-$HDF5_VER/config/cmake/scripts/HDF5options.cmake .

if [[ $UNAME == cori* ]];then

    if [[ $ALL_TESTS == 'false' ]];then
      SKIP_TESTS="-E '"
      SKIP_TESTS=$SKIP_TESTS"MPI_TEST_testphdf5_tldsc"
      SKIP_TESTS=$SKIP_TESTS"'"
    fi
    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/ctestP.sl.in.cmake
    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/knl_ctestP.sl.in.cmake
    perl -i -pe "s/^#SBATCH --nodes=1/#SBATCH -C haswell\n#SBATCH --nodes=1/" hdf5-$HDF5_VER/bin/batch/ctestS.sl.in.cmake
    perl -i -pe "s/^#SBATCH --nodes=1/#SBATCH -C haswell\n#SBATCH --nodes=1/" hdf5-$HDF5_VER/bin/batch/ctestP.sl.in.cmake
    perl -i -pe "s/^#SBATCH -p knl.*/#SBATCH -C knl,quad,cache/" hdf5-$HDF5_VER/bin/batch/knl_ctestS.sl.in.cmake
    perl -i -pe "s/^#SBATCH -p knl.*/#SBATCH --qos=regular\n#SBATCH -C knl,quad,cache/" hdf5-$HDF5_VER/bin/batch/knl_ctestP.sl.in.cmake
    perl -i -pe "s/^#SBATCH -t 00:30:00/#SBATCH -t 1:00:00/" hdf5-$HDF5_VER/bin/batch/knl_ctestP.sl.in.cmake

# Get the curent PrgEnv module setting
    module list &> out
    PRGENV_TYPE=`grep -i PrgEnv out | sed -e 's/.*PrgEnv-\(.*\)\/.*/\1/'`
    module unload cmake

    module load cmake/3.11.4
# unload the current PrgEnv and compiler associated with PrgEnv
    module unload PrgEnv-$PRGENV_TYPE
    module unload $PRGENV_TYPE
# the modules to test
#   the PrgEnv to test
    MASTER_MOD="PrgEnv-intel/6.0.4 PrgEnv-gnu/6.0.4"
    #MASTER_MOD="PrgEnv-intel/6.0.4"
#   the Compiler to switch to:
#    Format: <number of compiler versions to check> <compiler type> <list of compiler versions (modules) ... repeat)
    CC_VER=(2 intel/17.0.3.191 intel/18.0.2.199 2 gcc/7.3.0 gcc/8.2.0)
    #CC_VER=(1 gcc gcc/8.2.0)
    CTEST_OPTS="HPC=sbatch,$CTEST_OPTS"

    _CC=cc
    _FC=ftn
    _CXX=CC

elif [[ $UNAME == mutrino* ]];then

    if [[ $ALL_TESTS == 'false' ]];then
      SKIP_TESTS="-E '"
      SKIP_TESTS=$SKIP_TESTS"MPI_TEST_testphdf5_tldsc"
      SKIP_TESTS=$SKIP_TESTS"'"
    fi
    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/ctestP.sl.in.cmake

# Get the curent PrgEnv module setting
    module list &> out
    PRGENV_TYPE=`grep -i PrgEnv out | sed -e 's/.*PrgEnv-\(.*\)\/.*/\1/'`
    module unload craype-hugepages2M

    module load cmake
# unload the current PrgEnv and compiler associated with PrgEnv
    module unload PrgEnv-$PRGENV_TYPE
    module unload $PRGENV_TYPE
# the modules to test
#   the PrgEnv to test
    MASTER_MOD="PrgEnv-intel/6.0.4 PrgEnv-gnu/6.0.4"
#   the Compiler to switch to:
#    Format: <number of compiler versions to check> <compiler type> <list of compiler versions (modules) ... repeat)
    CC_VER=(2 intel/17.0.4 intel/18.0.2 2 gcc/7.2.0 gcc/8.2.0)
    CTEST_OPTS="HPC=sbatch,$CTEST_OPTS"

    _CC=cc
    _FC=ftn
    _CXX=CC

elif [[ $UNAME == serrano* ]]; then

    if [[ $ALL_TESTS == 'false' ]];then
      SKIP_TESTS="-E '"
      SKIP_TESTS=$SKIP_TESTS"MPI_TEST_testphdf5_selnone"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_tldsc"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_ecdsetw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk3"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_ccchunkw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_actualio"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_t_shapesame"
      SKIP_TESTS=$SKIP_TESTS"'"
    fi
    
    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/ctestP.sl.in.cmake

    echo 'set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DMPIEXEC_EXECUTABLE:STRING=mpirun")' >> hdf5-$HDF5_VER/config/cmake/scripts/HPC/sbatch-HDF5options.cmake
    echo 'set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DMPIEXEC_PREFLAGS:STRING=--mca;io;ompio")' >> hdf5-$HDF5_VER/config/cmake/scripts/HPC/sbatch-HDF5options.cmake

    module purge
    module load cmake

    MASTER_MOD="openmpi-intel/3.0"
    CC_VER=(2 intel/17.0 intel/18.0)
    CTEST_OPTS="HPC=sbatch,$CTEST_OPTS"

    _CC=mpicc
    _FC=mpif90
    _CXX=mpicxx

elif [[ $UNAME == chama* ]]; then

    if [[ $ALL_TESTS == 'false' ]];then

      SKIP_TESTS="-E '"
      SKIP_TESTS=$SKIP_TESTS"MPI_TEST_testphdf5_selnone"
      #SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_tldsc"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_ecdsetw"
      #SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_eidsetw2"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cngrpw-ingrpr"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk1"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk2"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk3"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk4"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cschunkw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_ccchunkw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_actualio"
      #SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_MC_coll_MD_read"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_t_shapesame"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_t_filters_parallel"
      SKIP_TESTS=$SKIP_TESTS"'"
    fi

    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/ctestP.sl.in.cmake

    echo 'set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DMPIEXEC_EXECUTABLE:STRING=mpirun")' >> hdf5-$HDF5_VER/config/cmake/scripts/HPC/sbatch-HDF5options.cmake
    echo 'set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DMPIEXEC_PREFLAGS:STRING=--mca;io;ompio")' >> hdf5-$HDF5_VER/config/cmake/scripts/HPC/sbatch-HDF5options.cmake

    module purge
    module load cmake

    MASTER_MOD="openmpi-intel/4.0"
    CC_VER=(1 intel/18.0)
    CTEST_OPTS="HPC=sbatch,$CTEST_OPTS"

    _CC=mpicc
    _FC=mpif90
    _CXX=mpicxx 

elif [[ $UNAME == eclipse* ]]; then

    module purge
    module load cmake

    MASTER_MOD="intel-mpi/2018"
    CC_VER=(2 intel/16.0 intel/18.0)
    CTEST_OPTS="HPC=sbatch,$CTEST_OPTS"

    _CC=mpicc
    _FC=mpif90
    _CXX=mpicxx

elif [[ $UNAME == quartz* ]]; then

    if [[ $ALL_TESTS == 'false' ]];then
      SKIP_TESTS="-E '"
      SKIP_TESTS=$SKIP_TESTS"MPI_TEST_t_bigio"
      SKIP_TESTS=$SKIP_TESTS"'"
    fi

    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/knl_ctestP.sl.in.cmake

    echo 'set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DMPIEXEC_EXECUTABLE:STRING=mpirun")' >> hdf5-$HDF5_VER/config/cmake/scripts/HPC/sbatch-HDF5options.cmake
    echo 'set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DMPIEXEC_PREFLAGS:STRING=--mca;io;ompio")' >> hdf5-$HDF5_VER/config/cmake/scripts/HPC/sbatch-HDF5options.cmake

    module purge
    module load cmake/3.12.1
    module load intel/16.0.4

    MASTER_MOD="openmpi/4.0.0 openmpi/4.0.0 openmpi/4.0.0"
    CC_VER=(1 intel/16.0.4 2 gcc/7.1.0 gcc/8.1.0 1 clang/3.9.0)
    CTEST_OPTS="HPC=sbatch,$CTEST_OPTS"

    _CC=mpicc
    _FC=mpif90
    _CXX=mpicxx

elif [[ $UNAME == ray* ]]; then

    if [[ $ALL_TESTS == 'false' ]];then
      SKIP_TESTS="-E '"
      SKIP_TESTS=$SKIP_TESTS"MPI_TEST_testphdf5_selnone"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_tldsc"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_ecdsetw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_eidsetw2"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cngrpw-ingrpr"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk1"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk2"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk3"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk4"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cschunkw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_ccchunkw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_actualio"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_MC_coll_MD_read"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_t_shapesame"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_t_filters_parallel"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_H5DIFF-h5diff_80"
      SKIP_TESTS=$SKIP_TESTS"'"
    fi

    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/ray_ctestP.lsf.in.cmake
    
    module purge
    module load cmake/3.12.1
    module load xl/2016.12.02

    MASTER_MOD="spectrum-mpi spectrum-mpi spectrum-mpi"
    CC_VER=(1 xl/2016.12.02 1 gcc/7.3.1 1 clang/coral-2018.08.08)
    CTEST_OPTS="HPC=raybsub,$CTEST_OPTS"

    _CC=mpicc
    _FC=mpif90
    _CXX=mpicxx

elif [[ $UNAME == lassen* ]]; then

    if [[ $ALL_TESTS == 'false' ]];then
      SKIP_TESTS="-E '"
      SKIP_TESTS=$SKIP_TESTS"MPI_TEST_testphdf5_selnone"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_tldsc"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_ecdsetw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk3"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_ccchunkw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_actualio"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_t_shapesame"
      SKIP_TESTS=$SKIP_TESTS"'"
    fi
 
    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/ctestP.lsf.in.cmake

    echo 'set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DMPIEXEC_EXECUTABLE:STRING=mpirun")' >> hdf5-$HDF5_VER/config/cmake/scripts/HPC/bsub-HDF5options.cmake

    perl -i -pe "s/^ctest.*/ctest . -E MPI_TEST_ -C ${BUILD_TYPE} -j 32 -T test >& ctestS.out/" hdf5-$HDF5_VER/bin/batch/ctestS.lsf.in.cmake
    
    module purge
    module load cmake/3.12.1
    module load xl/2019.02.07

    MASTER_MOD="spectrum-mpi spectrum-mpi spectrum-mpi"
    CC_VER=(1 xl/2019.02.07 1 gcc/7.3.1 1 clang/coral-2018.08.08)
    CTEST_OPTS="HPC=bsub,$CTEST_OPTS"

    _CC=mpicc
    _FC=mpif90
    _CXX=mpicxx

fi

# ------------------------
# www.olcf.ornl.gov
# STATUS: ACTIVE
# ------------------------
if [[ $HOSTNAME == summit* ]]; then

#CHECKS
    if [[ $ACCOUNT == '' ]];then
        printf "${ERROR_COLOR}FATAL ERROR: SUMMIT REQUIRES AN ALLOCATION ID TO BE SET \n"
        printf "    Usage: -p <ALLOCATION ID> ${NO_COLOR}\n\n"
        exit 1
    fi

    UNAME=$HOSTNAME
    echo 'set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DMPIEXEC_EXECUTABLE:STRING=jsrun")' >> hdf5-$HDF5_VER/config/cmake/scripts/HPC/bsub-HDF5options.cmake

    if [[ $ALL_TESTS == 'false' ]];then
      SKIP_TESTS="-E '"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_tldsc"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_eidsetw2"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cngrpw-ingrpr"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk1"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk2"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk3"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cchunk4"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_cschunkw"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_testphdf5_MC_coll_MD_read"
      SKIP_TESTS=$SKIP_TESTS"|MPI_TEST_t_filters_parallel"
      SKIP_TESTS=$SKIP_TESTS"'"
    fi

    perl -i -pe "s/^ctest.*/ctest . -R MPI_TEST_ ${SKIP_TESTS} -C ${BUILD_TYPE} -T test >& ctestP.out/" hdf5-$HDF5_VER/bin/batch/ctestP.lsf.in.cmake
    perl -i -pe "s/^ctest.*/ctest . -E MPI_TEST_ -C ${BUILD_TYPE} -j 32 -T test >& ctestS.out/" hdf5-$HDF5_VER/bin/batch/ctestS.lsf.in.cmake

# Custom BSUB commands
    perl -i -pe "s/^#BSUB -G.*/#BSUB -P ${ACCOUNT}/" hdf5-$HDF5_VER/bin/batch/ctestS.lsf.in.cmake
    perl -i -pe "s/^#BSUB -q.*//" hdf5-$HDF5_VER/bin/batch/ctestS.lsf.in.cmake
    perl -i -pe "s/^#BSUB -G.*/#BSUB -P ${ACCOUNT}/" hdf5-$HDF5_VER/bin/batch/ctestP.lsf.in.cmake
    perl -i -pe "s/^#BSUB -q.*//" hdf5-$HDF5_VER/bin/batch/ctestP.lsf.in.cmake

    module load cmake
    module load zlib

    module list xl &>out
    XL_DEFAULT=`grep xl/ out | sed -n 's/^.*\(xl[^ ]*\).*/\1/p'`
    rm -f out

    MASTER_MOD="spectrum-mpi"
    CC_VER=(1 $XL_DEFAULT)

    CTEST_OPTS="HPC=bsub,SITE_OS_NAME=${HOSTNAME},$CTEST_OPTS"

    _CC=mpicc
    _FC=mpif90
    _CXX=mpicxx
fi

# ------------------------
# www.alcf.anl.gov
# STATUS: ACTIVE
# ------------------------

if [[ $HOSTNAME == theta* ]]; then

    #CHECKS
    if [[ $ACCOUNT == '' ]];then
        printf "${ERROR_COLOR}FATAL ERROR: THETA REQUIRES AN ALLOCATION ID TO BE SET \n"
        printf "    Usage: -p <ALLOCATION ID> ${NO_COLOR}\n\n"
        exit 1
    fi
    if [[ $KNL == 'false' ]];then
        printf "${WARN_COLOR}WARNING: THETA REQUIRES KNL OPTION...SETTING OPTION \n"
        printf "    Usage: -knl ${NO_COLOR}\n\n"
        KNL="true"
        CTEST_OPTS="KNL=$KNL,$CTEST_OPTS"
    fi

    UNAME=$HOSTNAME

    module unload darshan
    module unload craype-mic-knl
    module load craype-haswell

    if [[ $ALL_TESTS == 'false' ]];then
      SKIP_TESTS="\"-E "
      SKIP_TESTS=$SKIP_TESTS"MPI_TEST_testphdf5_tldsc"
      SKIP_TESTS=$SKIP_TESTS"\""
      sed -i -e "s/^#SKIPTESTS.*/\nSKIP_TESTS=${SKIP_TESTS}/g" hdf5-$HDF5_VER/bin/batch/ctest.qsub.in.cmake
    fi

    # Select the newest cmake available
    MOD_CMAKE=`module avail cmake 2>&1 >/dev/null | grep 'cmake' | sed -n '${s/.* //; p}' | sed 's/(default)//g'`
    module load $MOD_CMAKE

    # Select the newest PrgEnv available
    MASTER_MOD=`module avail PrgEnv-intel 2>&1 >/dev/null | grep 'intel' | sed -n '${s/.* //; p}' | sed 's/(default)//g'`

    # Select the newest intel compiler available
    MOD_INTEL=`module avail intel/ 2>&1 >/dev/null | grep 'intel' | sed -n '${s/.* //; p}' | sed 's/(default)//g'`
    CC_VER=(1 $MOD_INTEL)

    CTEST_OPTS="HPC=qsub,SITE_OS_NAME=${HOSTNAME},LOCAL_BATCH_SCRIPT_ARGS=${ACCOUNT},$CTEST_OPTS"

    _CC=cc
    _FC=ftn
    _CXX=CC

fi

printf "${NOTICE_COLOR}HOST MACHINE NAME = ${UNAME}\n"
printf "${UNDERLINE}DEFAULT MODULES LOADED${CLEAR}\n"
module list
printf "\n$NO_COLOR"

icnt=-1
for master_mod in $MASTER_MOD; do

  icnt=$(($icnt+1))
  num_CC=${CC_VER[$icnt]} # number of compilers for the  master_mod

  module load $master_mod # Load the master module

  icnt=$(($icnt+1))
  COMPILER_TYPE=`echo ${CC_VER[icnt]} | sed 's/\/.*//'`
  module unload ${COMPILER_TYPE} # unload the general compiler type

  for i in `seq 1 $num_CC`; do # loop over compiler versions
    cc_ver=${CC_VER[$icnt]} # compiler version
    rm -fr build

    module load $cc_ver # load the compiler with version
    module load $master_mod

    export CC=$_CC
    export FC=$_FC
    export CXX=$_CXX

    module list

    printf "$OK_COLOR"
    printf "ctest . -S HDF5config.cmake,SITE_BUILDNAME_SUFFIX=\"$HDF5_VER-$master_mod-$cc_ver\",${CTEST_OPTS}MPI=true,BUILD_GENERATOR=Unix,LOCAL_SUBMIT=true,MODEL=HPC -C ${BUILD_TYPE} -VV -O hdf5.log \n"
    printf "$NO_COLOR"
    timeout 3h ctest . -S HDF5config.cmake,SITE_BUILDNAME_SUFFIX="$HDF5_VER-$master_mod--$cc_ver",${CTEST_OPTS}MPI=true,BUILD_GENERATOR=Unix,LOCAL_SUBMIT=true,MODEL=HPC -C ${BUILD_TYPE} -VV -O hdf5.log

    if [ ! -d $BASEDIR/hdf5_install ]; then
        mkdir $BASEDIR/hdf5_install
    fi
    tar zxf HDF5-*.tar.gz -C $BASEDIR/hdf5_install
    if [[ "$HDF5_BRANCH" != "" ]]; then
        BRANCH_STRING=-$HDF5_BRANCH
    fi
    MODULE_STRING=`echo $master_mod | sed 's?/?-?g'`
    if [ -d $BASEDIR/hdf5_install/$HDF5_VER$BRANCH_STRING/hdf5-$MODULE_STRING$cc_ver ]; then
        rm -rf $BASEDIR/hdf5_install/$HDF5_VER$BRANCH_STRING/hdf5-$MODULE_STRING$cc_ver
    fi
    mkdir -p $BASEDIR/hdf5_install/$HDF5_VER$BRANCH_STRING/hdf5-$MODULE_STRING$cc_ver
    mv $BASEDIR/hdf5_install/HDF5-*/HDF_Group/HDF5/$HDF5_VER/* $BASEDIR/hdf5_install/$HDF5_VER$BRANCH_STRING/hdf5-$MODULE_STRING$cc_ver
    rm -rf $BASEDIR/hdf5_install/HDF5-*
    module unload $cc_ver  # unload the compiler with version

    icnt=$(($icnt+1))

    #rm -fr build
  done
  module unload $master_mod #unload master module
done

pwd
cd $IN_DIR
