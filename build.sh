#!/bin/bash


cmake ../ \
	-DCMAKE_TOOLCHAIN_FILE=versal-linux \
	-DCMAKE_INCLUDE_PATH="$(pwd)/../include;$(pwd)/../recipe-sysroot/usr/include;$(pwd)/../recipe-sysroot/usr/include/xrt;${XILINX_VITIS}/aietools/include" \
	-DCMAKE_LIBRARY_PATH="$(pwd)/../recipe-sysroot/lib;$(pwd)/../recipe-sysroot/usr/lib;${XILINX_VITIS}/aietools/lib/aarch64.o" \
	-DCMAKE_PREFIX_PATH="$(pwd)/../recipe-sysroot" \
	-DCMAKE_SYSROOT="$(pwd)/../recipe-sysroot" \

if [ $? -ne 0 ]; then
	exit 255
fi
make VERBOSE=1 DESTDIR=$(pwd) install
