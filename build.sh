#!/bin/bash
set -x
cmake ../ \
	-DCMAKE_TOOLCHAIN_FILE=versal-linux \
	-DCMAKE_INCLUDE_PATH="/scratch/workspace/mdgr/acapd/include" \
	-DCMAKE_LIBRARY_PATH="/scratch/workspace/mdgr/acapd/lib" \
	-DWITH_EXAMPLE=on \

if [ $? -ne 0 ]; then
	exit 255
fi
make VERBOSE=1 DESTDIR=$(pwd) install
