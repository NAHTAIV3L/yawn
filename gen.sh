#!/bin/bash

bin="yawn"
libs=""

echo "cflags = -Wall -g $(pkg-config --cflags $libs 2>/dev/null)"
echo "libs = $(pkg-config --libs $libs 2>/dev/null)"

echo 'rule cc'
echo '  deps = gcc'
echo '  depfile = $out.d'
echo '  description = CC $out'
echo '  command = gcc -MD -MF $out.d $cflags -c $in -o $out'
echo ''

echo 'rule link'
echo '  description = LD $out'
echo '  command = gcc $libs $in -o $out'
echo ''

files="$(find src -name '*.c')"
ofiles=$(echo $files | sed 's,^src/\(.*\)\.c$,objs/\1.o,g')

for f in $files; do
    of=$(echo $f | sed 's,^src/\(.*\)\.c$,objs/\1.o,g')
    echo -en "build $of: cc $f\n"
done

echo "build $bin: link $ofiles"
