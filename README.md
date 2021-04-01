#########################################################################
# Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
# SPDX-License-Identifier: MIT
#########################################################################

# Overview
DFX-MGR provides APIs to launch accelerators and provide acceleration
services for across different platforms. Once the code is compiled you will
get dfx-mgrd-static binary and libdfx.so. The library provides client api to
interact with the daemon which can be used by application code or you can use
dfx-mgr-client binary to interact with daemon from console.

Library source code is in `src/`, the examples are in `example/`, headers are
in `include/` and required external libraries can be found in `lib/`

# How to build
DFX-MGR depends on external libraries/frameworks like libdfx, libwebsockets,
i-notify etc. The recommended way to compile this repo is using yocto where
the required dependency are taken care of.

## How to build using yocto

To compile using yocto in 2021.1 onwards, do `bitbake dfx-mgr`.

## How to build using cmake

You would need to provide dependency libraries using -DCMAKE_LIBRARY_PATH for cmake.

There is generic cmake toolchian file for generic Linux which is in
`cmake/platforms/cross-linux-gcc.cmake`

There is toolchian file build for Xilinx Versal Linux:
`cmake/platforms/versal-linux.cmake`.

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
