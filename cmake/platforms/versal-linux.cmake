set (CMAKE_SYSTEM_PROCESSOR "aarch64"            CACHE STRING "")
set (CROSS_PREFIX           "aarch64-linux-gnu-" CACHE STRING "")
set (CMAKE_C_FLAGS		"-Wall -Wextra -Os" CACHE STRING "")
include (cross-linux-gcc)

# vim: expandtab:ts=2:sw=2:smartindent
