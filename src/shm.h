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
	void *va; /**< shared memory virtual address */
	size_t size; /**< shared memory size */
	unsigned int flags; /**< shared memory flag, cacheable, or noncacheable */
	int id; /**< shared memory id */
	int refcount; /**< reference count */
	struct acapd_shm_allocator *allocator; /**< allocator where this shared memory is from */
	acapd_list_t refs; /**< attached acapd channels references list */
} acapd_shm_t;

#ifdef __cplusplus
}
#endif

#endif /*  _ACAPD_SHM_H */
