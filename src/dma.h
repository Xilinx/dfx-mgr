/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	dma.h
 * @brief	DMA primitives for libmetal.
 */

#ifndef _ACAPD_DMA_H
#define _ACAPD_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup dma DMA Interfaces
 *  @{ */

#include <stdint.h>
#include <stdlib.h>
#include <acapd/print.h>
#include <acapd/helper.h>
/**
 * @brief ACAPD DMA transaction direction
 */
typedef enum acapd_dir {
	ACAPD_DMA_DEV_R	= 1U, /**< DMA direction, device read */
	ACAPD_DMA_DEV_W, /**< DMA direction, device write */
	ACAPD_DMA_DEV_WR, /**< DMA direction, device read/write */
} acapd_dir_t;

/**
 * @brief ACAPD channel connection type
 */
typedef enum acapd_chnl_conn {
	ACAPD_CHNL_CC		= 0x1U, /**< Connection is cache coherent */
	ACAPD_CHNL_NONC		= 0x2U, /**< Connection is not cache coherent */
	ACAPD_CHNL_VADDR	= 0x4U, /**< Connection is not cache coherent */
} acapd_chnl_conn_t;

/**
 * @brief ACAPD channel status
 */
typedef enum acpad_chnl_status {
	ACAPD_CHNL_IDLE		= 0U, /**< Channel is idle */
	ACAPD_CHNL_INPROGRESS	= 1U, /**< Channel is in progress */
	ACAPD_CHNL_STALLED	= 2U, /**< Channel is stalled */
	ACAPD_CHNL_ERRORS	= 4U, /**< Channel is stalled */
	ACAPD_CHNL_TIMEOUT	= 8U, /**< Channel is stalled */
} acapd_chnl_status_t;

#define ACAPD_MAX_DIMS	4U

/**
 * @brief ACAPD data shape structure
 */
typedef struct acapd_shape {
	int num_of_dims; /**< Number of dimensions */
	int dim[ACAPD_MAX_DIMS]; /**< Number of elements in each dimension */
} acapd_shape_t;

typedef struct acapd_chnl acapd_chnl_t;
typedef struct acapd_shm acapd_shm_t;

/**
 * @brief DMA fence data structure
 * TODO
 */
typedef int acapd_fence_t;

/**
 * @brief DMA poll callback
 * TODO
 */
typedef void (*acapd_dma_cb_t)(acapd_chnl_t *chnl, int reason);

/** DMA Channel Operations */
typedef struct acapd_dma_ops {
	const char name[128]; /**< name of the DMA operation */
	void *(*mmap)(acapd_chnl_t *chnl, acapd_shm_t *shm);
	int (*munmap)(acapd_chnl_t *chnl, acapd_shm_t *shm);
	int (*transfer)(acapd_chnl_t *chnl, acapd_shm_t *shm,
			void *va, size_t size,
			acapd_shape_t *stride, uint32_t auto_repeat,
			acapd_fence_t *fence);
	int (*stop)(acapd_chnl_t *chnl);
	acapd_chnl_status_t (*poll)(acapd_chnl_t *chnl);
	int (*reset)(acapd_chnl_t *chnl);
	int (*open)(acapd_chnl_t *chnl);
	int (*close)(acapd_chnl_t *chnl);
} acapd_dma_ops_t;

/**
 * @brief ACAPD DMA channel data structure
 */
typedef struct acapd_chnl {
	char name[128]; /**< DMA channel name/or path */
	const char *dev_name; /**< related device name */
	char *id; /**< DMA channel logical id */
	int iommu_group; /**< iommu group if the channel is behind IOMMU */
	int chnl_id; /**< hardware channel id of a data mover controller */
	acapd_dir_t dir; /**< DMA channel direction */
	uint32_t conn_type; /**< type of data connection with this channel */
	void *sys_info; /**< System private data for the channel */
	int is_open; /**< Indicate if the channel is open */
	acapd_dma_ops_t *ops; /**< DMA operations */
	acapd_list_t node; /**< list node */
} acapd_chnl_t;

int acapd_dma_transfer(acapd_chnl_t *chnl, acapd_shm_t *shm,
		       void *va, size_t size,
		       acapd_shape_t *stride, uint32_t auto_repeat,
		       acapd_fence_t *fence);
int acapd_dma_stop(acapd_chnl_t *chnl);
int acapd_dma_poll(acapd_chnl_t *chnl, uint32_t wait_for_complete,
		   acapd_dma_cb_t poll_cb, uint32_t timeout);
int acapd_dma_open(acapd_chnl_t *chnl);
int acapd_dma_reset(acapd_chnl_t *chnl);
int acapd_dma_close(acapd_chnl_t *chnl);
int acapd_create_dma_channel(const char *name, const char *dev_name,
			     int iommu_group,
			     acapd_chnl_conn_t conn_type, int chnl_id,
			     acapd_dir_t dir, acapd_chnl_t *chnl);
int acapd_destroy_dma_channel(acapd_chnl_t *chnl);

#include <acapd/sys/@PROJECT_SYSTEM@/dma.h>

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_DMA_H */
