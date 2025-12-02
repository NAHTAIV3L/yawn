#!/bin/bash

bin="yawn"
libs=""
builddir="objs"

write() {
    echo "$@" >> build.ninja
}

rm -f build.ninja

write "cflags = -Wall -g $(pkg-config --cflags $libs 2>/dev/null)"
write "libs = $(pkg-config --libs $libs 2>/dev/null)"

write 'rule cc'
write '  deps = gcc'
write '  depfile = $out.d'
write '  description = CC $out'
write '  command = gcc -MD -MF $out.d $cflags -c $in -o $out'
write ''

write 'rule link'
write '  description = LD $out'
write '  command = gcc $libs $in -o $out'
write ''

files="$(find src -name '*.c')"
sedstr=$(printf 's,^src/\(.*\)\.c$,%s/\\1.o,g' ${builddir})
ofiles=$(echo $files | sed ${sedstr})

for f in $files; do
    of=$(echo $f | sed ${sedstr})
    write "build $of: cc $f"
done

write "build $bin: link $ofiles"
