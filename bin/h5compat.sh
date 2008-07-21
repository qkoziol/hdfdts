#! /bin/sh

if [ -r ~hdftest/snapshots-h5compat/.h5compatrc ]; then
   cp ~hdftest/snapshots-h5compat/.h5compatrc ./api
   cp ~hdftest/snapshots-h5compat/.h5compatrc ./format
fi

if (cd ./api &&
    ./check_api.sh &&
    cd ../format &&
    ./check_format.sh &&
    cd ..); then
    :   #continue
else
    errcode=$?
    echo "===== Exit h5compat.sh with status=$errcode ====="
    echo ""
    exit $errcode
fi

