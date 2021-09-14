#### Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
#### SPDX-License-Identifier: MIT

## Overview

DFX-MGR provides infrastructure to abstract configuration and hardware resource
management for dynamic deployment of accelerator across different platforms.
Once the code is compiled you will get dfx-mgrd-static binary and libdfx.so.
The library provides client application to interact with the daemon from console.
User can also write application code to manage accelerators using the libary api.

Library source code is in `src/`, the examples are in `example/`, headers are
in `include/` and required external libraries can be found in `lib/`

![Screenshot](https://media.gitenterprise.xilinx.com/user/978/files/c2254180-9d53-11eb-9371-ad2d44922a8b)

## Files required

DFX-MGR recognizes the designs under '/lib/firmware/xilinx' on device filesystem. Designs could be downloaded
using dnf or any other package manager or manually copied to the previously mentioned location. Each folder
under '/lib/firmware/xilinx' will be treated as a base shell design wherein the folder is expected to have
a shell.json file. Base design folder can then have sub-folder for each of the accelerators. Each accelerator
is expected to have a accel.json.
Have a look at below folder structure for more understanding and the details of json config files.

The expected directory structure under '/lib/firmware/xilinx' which contains a 3x1 PL shell design, AIE accel
and a flat shell design. 3x1 shell has an accelerator called FFT which contains three different partial bitstreams
for three slots of base shell. DFX-MGR expects '_slot#' as subfolders for each of the accelerators for DFX designs.
Place the bitstream corresponding to that slot in the respective subfolder.

It is not mandatory to have all the partial bitstreams for each slot but DFX-MGR will fail to load an accelerator
to the slot if no partial design is found.
```
$ls /lib/firmware/xilinx/base_design
base_Shell.pdi + base.dtbo + shell.json FFT AIE_Accel

$ls /lib/firmware/xilinx/base_design/FFT/
FFT_slot0 FFT_slot1 FFT_slot2

$ls /lib/firmware/xilinx/base_design/FFT/FFT_slot0/ 
partial_slot0.pdi  + partial.dtbo + accel.json

$ls /lib/firmware/xilinx/base_design/FFT/FFT_slot1/
partial_slot1.pdi + partial.dtbo + accel.json

$ls /lib/firmware/xilinx/base_design/FFT/FFT_slot2/
partial_slot2.pdi + partial.dtbo + accel.json

$ls /lib/firmware/xilinx/base_design/AIE_accel
AIE_accel_slot3

$ls /lib/firmware/xilinx/base_design/AIE_accel/AIE_accel_slot3
aie.xclbin accel.json

$ls /lib/firmware/xilinx/flat_shell/
Flat_shell.bit+ flat_shell.dtbo + shell.json
```
As shown above, base_design has first three slots (0,1,2) corresponding to PL slots from the FFT subfolders,
and AIE_accel has slot3. When DFX-MGR successfully loads FFT accel to one of the slots, -listPackage output would
show active slot as 0,1 or 2. When AIE_accel is programmed, it will show active slot as 3. If AIE were to have
multiple slots, the subfolders would have to be named 3,4 and so on.

### daemon.conf

DFX-MGR is started on linux bootup and reads the config file '/etc/dfx-mgrd/daemon.conf' from device.
```
$cat /etc/dfx-mgrd/daemon.conf
{
	"default_accel":"/etc/dfx-mgrd/default_firmware", //Optional: echo the package-name to default_firmware for
					// any accelerator is to be loaded by default on bootup
	"firmware_location": ["/lib/firmware/xilinx"]  //Required: Package directories monitored by DFX-MGR
}
```
### shell.json

shell.json describes the base shell configuration information. Optional fields can be skipped if not desired.
```
$cat shell.json
{
    "shell_type" : "PL_DFX", 	// Required: valid values are XRT_FLAT/PL_FLAT/PL_DFX
	"num_pl_slots": 3,	//Optional: Number of pl slots in your base shell design
	"num_aie_slots":1, 	//Optional: Number of aie slots in your base shell design
	"load_base_design": "no" //Optional : If the shell was loaded during boot and you want to skip loading
                                //base shell when loading the accelerators to the slot.
    "device_name" : "a0010000.SIHA_Manager", //optional 
    "reg_base" : "", //Optional: device base address
    "reg_size" : "", //Optional
    "clock_device_name" : "a0000000.clk_wiz", //optional 
    "clock_reg_base" : "",
    "clock_reg_size" : "",
	"isolation_slots": [  //optional, specify the register offsets and the corresponding values to 
				//be written for each slot to bring it out of isolation, expects the length of array
				// equal to number of slots 
        {
            "offset": ["0x4000","0x4004","0x4008","0x400c"],
            "values": ["0x01","0x01", "0x0b", "0x02"]
        },
        {
            "offset": ["0x5000","0x5004","0x5008","0x500c"],
            "values": ["0x01","0x01", "0x0b", "0x02"]
        },
        {
            "offset": ["0x6000","0x6004","0x6008","0x600c"],
            "values": ["0x01","0x01", "0x0b", "0x02"]
        }]
}
```
### accel.json

accel.json describes the accelerator configuration. Optional fields can be skipped if not desired.
```
$cat accel.json

{
    "accel_type": "",  //Required: supported types are SIHA_PL_DFX/XRT_AIE_DFX/XRT_PL_DFX
    "accel_devices":[ 
    {			// Optional: list of IP devices corresponding to this slot design
        "dev_name": "20100000000.accel",
        "reg_base":"",
        "reg_size":""
    }],
    "sharedMemoryConfig": {
        "sizeInKB": "",
        "sharedMemType" : ""
    },
    "dataMoverConfig": { //Optional: skip this node if application handles its own dma
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
            {
                "chnl_id": 0,
                "chnl_dir": "ACAPD_DMA_DEV_W"
            },
            {
                "chnl_id": 0,
                "chnl_dir": "ACAPD_DMA_DEV_R"
            }]
    },
    "AccelHandshakeType": "”,
    "fallbackBehaviour": "software" //Optional: If hw accelerator fails to load, DFX-MGR will try software fallback
}
```

## How to build
DFX-MGR depends on external libraries/frameworks like libdfx, libwebsockets,
i-notify etc. The recommended way to compile this repo is using yocto where
the required dependency are taken care of in the recipe.

If not using yocto then dependent libaries will need to be provided to cmake
using appropriate -DCMAKE_LIBRARY_PATH.

### How to build using yocto

To compile using yocto in 2021.1 onwards, do `bitbake dfx-mgr`.

### How to build using cmake

You would need to provide dependency libraries using -DCMAKE_LIBRARY_PATH for cmake.

There is generic cmake toolchian file for generic Linux which is in
`cmake/platforms/cross-linux-gcc.cmake`

The Linux acapd uses libfpga and some other external libraries. The pre-compiled
versions can be found under lib/ folder or to use your own version you can edit
the corresponding path to cmake in build.sh.
```
  $ cd acapd
  $ mkdir build
  $ cd build
  $ ../build.sh
```
After build successfully completes, library can be found under `build/usr/local/lib`
and binary can be under `build/usr/local/bin`.

## Known limitations

1. Interactions with daemon will be serialized becuase of the way it is currently designed
and multiple applications trying to stress test load/unload concurrently will be undetermined.
However the applications can load and then compute as many times.

2. DFX-MGR uses i-notify for firmware file updates and i-notify doesn't work with network filesystem.
Hence it is recommended to NOT boot linux over NFS for correct functinality of DFX-MGR daemon.

3. DFX-MGR package names i.e. firmware folder names are limited to 64 characters currently. 

4. Swapping of base shell not supported unless power cycling the board. #CR-1094476

5. Multiple graph merge not supported (future work)

6. I/O nodes doesn't support zero copy.

