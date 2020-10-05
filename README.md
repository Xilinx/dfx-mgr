
# Overview
ACPAD provides APIs to launch accelerators and provide acceleration
services for across different platforms. Once the code is compiled you will
get acapd_daemon-static binary and libacapd.so. The library provides client api to
interact with the daemon which can be used by application code or you can use
acap-static binary to interact with daemon from console.

Library source code is in `src/`, the examples are in `example/`, headers are
in `include/` and required external libraries can be found in `lib/`

# How to build
ACAPD uses cmake for cross platform and OSes compilation

## How to build for Linux

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

3. Current shared mem buffer size is 16MB for each slot, will be changed to configure
using json file.
