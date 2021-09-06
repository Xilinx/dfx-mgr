#!/bin/bash
set -x
CHROOT="/scratch/abajpai/openacap/versal/software/platform_repo/sysroot/sysroots/cortexa72-cortexa53-xilinx-linux"


cmake ../ \
	-DCMAKE_TOOLCHAIN_FILE=versal-linux \
	-DCMAKE_INCLUDE_PATH="$(pwd)/../include;$(pwd)/../recipe-sysroot/usr/include;${CHROOT}/usr/include;${CHROOT}/usr/include/xrt" \
	-DCMAKE_LIBRARY_PATH="${CHROOT}/usr/lib" \
	-DCMAKE_PREFIX_PATH="$(pwd)/../recipe-sysroot" \
	-DCMAKE_SYSROOT="$(pwd)/../recipe-sysroot" \

if [ $? -ne 0 ]; then
	exit 255
fi
make VERBOSE=1 DESTDIR=$(pwd) install
