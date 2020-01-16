/*
 * Copyright (c) 2020, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	shm.h
 * @brief	shared memory definition.
 */

#ifndef _ACAPD_SHM_H
#define _ACAPD_SHM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <metal/io.h>
#include <acapd/helper.h>
#include <acapd/dma.h>

/** Shared memory provider data structure. */
typedef struct acapd_shm_allocator {
	const char *name; /**< name of shmem provider */
	void *priv; /**< private data */
	void *(*alloc)(acapd_shm_allocator_t *allocator, acapd_shm_t *shm, uint3_t size,  uint32_t attribute); /**< shmem allocation function */
	void  (*free)(acapd_shm_allocator_t *allocator, acapd_shm_t *shm); /**< shmem free function */
	struct acapd_list_t node; /**< node */
} acapd_shm_allocator_t;

/** ACPAD shared memory data structure. */
struct acapd_shm {
	char name[64]; /**< shared memory name */
	struct metal_io_region io; /**< shared memory io region */
	char *va; /**< shared memory virtual address */
	size_t size; /**< shared memory size */
	unsigned int flags; /**< shared memory flag, cacheable, or noncacheable */
	int id; /**< shared memory id */
	int refcount; /**< reference count */
	struct acapd_shm_allocator *allocator; /**< allocator where this shared memory is from */
	acapd_list_t refs; /**< attached acapd channels references list */
} acapd_shm_t;

int acapd_alloc_shm(char *shm_allocator_name, acapd_shm_t *shm, size_t size, uint32_t attr);

int acapd_free_shm(acapd_shm_t *shm);

int acapd_attach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm);

int acapd_detach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm);

int acapd_create_dma_channel(char *name, uint32_t type, int chnl_id, acapd_chnl_t *chnl);

int acapd_destroy_dma_channel(acapd_chnl_t *chnl);

int acapd_sync_shm_device(acapd_shm_t *shm, acapd_chnl_t *chnl);

//High level api's
int acapd_accel_alloc_shm(acapd_accel_t *accel, size_t size, acapd_shm_t *shm);

int acapd_accel_transfer_data(acapd_accel_t *accel, acapd_shm_t *tx_shm,
			      acapd_shm_t *rx_shm);

int acapd_accel_wait_for_data_ready(acapd_accel_t *accel);

#ifdef __cplusplus
}
#endif

#endif /*  _ACAPD_SHM_H */
