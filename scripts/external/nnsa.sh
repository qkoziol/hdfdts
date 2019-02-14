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
#git clone https://git@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git hdf5-$HDF5_VER
sleep 1

# CROSS-COMPILATION
CCOMP=""
#CCOMP="KNL"
KNL="false"
HPC=""
if [[ $CCOMP == "KNL" ]];then
    KNL="true"
fi

if [[ $UNAME == "jelly" ]];then
    MPI_MOD="MPICH/3.2-GCC-4.9.3 MPICH/3.2.1-PGI-18.4"
    CC_MOD=" "
    HPC="sbatch"
fi


for mpi_mod in $MPI_MOD; do
  for cc_mod in $CC_MOD; do
    
    module load CMake
    module load $mpi_mod
    module load $cc_mod

    export CC=cc
    export FC=ftn
    export CXX=CC

    echo "timeout 2h ctest . -S HDF5config.cmake,SITE_BUILDNAME_SUFFIX=\"$mpi_mod\",HPC=\"$HPC\",MPI=\"true\",BUILD_GENERATOR=Unix -C Release -V -O hdf5_$mpi_mod.log"
#timeout 2h ctest . -S HDF5config.cmake,SITE_BUILDNAME_SUFFIX="$mpi_mod",HPC="$HPC",MPI="true",BUILD_GENERATOR=Unix -C Release -V -O hdf5_$mpi_mod.log

    module purge
  done
done
