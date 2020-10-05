#!/bin/bash
set -x
cmake ../ \
	-DCMAKE_TOOLCHAIN_FILE=versal-linux \
	-DCMAKE_INCLUDE_PATH="$(pwd)/../include" \
	-DCMAKE_LIBRARY_PATH="$(pwd)/../lib" \

if [ $? -ne 0 ]; then
	exit 255
fi
make VERBOSE=1 DESTDIR=$(pwd) install
