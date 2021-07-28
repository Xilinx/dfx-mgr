#!/bin/bash
set -x
cmake ../ \
	-DCMAKE_TOOLCHAIN_FILE=versal-linux \
	-DCMAKE_INCLUDE_PATH="$(pwd)/../include;$(pwd)/../recipe-sysroot/usr/include" \
	-DCMAKE_LIBRARY_PATH="" \
	-DCMAKE_PREFIX_PATH="$(pwd)/../recipe-sysroot" \
	-DCMAKE_SYSROOT="$(pwd)/../recipe-sysroot" \

if [ $? -ne 0 ]; then
	exit 255
fi
make VERBOSE=1 DESTDIR=$(pwd) install
