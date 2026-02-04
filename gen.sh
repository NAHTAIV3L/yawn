#!/bin/bash

bin="yawn"
libs="libdrm"
release=0
builddir="objs"
installing=0
PREFIX="/usr"

run_install() {
    echo "Installing ${bin}"
    install -m 755 ./${bin} ${PREFIX}/bin
    exit 0
}

while [ "$1" ]; do
	case "$1" in
        --release|-r) release=1 ;;
        --builddir=*) builddir="${1#*=" ;;
        --prefix=*) PREFIX="${1#*=}" ;;
        --install) installing=1 ;;
		-*) exit 1 ;;
	esac
	shift
done

[ $installing -eq 1 ] && run_install

CFLAGS=${CFLAGS:-""}

[ $release -eq 1 ] && CFLAGS="$CFLAGS -O2" || CFLAGS="$CFLAGS -Wall -g"

write() {
    echo "$@" >> build.ninja
}

mkdir -p $builddir

echo "$CFLAGS $(pkg-config --cflags $libs 2>/dev/null)" | tr ' ' '\n' > compile_flags.txt
rm -f build.ninja

write "builddir = $builddir"
write "cflags = $CFLAGS $(pkg-config --cflags $libs 2>/dev/null)"
write "libs = $(pkg-config --libs $libs 2>/dev/null)"

write 'rule cc'
write '  deps = gcc'
write '  depfile = $out.d'
write '  description = CC $out'
write '  command = gcc -MD -MF $out.d $cflags -c $in -o $out'

write 'rule link'
write '  description = LD $out'
write '  command = gcc $libs $in -o $out'

write 'rule run_install'
write '  description = Installing Project'
write "  command = ./gen.sh --install --prefix=${PREFIX}"

files="$(find src -name '*.c')"
sedstr=$(printf 's,^src/\(.*\)\.c$,%s/\\1.o,g' ${builddir})
ofiles=$(echo "$files" | sed ${sedstr})

for f in $files; do
    of=$(echo $f | sed ${sedstr})
    write "build $of: cc $f"
done

write "build install: run_install"
write "build $bin: link $(echo $ofiles)"
write "default ${bin}"

echo "Wrote ./build.ninja file"
