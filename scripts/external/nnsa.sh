#!/bin/bash -l

#Defaults

HDF5_VER="1.11.4"
KNL="false"

# READ COMMAND LINE FOR THE TEST TO RUN

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -v|--version)
    HDF5_VER="$2"
    shift # past argument
    shift # past value
    ;;
    -knl)
    KNL="true"
    shift # past argument
    ;;
    -h|--help)
    echo "USAGE: nnsa.sh [-v,--version x.x.x] [-knl] [-h,--help]

    where:

       -v,--version x.x.x    hdf5 version to test [default: 1.11.4]
       -knl                  compile for KNL [default: no]
       -h,--help             show this help text"
    exit 0
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# Summary of command line inputs
echo "HDF5_VER: $HDF5_VER"
echo "KNL: $KNL"

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

mkdir -p CI; cd CI
rm -rf hdf5-$HDF5_VER build hdf5.log Failed*
sleep 1
git clone https://git@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git hdf5-$HDF5_VER
sleep 1
ln -s hdf5-$HDF5_VER/config/cmake/scripts/CTestScript.cmake .
ln -s hdf5-$HDF5_VER/config/cmake/scripts/HDF5config.cmake .
ln -s hdf5-$HDF5_VER/config/cmake/scripts/HDF5options.cmake .


HPC=""
if [[ $UNAME == mutrino* ]];then
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
    MASTER_MOD="PrgEnv-gnu/6.0.4 PrgEnv-intel/6.0.4"
#   the Compiler to switch to:
#    Format: <number of compiler versions to check> <compiler type> <list of compiler versions (modules) ... repeat)
    CC_VER=(2 gcc gcc/4.9.3 gcc/7.2.0 2 intel intel/16.0.3 intel/18.0.2)
    HPC="sbatch"

elif [[ $UNAME == eclipse* ]]; then

    module purge
    module load cmake

    MASTER_MOD="intel-mpi/2018"
    CC_VER=(2 intel intel/16.0 intel/18.0)
    HPC="sbatch"

fi
module list

icnt=-1
for master_mod in $MASTER_MOD; do

  icnt=$(($icnt+1))
  num_CC=${CC_VER[$icnt]} # number of compilers for the  master_mod

  icnt=$(($icnt+1))
  module load $master_mod # Load the master module

  module unload ${CC_VER[$icnt]} # unload the general compiler type

  for i in `seq 1 $num_CC`; do # loop over compiler versions
    icnt=$(($icnt+1))
    cc_ver=${CC_VER[$icnt]} # compiler version

    module load $cc_ver # load the compiler with version

    export CC=cc
    export FC=ftn
    export CXX=CC

#DEBUG    module list

    echo "timeout 3h ctest . -S HDF5config.cmake,SITE_BUILDNAME_SUFFIX=\"$master_mod $cc_ver\",HPC=\"$HPC\",MPI=\"true\",KNL=\"$KNL\",BUILD_GENERATOR=Unix,LOCAL_SUBMIT=true,MODEL=HPC -C Release -VV -O hdf5.log"
    timeout 3h ctest . -S HDF5config.cmake,SITE_BUILDNAME_SUFFIX="$master_mod",HPC="$HPC",MPI="true",KNL="$KNL",BUILD_GENERATOR=Unix,LOCAL_SUBMIT=true,MODEL=HPC -C Release -VV -O hdf5.log

    module unload $cc_ver  # unload the compiler with version

    rm -fr build 
  done
  module unload $master_mod # 
done
