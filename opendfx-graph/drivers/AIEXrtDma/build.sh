#!/bin/bash
set -x
CHROOT="/scratch/abajpai/openacap20210907/versal/software/platform_repo/sysroot/sysroots/cortexa72-cortexa53-xilinx-linux"

cmake ../ \
	-DCMAKE_TOOLCHAIN_FILE=versal-linux \
	-DCMAKE_INCLUDE_PATH="/group/siv_proto3/work/everest/abajpai/2021.1/versal20211/4RMWithAI2/dfx-mgr/build/usr/local/include" \
	-DCMAKE_LIBRARY_PATH="${CHROOT}/usr/lib;${CHROOT}/lib;${XILINX_VITIS}/aietools/lib/aarch64.o" \
	-DCMAKE_SYSROOT="/scratch/abajpai/openacap20210907/versal/software/platform_repo/sysroot/sysroots/cortexa72-cortexa53-xilinx-linux/" \

if [ $? -ne 0 ]; then
	exit 255
fi
#make VERBOSE=1 all
#make VERBOSE=1 DESTDIR=$(pwd) install

#$(pwd)/../include;$(pwd)/../recipe-sysroot/usr/include;${CHROOT}/usr/include;${CHROOT}/usr/include/xrt" \
#	-DCMAKE_PREFIX_PATH="${CHROOT}" \
