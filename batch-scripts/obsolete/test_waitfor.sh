#!/bin/sh


# Print messages to stdout
# Use this to show output heading to stdout
PRINT()
{
    echo "$*"
}



# Wait for a file for at most number of minutes
# $1--the file
# $2--number of minutes
# WAIT_STATUS set to:
#       -1 if errors encountered
#       0  if file found within time limit
#       1  if file not found within time limit
WAITFOR()
{
    wait_file=$1
    nminutes=$2
    if [ -z "$wait_file" -o ! "$nminutes" -ge 0 ]
    then
        PRINT "errors in argument of WAITFOR(): wait_file($1) or nminutes($2)"
        WAIT_STATUS=-1
        return
    fi
    while [ ! -f $wait_file ]; do
        if [ $nminutes -gt 0 ]; then
            PRINT "Wait For $wait_file to appear"
            sleep 10                    #sleep 1 minute
        else
            WAIT_STATUS=1
            return
        fi
        mv hdfcp.e* hdfcp.rr
        nminutes=`expr $nminutes - 1`
    done
    WAIT_STATUS=0
    PRINT "$wait_file is present."
    return
}
errfile=hdfcp.rr

    WAITFOR $errfile 90
    if [ $WAIT_STATUS -ne 0 ]; then
        errcode=$WAIT_STATUS
        PRINT "$errfile failed to appear before time expired."
        exit $errcode
    fi

