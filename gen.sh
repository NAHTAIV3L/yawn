#!/bin/bash

bin="yawn"
libs=""
release=0
builddir="objs"

while [ "$1" ]; do
	case "$1" in
        --release|-r) release=1 ;;
        --builddir*) builddir="$(echo $1 | cut -d= -f2)" ;;
		-*) exit 1 ;;
	esac
	shift
done

CFLAGS=${CFLAGS:-""}

[ $release -eq 1 ] && CFLAGS="$CFLAGS -O2" || CFLAGS="$CFLAGS -Wall -g"

write() {
    echo "$@" >> build.ninja
}

rm -f build.ninja


write "builddir = $builddir"
write "cflags = $CFLAGS $(pkg-config --cflags $libs 2>/dev/null)"
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
echo "Wrote ./build.ninja file"
