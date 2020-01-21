/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	accel.h
 * @brief	accelerator definition.
 */

#ifndef _ACAPD_ACCEL_H
#define _ACAPD_ACCEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define ACAPD_ACCEL_STATUS_UNLOADED	0U
#define ACAPD_ACCEL_STATUS_LOADING	1U
#define ACAPD_ACCEL_STATUS_INUSE	2U
#define ACAPD_ACCEL_STATUS_UNLOADING	3U

#define ACAPD_ACCEL_SUCCESS		0
#define ACAPD_ACCEL_FAILURE		(-1)
#define ACAPD_ACCEL_TIMEDOUT		(-2)
#define ACAPD_ACCEL_MISMATCHED		(-3)
#define ACAPD_ACCEL_RSCLOCKED		(-4)
#define ACAPD_ACCEL_NOTSUPPORTED	(-5)
#define ACAPD_ACCEL_INVALID		(-6)
#define ACAPD_ACCEL_LOAD_INUSE		(-7)

#define ACAPD_ACCEL_INPROGRESS		1

#define ACAPD_ACCEL_PKG_TYPE_NONE	0U
#define ACAPD_ACCEL_PKG_TYPE_PDI	1U
#define ACAPD_ACCEL_PKG_TYPE_LAST	2U

#include <acapd/dma.h>
#include <acapd/helper.h>
#include <acapd/sys/@PROJECT_SYSTEM@/accel.h>

/**
 * @brief accel package information structure
 */
typedef struct {
	uint32_t type; /**< type of package element */
	char name[128]; /**< name of the package element */
	uint64_t size; /**< size of package element */
	uint32_t is_end; /**< if it is the end of package */
} acapd_accel_pkg_hd_t;

/**
 * @brief accel structure
 */
typedef struct {
	acapd_accel_pkg_hd_t *pkg; /**< pointer to the package */
	acapd_accel_sys_t sys_info; /**< system specific accel information */
	unsigned int type; /**< type of the accelarator */
	unsigned int status; /**< status of the accelarator */
	unsigned int is_cached; /**< if the accelerator is cached */
	int load_failure; /**< load failure */
	int num_chnls;	/**< number of channels */
	acapd_chnl_t *chnls; /**< list of channels */
} acapd_accel_t;

acapd_accel_pkg_hd_t *acapd_alloc_pkg(size_t size);

/* User applicaiton will need to allocate large enough memory for the package.
 * Library is not going to allocate the memory for it. This function is supposed
 * to be replaced by a host tool.
 */
int acapd_config_pkg(acapd_accel_pkg_hd_t *pkg, uint32_t type, char *name,
		     size_t size, void *data, int is_end);

void init_accel(acapd_accel_t *accel, acapd_accel_pkg_hd_t *pkg);

int load_accel(acapd_accel_t *accel, unsigned int async);

int accel_load_status(acapd_accel_t *accel);

/*
 * TODO: Do we want stop accel for accel swapping?
 */
int remove_accel(acapd_accel_t *accel, unsigned int async);

#ifdef ACAPD_INTERNAL
int sys_load_accel(acapd_accel_t *accel, unsigned int async);

int sys_remove_accel(acapd_accel_t *accel, unsigned int async);
#endif /* ACAPD_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_ACCEL_H */
