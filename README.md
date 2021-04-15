#########################################################################
# Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
# SPDX-License-Identifier: MIT
#########################################################################

# Overview
DFX-MGR provides infrastructure to abstract configuration and hardware resource
management for dynamic deployment of accelerator across different platforms.
Once the code is compiled you will get dfx-mgrd-static binary and libdfx.so.
The library provides client application to interact with the daemon from console.
User can also write application code to manage accelerators using the libary api.

Library source code is in `src/`, the examples are in `example/`, headers are
in `include/` and required external libraries can be found in `lib/`

# How to build
DFX-MGR depends on external libraries/frameworks like libdfx, libwebsockets,
i-notify etc. The recommended way to compile this repo is using yocto where
the required dependency are taken care of in the recipe.

If not using yocto then dependent libaries will need to be provided to cmake
using appropriate -DCMAKE_LIBRARY_PATH.

## How to build using yocto

To compile using yocto in 2021.1 onwards, do `bitbake dfx-mgr`.

## How to build using cmake

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
After build succesfully completes, library can be found under `build/usr/local/lib`
and binary can be under `build/usr/local/bin`.

## Known limitations

1. Acapd internally uses dma_buf for shared memory buffer allocations and the file descriptor
for the buffers are shared with application over unix domain sockets. This is separate socket
connection in addition to the websocket communication between daemon and client. This is due
to not known way to share fd's over websocket/http.

2. Interactions with daemon will be serialized becuase of the way it is currently designed
and multiple applications trying to stress test load/unload concurrently will be undetermined.
However the applications can load and then compute as many times.

3. DFX-MGR uses i-notify for firmware file updates and i-notify doesn't work with network filesystem.
Hence it is recommended to NOT boot linux over NFS for correct functinality of DFX-MGR daemon.

4. DFX-MGR package names i.e. firmware folder names are limited to 32 characters currently. 
