
# Overview
ACPAD provides APIs to launch accelerators and provide acceleration
services for across different platforms.

Library source code is in `src/`, the examples are in `example/`

# How to build
ACAPD uses cmake for cross platform and OSes compilation

## How to build for Linux

There is generic cmake toolchian file for generic Linux which is in
`cmake/platforms/cross-linux-gcc.cmake`

There is toolchian file build for Xilinx Versal Linux:
`cmake/platforms/versal-linux.cmake`.

The Linux acapd uses libfpga, in order to build the library, you will need
to specify where the libfpga is. Here is how to build the Linux library:
```
  $ cd acapd
  $ mkdir build
  $ cd build
  $ cmake ../ -DCMAKE_TOOLCHAIN_FILE=versal-linux \
    -DCMAKE_INCLUDE_PATH=<libfpga_include>
  $ make VERBOSE=1 DESTDIR=$(pwd) install
```
The `libfpga` source and library is in `/group/siv3/staff/acapd/`
The library will be `usr/local/lib/`, includes will be in `usr/local/include`

The Linux example is available from the library. Here is how to build the Linux
example:
```
  $ cd acapd
  $ mkdir build
  $ cd build
  $ cmake ../ -DCMAKE_TOOLCHAIN_FILE=versal-linux \
    -DCMAKE_INCLUDE_PATH=<libfpga_include> \
    -DWITH_EXAMPLE=y
  $ make VERBOSE=1 DESTDIR=$(pwd) install
```
The application is installed in `usr/local/bin`
