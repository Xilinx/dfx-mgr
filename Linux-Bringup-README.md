
# Default Images
Location: /group/siv3/staff/acapd/default_images/
```
├── arm-trusted-firmware.elf # ARM trusted firmware
├── bzip2-system-nopl.dtb # Linux bringup DTB
├── bzip2.tcl # example TCL to bringup Linux
├── bzip_accel_eve_wrapper.pdi # Bzip2 hardware PDI
├── Image # Default Linux image
├── rootfs.cpio # Base rootfs cpio
└── rootfs.cpio.ub # base Rootfs with u-boot wrapper
```

# How to launch the images
TCL Example: /group/siv3/staff/acapd/default_images/bzip2.tcl

Steps:
Launch a command line console for the interaction with the target Linux
$ systest tenzing
# Wait until you get a board
# Setup tftpd directory to load images from u-boot
Systest# tftpd "<your tftpd directory>"
# Connect to console
Systest# connect com0
Launch another command linux console to use xsdb with systest:
$ ssh <host_name of the systest machine you have got>
> /opt/systest/common/bin/systest-client
Systest# xsdb
xsdb% connect
xsdb% targets -set -filter {name =~ "Versal*"}
xsdb% device program <PDI>
xsdb% targets -set -filter {name =~ "Cortex-A72 #0*" }
xsdb% rst -proc
xsdb% dow -force -data <DTB> 0x1000
xsdb% dow -force u-boot.elf
xsdb% dow -force arm-trusted-firmware.elf
xsdb% con
On the other console, when you see u-boot boots, tftp boot the Linux images:
# If Image doesn't include rootfs:
Versal> dhcp; setenv serverip 10.10.70.101; tftpb 80000 Image; tftpb 10000000 rootfs.cpio.ub; tftpb 14000000 system.dtb; booti 80000 10000000 14000000
# If Image includes rootfs:
Versal> dhcp; setenv serverip 10.10.70.101; tftpb 80000 Image;tftpb 14000000 system.dtb; booti 80000 - 14000000
Repackage Rootfs
#Extract rootfs.cpio(do all operations with sudo access)
mkdir tmprootfs
cd tmprootfs
sudo cpio -idv < ../rootfs.cpio

#Make changes as per your requiredments
#Pack rootfs
sudo find . | cpio --create --format='newc' > ../rootfs.cpio
Rebuild Linux image with the new Rootfs CPIO
#On bash setup env before starting compilation stepsexport ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
source /proj/xbuilds/2019.2_daily_latest/installs/lin64/Scout/2019.2/settings64.sh
# Forking the Linux repo failed, for now use this tested commit: 724da59eee2f2b9b7172116c9729ea8a38d2d3c4
git clone https://gitenterprise.xilinx.com/linux/linux-xlnx.git -b master-next-test
git checkout 724da59eee2f2b9b7172116c9729ea8a38d2d3c4
 
#For Versal
make ARCH=arm64 xilinx_versal_defconfig
 
make ARCH=arm64 menuconfig
#Point to your arm64 rootfs cpio image generated in previous step CONFIG_INITRAMFS_SOURCE=""
#Enable the following options:
CONFIG_INITRAMFS_SOURCE="<CPIO_PATH>"
CONFIG_FPGA=y
CONFIG_FPGA_MGR_VERSAL_FPGA=y


make ARCH=arm64 -j


# Image will be in arch/arm64/boot/Image 


