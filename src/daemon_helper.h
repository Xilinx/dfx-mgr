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

#define CONFIG_PATH		"/etc/dfx-mgrd/daemon.conf"
#define WATCH_PATH_LEN 256
#define MAX_WATCH 500

/* Error codes */
#define DFX_MGR_LOAD_ERROR			(1U)
#define DFX_MGR_NO_PACKAGE_FOUND_ERROR		(2U)
#define DFX_MGR_NO_EMPTY_SLOT_ERROR		(3U)

int load_accelerator(const char *accel_name);
int remove_accelerator(int slot);
void allocBuffer(uint64_t size);
void sendBuff(uint64_t size);
void freeBuff(uint64_t pa);
int getFD(int slot, char *dev_name);
int dfx_getFDs(int slot, int *fd);
void list_accel_uio(int, char *, size_t);
char *get_accel_uio_by_name(int, const char *);
int siha_ir_buf_list(uint32_t sz, char *buf);
int siha_ir_buf_set(char *user_slot_seq);
void getShellFD();
void getClockFD();
char *listAccelerators();
void getRMInfo();
int dfx_init();
int dfx_getFDs(int slot, int *fd);
int user_load(int flag, char *binfile, char *overlay, char *region);
int user_unload_overlay(char *region);
int user_unload(int handle);

#ifdef __cplusplus
}
#endif

#endif
