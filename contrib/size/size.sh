#!/bin/sh

if [ "$1" != "" ]; then
    CROSS_COMPILE="$1-"
else
    CROSS_COMPILE=""
fi

CC="ccache ${CROSS_COMPILE}gcc"
CXX="ccache ${CROSS_COMPILE}g++"

OPTS="\
-Os
-O2
-Os -flto
-O2 -flto
-DNO_COMPILER -DNO_GARBAGE_COLLECTOR -Os -flto
-DNO_COMPILER -DNO_GARBAGE_COLLECTOR -O2 -flto
-Os -flto -static
-O2 -flto -static
"


target=$($CC -v 2>&1 | awk '/^Target:/ {print $2}')
version=$($CC -v 2>&1 | awk '/^gcc version/ {print $3}')

fname=sizes-${target}_gcc$version-$(date "+%Y%m%d").txt
# Full version string
$CC -v 2>&1 | grep "^gcc version" >$fname
echo >>$fname

IFS='
'

for opt in $OPTS; do
    make clean
    make OPT="$opt -s" CC="$CC" CXX="$CXX"
    echo $opt >>$fname
    size sqrl >>$fname
    echo >>$fname
done
