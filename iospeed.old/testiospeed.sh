#!/bin/sh
# Test iospeed functionality.

fsize=10	# 10 MB
nerrors=0

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
	nerrors=`expr $nerrors + 1`
    fi
    # append -a
    cmd="$cmd -a"
    BANNER $cmd
    if $cmd ; then
	echo "--PASSED--"
    else
	echo "**FAILED**"
	nerrors=`expr $nerrors + 1`
    fi
done

if [ $nerrors -gt 0 ]; then
    echo "$nerrors failure(s) reported."
    exit 1
else
    echo All passed.
fi
exit 0
