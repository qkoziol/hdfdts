#! /bin/sh

if [ -r ~hdftest/snapshots-h5compat/.h5compatrc ]; then
   cp ~hdftest/snapshots-h5compat/.h5compatrc ./api
   cp ~hdftest/snapshots-h5compat/.h5compatrc ./format
fi

cd ./api
./check_api.sh
cd ../format
./check_format.sh
cd ..

