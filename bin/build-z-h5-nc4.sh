#! /bin/bash

while [ $# -gt 0 ]; do
   TEST_ARGS="$TEST_ARGS $1"
   shift
done

HOST_NAME=`hostname | cut -f1 -d.`       # no domain part

mkdir /mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME
# Assuming that if the directory exists the command above does nothing.  Could test for directory and make or clear it.

#wget -N ftp://ftp.unidata.ucar.edu/pub/netcdf/netcdf-4/zlib-1.2.3.tar.gz
#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#rm -rf zlib/*
#rm -rf ../hdf5/*

#gunzip zlib-1.2.3.tar.gz
#tar xf zlib-1.2.3.tar
#errcode=$?
#rm zlib-1.2.3.tar
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi
#ln -s zlib-1.2.3 zlib

#wget -N ftp://ftp.unidata.ucar.edu/pub/netcdf/netcdf-4.0-beta1.tar.gz
# moved getting and extracting the tarfile to snapshot
#WGET_OUTPUT=$(2>&1 cd /mnt/scr1/SnapTest/snapshots-netcdf4/current;2>&1 wget -N ftp://ftp.unidata.ucar.edu/pub/netcdf/snapshot/netcdf-4-daily.tar.gz)
#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#if [ $? -ne 0 ];then
#   echo $0: "$WGET_OUTPUT" Exiting.
#   exit 1
#fi

#if echo "$WGET_OUTPUT" | fgrep 'not retrieving' &> /dev/null
#then
#   echo "Snapshot unchanged"
#else
#   echo "New snapshot downloaded"
#   echo $(cd /mnt/scr1/SnapTest/snapshots-netcdf4/current; \
#          gunzip netcdf-4-daily.tar.gz; \
#          tar xf netcdf-4-daily.tar; \
#          EXTRACTDIR=`ls | grep netcdf-4.0-snapshot`; \
#          echo "netcdf-4-daily.tar extracted into $EXTRACTDIR; moving it to netcdf4.\n"; \
#          rm -rf netcdf4; \
#          mv $EXTRACTDIR netcdf4)
#   
#   errcode=$?
  # rm /mnt/scr1/SnapTest/snapshots-netcdf4/current/netcdf-4-daily.tar
  # if [[ $errcode -ne 0 ]]; then
  #    exit $errcode
  # fi
#fi

#patch -N -i /mnt/hdf/hdftest/NETCDF-4/current/tst_compounds.c.patch /mnt/scr1/SnapTest/snapshots-netcdf4/current/netcdf4/libsrc4/tst_compounds.c

#cd zlib

#./configure --prefix=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME
#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#make install > makeinstall.out
#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#mkdir ../hdf5
#mkdir ../hdf5/build

#svn export --force http://svn.hdfgroup.uiuc.edu/hdf5/trunk ../hdf5

#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#cd ../hdf5/build

#../configure --with-zlib=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME --prefix=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME --enable-fortran --enable-cxx --disable-shared > configure.out
#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#make > make.out
#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#make check > makecheck.out
#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#make install > makeinstall.out
#errcode=$?
#if [[ $errcode -ne 0 ]]; then
#   exit $errcode
#fi

#cd ../..

cd /mnt/scr1/SnapTest/snapshots-netcdf4/TestDir/$HOST_NAME

mkdir netcdf4
rm -rf netcdf4/*
cd netcdf4

#if [ "$HOST_NAME" == "linew" ];then
#   /mnt/scr1/SnapTest/snapshots-netcdf4/current/netcdf4/configure --enable-netcdf-4 --disable-f77 --with-hdf5=/mnt/scr1/pre-release/hdf5/v180/linew --with-zlib=/usr/lib --prefix=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME > configure.out
#   /mnt/scr1/SnapTest/snapshots-netcdf4/current/netcdf4/configure --enable-netcdf-4 --disable-f77 --with-hdf5=/mnt/scr1/SnapTest/snapshots-hdf5/TestDir/linewnsz/ --with-zlib=/usr/lib --prefix=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME > configure.out
#else
   /mnt/scr1/SnapTest/snapshots-netcdf4/current/netcdf4/configure --enable-netcdf-4 --disable-f77 --with-hdf5=/mnt/scr1/pre-release/hdf5/v180/$HOST_NAME $TEST_ARGS --prefix=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME #>& configure.out
#fi

#../../../current/netcdf4/configure --enable-netcdf-4 --disable-f77 --with-hdf5=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME --with-zlib=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME --prefix=/mnt/hdf/hdftest/NETCDF-4/current/image/$HOST_NAME > configure.out
errcode=$?
if [[ $errcode -ne 0 ]]; then
   exit $errcode
fi

make #>& make.out
errcode=$?
if [[ $errcode -ne 0 ]]; then
   exit $errcode
fi

make check #>& makecheck.out
errcode=$?
if [[ $errcode -ne 0 ]]; then
   exit $errcode
fi

make install #>& makeinstall.out
errcode=$?
echo "Exiting build-z-h5-nc4.sh with error code $errcode after running make install."
exit $errcode

