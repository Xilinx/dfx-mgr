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

#include <acapd/accel.h>
#include <acapd/helper.h>
#include <acapd/dma.h>
#include <stdint.h>

typedef struct acapd_shm acapd_shm_t;

/** Shared memory provider data structure. */
typedef struct acapd_shm_allocator acapd_shm_allocator_t;
typedef struct acapd_shm_allocator {
	const char *name; /**< name of shmem provider */
	void *priv; /**< private data */
	void *(*alloc)(acapd_shm_allocator_t *allocator, acapd_shm_t *shm, size_t size,  uint32_t attr); /**< shmem allocation function */
	void  (*free)(acapd_shm_allocator_t *allocator, acapd_shm_t *shm); /**< shmem free function */
	acapd_list_t node; /**< node */
} acapd_shm_allocator_t;

/** ACPAD shared memory data structure. */
struct acapd_shm {
	char *name; /**< shared memory name */
	char *va; /**< shared memory virtual address */
	size_t size; /**< shared memory size */
	unsigned int flags; /**< shared memory flag, cacheable, or
			      noncacheable */
	int id; /**< shared memory id */
	int refcount; /**< reference count */
	acapd_shm_allocator_t *allocator; /**< allocator where this shared
					    memory is from */
	acapd_list_t refs; /**< attached acapd channels references list */
};

extern acapd_shm_allocator_t acapd_default_shm_allocator;

void *acapd_alloc_shm(char *shm_allocator_name, acapd_shm_t *shm, size_t size,
		      uint32_t attr);

int acapd_free_shm(acapd_shm_t *shm);

int acapd_attach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm);

int acapd_detach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm);

int acapd_sync_shm_device(acapd_shm_t *shm, acapd_chnl_t *chnl);

//High level api's
void *acapd_accel_alloc_shm(acapd_accel_t *accel, size_t size, acapd_shm_t *shm);

int acapd_accel_write_data(acapd_accel_t *accel, acapd_shm_t *shm);

int acapd_accel_read_data(acapd_accel_t *accel, acapd_shm_t *shm);


#ifdef __cplusplus
}
#endif

#endif /*  _ACAPD_SHM_H */
