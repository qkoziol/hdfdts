#!/bin/sh
# Test iospeed functionality.

fsize=10	# 10 MB

BANNER()
{
    echo ================================================
    echo Testing $*
    echo ================================================
}


for n in 1 2 3 4 5 6 7 8 ; do
    cmd="./iospeed -m $n -s $fsize"
    BANNER $cmd
    if $cmd ; then
	echo "--PASSED--"
    else
	echo "**FAILED**"
    fi
    # append -a
    cmd="$cmd -a"
    BANNER $cmd
    if $cmd ; then
	echo "--PASSED--"
    else
	echo "**FAILED**"
    fi
done
