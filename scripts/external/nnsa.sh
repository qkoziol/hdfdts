#!/bin/bash

HDF5_VER="1.11.4"

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

# CROSS-COMPILATION
CCOMP=""
#CCOMP="KNL"
KNL="false"
HPC=""
if [[ $CCOMP == "KNL" ]];then
    KNL="true"
fi

module list &> out
PRGENV_TYPE=`grep -i PrgEnv out | sed -e 's/.*PrgEnv-\(.*\)\/.*/\1/'`

if [[ $UNAME == "mutrino" ]];then
    module unload craype-hugepages2M
    module load cmake
    module unload PrgEnv-$PRGENV_TYPE
    module unload $PRGENV_TYPE
    MPI_MOD="PrgEnv-gnu/6.0.4 PrgEnv-intel/6.0.4"
    CC_MOD=(2 gcc gcc/4.9.3 gcc/7.2.0 2 intel intel/16.0.3 intel/18.0.2)
    HPC="sbatch"
fi
module list

icnt=-1
for mpi_mod in $MPI_MOD; do
  icnt=$(($icnt+1))
  num_CC=${CC_MOD[$icnt]}
  icnt=$(($icnt+1))
  CC_VER=${CC_MOD[$icnt]}
  for i in `seq 1 $num_CC`; do
    icnt=$(($icnt+1))
    cc_mod=${CC_MOD[$icnt]}
    
    module load $mpi_mod

    module unload $CC_VER
    module load $cc_mod

    export CC=cc
    export FC=ftn
    export CXX=CC

#    module list

    echo "timeout 2h ctest . -S HDF5config.cmake,SITE_BUILDNAME_SUFFIX=\"$mpi_mod $cc_mod\",HPC=\"$HPC\",MPI=\"true\",KNL=\"$KNL\",BUILD_GENERATOR=Unix -C Release -V -O hdf5.log"
#timeout 2h ctest . -S HDF5config.cmake,SITE_BUILDNAME_SUFFIX="$mpi_mod",HPC="$HPC",MPI="true",KNL="$KNL",BUILD_GENERATOR=Unix -C Release -V -O hdf5.log

    module unload $mpi_mod
    module unload $cc_mod
  done
done
