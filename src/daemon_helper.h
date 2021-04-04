/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    daemon_helper.h
 */

#ifndef _ACAPD_DAEMON_HELPER_H
#define _ACAPD_DAEMON_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#define WATCH_PATH_LEN 256
#define MAX_WATCH 50
int socket_d;

int load_accelerator(const char *accel_name);
void remove_accelerator(int slot);
void allocBuffer(uint64_t size);
void sendBuff(uint64_t size);
void freeBuff(uint64_t pa);
void getFDs(int slot);
int dfx_getFDs(int slot, int *fd);
void getShellFD();
void getClockFD();
void listAccelerators();
void getRMInfo();
void *threadFunc();
int dfx_init();
int dfx_getFDs(int slot, int *fd);
#ifdef __cplusplus
}
#endif

#endif
