#!/bin/bash


cmake ../ \
	-DCMAKE_TOOLCHAIN_FILE=versal-linux \
	-DCMAKE_INCLUDE_PATH="$(pwd)/../include" \
	-DCMAKE_LIBRARY_PATH="$(pwd)/../opendfx-graph/drivers/XF2DFilter/src/lib/sw/aarch64-linux;" \

if [ $? -ne 0 ]; then
	exit 255
fi
make VERBOSE=1 DESTDIR=$(pwd) install
