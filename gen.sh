#!/bin/bash

bin="yawn"
libs=""

echo "cflags = -Wall -g $(pkg-config --cflags $libs 2>/dev/null)" > build.ninja
echo "libs = $(pkg-config --libs $libs 2>/dev/null)" >> build.ninja

echo 'rule cc' >> build.ninja
echo '  deps = gcc' >> build.ninja
echo '  depfile = $out.d' >> build.ninja
echo '  description = CC $out' >> build.ninja
echo '  command = gcc -MD -MF $out.d $cflags -c $in -o $out' >> build.ninja
echo '' >> build.ninja

echo 'rule link' >> build.ninja
echo '  description = LD $out' >> build.ninja
echo '  command = gcc $libs $in -o $out' >> build.ninja
echo '' >> build.ninja

files="$(find src -name '*.c')"
ofiles=$(echo $files | sed 's,^src/\(.*\)\.c$,objs/\1.o,g')

for f in $files; do
    of=$(echo $f | sed 's,^src/\(.*\)\.c$,objs/\1.o,g')
    echo -en "build $of: cc $f\n" >> build.ninja
done

echo "build $bin: link $ofiles" >> build.ninja
