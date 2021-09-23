#### Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
#### SPDX-License-Identifier: MIT

## Overview

DFX-MGR provides infrastructure to abstract configuration and hardware resource
management for dynamic deployment of Xilinx based accelerator across different
platforms.

DFX-MGR is merged in Petalinux 2021.1 onwards and the recipe is called
‘dfx-mgr’. The recipe is not enabled by default and user is expected to enable
it. As of today, DFX-MGR can be used for dynamic loading/unloading of
accelerators to PL (Programmable Logic) or AI Engine. The functionality is
tested for loading/unloading of Flat shell (i.e. shell which does not have any
reconfigurable partitions) and DFX shell (i.e. shell which contains static and
dynamic region).  As you can see in the diagram below DFX-MGR can load a 3RP
design and a 2RP design with the corresponding accelerators dynamically without
having to reboot the board

Once you compile petalinux by enabling the dfx-mgr recipe, you should have
dfx-mgrd and dfx-mgr-client in `/usr/bin` of rootfs and libdfx-mgr.so in
`/usr/lib`. The config file `daemon.conf` can be found in `/etc/dfx-mgrd/`.
Config file is mandatory, refer the files section for details of it. A default
daemon.conf will be copied by the recipe and the user is expected to update as
required.

![Screenshot](https://media.gitenterprise.xilinx.com/user/978/files/c2254180-9d53-11eb-9371-ad2d44922a8b)

## Files required

DFX-MGR recognizes the designs under `/lib/firmware/xilinx` on device
filesystem, location can be updated in `daemon.conf`. Designs could be
downloaded using dnf or any other package manager or manually copied to the
previously mentioned location. Each folder under `/lib/firmware/xilinx` will be
treated as a base shell design wherein the folder is expected to have a
shell.json file. Base design folder can then have sub-folder for each of the
accelerators. Each accelerator is expected to have a accel.json. Have a look at
below folder structure for more understanding and the details of json config
files.

The expected directory structure under `/lib/firmware/xilinx` which contains a
3x1 PL shell design, AIE accel and a flat shell design. 3x1 shell has an
accelerator called FFT which contains three different partial bitstreams for
three slots of base shell. DFX-MGR expects '_slot#' as subfolders for each of
the accelerators for DFX designs.  Place the bitstream corresponding to that
slot in the respective subfolder.

It is not mandatory to have all the partial bitstreams for each slot but DFX-MGR
will fail to load an accelerator to the slot if no partial design is found.

```
$ls /lib/firmware/xilinx/base_design
base_Shell.pdi base.dtbo shell.json FFT AIE_Accel

$ls /lib/firmware/xilinx/base_design/FFT/
FFT_slot0 FFT_slot1 FFT_slot2

$ls /lib/firmware/xilinx/base_design/FFT/FFT_slot0/
partial.pdi partial.dtbo accel.json

$ls /lib/firmware/xilinx/base_design/FFT/FFT_slot1/
partial.pdi partial.dtbo accel.json

$ls /lib/firmware/xilinx/base_design/FFT/FFT_slot2/
partial_slot2.pdi partial.dtbo accel.json

$ls /lib/firmware/xilinx/base_design/AIE_accel
AIE_accel_slot3

$ls /lib/firmware/xilinx/base_design/AIE_accel/AIE_accel_slot3
aie.xclbin accel.json

$ls /lib/firmware/xilinx/flat_shell/
Flat_shell.bit.bin flat_shell.dtbo shell.json
``` 

As shown above, base_design has first three slots (0,1,2)
corresponding to PL slots from the FFT subfolders, and AIE_accel has slot3. When
DFX-MGR successfully loads FFT accel to one of the slots, -listPackage output
would show active slot as 0,1 or 2. When AIE_accel is programmed, it will show
active slot as 3. If AIE were to have multiple slots, the subfolders would have
to be named 3,4 and so on.

### daemon.conf

DFX-MGR is started on linux bootup and reads the config file
`/etc/dfx-mgrd/daemon.conf` from device.
 
```
$cat /etc/dfx-mgrd/daemon.conf
{
  "default_accel":"/etc/dfx-mgrd/default_firmware", //Optional: echo the
                             //package-name to default_firmware for any
                             // accelerator to be loaded on start of daemon
 "firmware_location": ["/lib/firmware/xilinx"]  //Required:Package directories
                        // monitored by DFX-MGR }
```

### shell.json

shell.json describes the base shell configuration information. Optional fields
can be skipped if not desired.

One of the below type should be used for shell_type as per your design.
XRT-FLAT: dfx-mgr will program the PL and update /etc/vart.conf on target with the
path to the active xclbin on success.
PL_FLAT: dfx-mgr will program the PL bitstream and treat the design as static.
PL_DFX: dfx-mgr will treat the design as DFX with number of slots as mentioned in
json.
```
$cat shell.json 
 { 
  "shell_type" : "PL_DFX",// Required: valid values are XRT_FLAT/PL_FLAT/PL_DFX
  "num_pl_slots": 3, //Required: Number of pl slots in your base shell design
  "num_aie_slots":1, //Required: Number of aie slots in your base shell design
  "load_base_design": "no" //Optional : If the shell was loaded during boot
              //and you want to skip loading base shell when loading the
              // accelerators to the slot.
  "device_name" : "a0010000.SIHA_Manager", //optional: IP name
  "reg_base" : "", //Optional: IP device base address 
  "reg_size" : "", //Optional 
  "clock_device_name" : "a0000000.clk_wiz", //optional
  "clock_reg_base" : "",//optional
  "clock_reg_size" : "", //optional
  "isolation_slots": [
	// optional, specify the register offsets and the corresponding values to be
	// written for each slot to bring it out of isolation, expects the length
    //of array equal to number of slots 
	{ "offset": ["0x4000","0x4004","0x4008","0x400c"],//Address offset for first slot
	"values": ["0x01","0x01", "0x0b", "0x02"] }, //Value to be written to above regs
	{ "offset": ["0x5000","0x5004","0x5008","0x500c"],
	"values": ["0x01","0x01", "0x0b", "0x02"]},
	{ "offset": ["0x6000","0x6004","0x6008","0x600c"],
	"values": ["0x01","0x01","0x0b", "0x02"] }]
	 }
```

### accel.json

accel.json describes the accelerator configuration. Optional fields can be
skipped if not desired. Flat shell designs are not required to have accel.json
since they do not have reconfigurable partition.

SIHA_PL_DFX: Use this option for PL accelerators build with siha hw manager,
this enables some extra steps required to bring the slots out of isolation in
addition to programming the bitstream.
XRT_AIE_DFX: Use this option for AIE accelerator which expects an xclbin to
program the AIE.
XRT_PL_DFX: Use this option for XRT based PL accelerator.
```
$cat accel.json
{ 
  "accel_type": "",  //Required: supported types are SIHA_PL_DFX/XRT_AIE_DFX/XRT_PL_DFX
  "accel_devices":[ // Optional: list of IP devices corresponding to this slot design
  { "dev_name": "20100000000.accel",
   "reg_base":"", 
   "reg_size":"" }],
  "sharedMemoryConfig": {
   "sizeInKB": "",
   "sharedMemType" : ""},
  "dataMoverConfig": { //Optional: skip
this node if application handles its own dma
  "dma_dev_name":"a4000000.dma",
  "dma_driver":"vfio-platform",
  "dma_reg_base":"",
  "dma_reg_size":"",
  "iommu_group":"0",
  "Bus": "platform",
  "HWType": "mcdma",
  "max_buf_size":"8388608",
  "dataMoverCacheCoherent": "Yes",
  "dataMoverVirtualAddress": "Yes",
  "dataMoverChnls":[
  {"chnl_id": 0,
   "chnl_dir":"ACAPD_DMA_DEV_W" },
  { "chnl_id": 0,
   "chnl_dir": "ACAPD_DMA_DEV_R" }]
  },
  "AccelHandshakeType": "”,
  "fallbackBehaviour": "software" //Optional: If hw accelerator fails to load,
                    //DFX-MGR will try software fallback
} 

```

## How to build

DFX-MGR depends on external libraries/frameworks like libdfx, libwebsockets, XRT,
i-notify etc. The recommended way to compile this repo is using
yocto where the required dependency are taken care of in the recipe.

If not using yocto then dependent libaries will need to be provided to cmake
using appropriate -DCMAKE_LIBRARY_PATH.

### How to build using yocto

To compile using yocto in 2021.1 onwards, do `bitbake dfx-mgr`.

### How to build using cmake

You would need to provide dependency libraries using -DCMAKE_LIBRARY_PATH for
cmake.

There is generic cmake toolchian file for generic Linux which is in
`cmake/platforms/cross-linux-gcc.cmake`

Set the path where you have all the dependent libraries in build.sh.

``` 
$ cd dfx-mgr 
$ mkdir build 
$ cd build 
$ ../build.sh
```

After build successfully completes, libdfx-mgr.so can be found
under `build/usr/local/lib` and binary under `build/usr/local/bin`.

## Usage guidelines

### Using command line

dfx-mgrd daemon should mostly be running on linux startup. Assuming it is
running, you can use below commands from command line to load/unload
accelerators.

1. Command to list the packages present on target filesystem under
`/lib/firmware/xilinx`.

An alternative command line utility which some users might be familiar with can
also be used `xmutil listapps`.
```
$dfx-mgr-client -listPackage
Accelerator    Accel_type          Base   Base_type  #slots(PL+AIE) Active_slot

  flat_shell	  PL_FLAT    flat_shell     PL_FLAT           (0+0)          -1
  AIE_accel   XRT_AIE_DFX   base_design      PL_DFX           (3+1)          3,
		FFT   SIHA_PL_DFX   base_design      PL_DFX           (3+1)         0,1
```
In the above output, AIE_accel is currently programmed to AIE and hence the
slot shows as 3. FFT is programmed to first two slots out of three slots of PL.
flat_shell is not programmed and show -1. Flat shell design do not have a
dynamic reconfigurable partition and hence #slots shows as 0.

2. Command to load one of the above listed accelerator. For dfx designs, base
shell would be loaded first if not already loaded and then accelerator will be
loaded to one of the free slots. If the shell is already loaded then only the
accelerator will be loaded to one of free slots.

Equivalent xmutil command is `xmutil loadapp FFT`.
```
$dfx-mgr-client -load FFT
```
3. Command to remove accelerator from the slot. If there is not accel in the
mentioned slot, this command will do nothing.

Equivalent xmutil command is `xmutil unloadapp 1`.
```
$dfx-mgr-client -remove 1
```

### Using library API

Users can write application to interact with daemon. Refer to example source
code in `repo/example/sys/linux/load_accel.c` for a simple example how to load
an accelerator.

For DFX designs we recommend using graph api's which provide good abstract
way to create a visual model of the RP's and inter connections. Refer to some
example code under `<repo>/example/sys/linux/graph`. 

## Known limitations

1. DFX-MGR uses i-notify for firmware file updates and i-notify doesn't work
with network filesystem.  Hence it is recommended to NOT boot linux over NFS for
correct functinality of DFX-MGR daemon.

2. DFX-MGR package names i.e. firmware folder names are limited to 64 character
currently and absolute path lengths are limited to 512 char. Hence avoid
creating long filenames. If absolutely required for longer names, file an issue
in the repo and will be adderssed on priority. 

3. I/O nodes doesn't support zero copy.

