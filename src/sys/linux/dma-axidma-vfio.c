/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/dma.h>
#include <acapd/assert.h>
#include <acapd/print.h>
#include <acapd/shm.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>
#include <libfpga.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "dma-vfio-common.h"

#define XAXIDMA_TX_OFFSET	0x00000000 /**< TX channel registers base
					     *  offset */
#define XAXIDMA_RX_OFFSET	0x00000030 /**< RX channel registers base
					     * offset */
#define XAXIDMA_CR_OFFSET	 0x00000000   /**< Channel control */
#define XAXIDMA_SR_OFFSET	 0x00000004   /**< Status */
#define XAXIDMA_CDESC_OFFSET	 0x00000008   /**< Current descriptor pointer */
#define XAXIDMA_CDESC_MSB_OFFSET 0x0000000C   /**< Current descriptor pointer */
#define XAXIDMA_TDESC_OFFSET	 0x00000010   /**< Tail descriptor pointer */
#define XAXIDMA_TDESC_MSB_OFFSET 0x00000014   /**< Tail descriptor pointer */
#define XAXIDMA_SRCADDR_OFFSET	 0x00000018   /**< Simple mode source address
						pointer */
#define XAXIDMA_SRCADDR_MSB_OFFSET	0x0000001C  /**< Simple mode source address
						pointer */
#define XAXIDMA_DESTADDR_OFFSET		0x00000018   /**< Simple mode destination address pointer */
#define XAXIDMA_DESTADDR_MSB_OFFSET	0x0000001C   /**< Simple mode destination address pointer */
#define XAXIDMA_BUFFLEN_OFFSET		0x00000028   /**< Tail descriptor pointer */
#define XAXIDMA_SGCTL_OFFSET		0x0000002c   /**< SG Control Register */

#define XAXIDMA_CR_RUNSTOP_MASK	0x00000001 /**< Start/stop DMA channel */
#define XAXIDMA_CR_RESET_MASK	0x00000004 /**< Reset DMA engine */
#define XAXIDMA_CR_KEYHOLE_MASK	0x00000008 /**< Keyhole feature */
#define XAXIDMA_CR_CYCLIC_MASK	0x00000010 /**< Cyclic Mode */

#define XAXIDMA_HALTED_MASK		0x00000001  /**< DMA channel halted */
#define XAXIDMA_IDLE_MASK		0x00000002  /**< DMA channel idle */
#define XAXIDMA_ERR_INTERNAL_MASK	0x00000010  /**< Datamover internal
						      *  err */
#define XAXIDMA_ERR_SLAVE_MASK		0x00000020  /**< Datamover slave err */
#define XAXIDMA_ERR_DECODE_MASK		0x00000040  /**< Datamover decode
						      *  err */
#define XAXIDMA_ERR_SG_INT_MASK		0x00000100  /**< SG internal err */
#define XAXIDMA_ERR_SG_SLV_MASK		0x00000200  /**< SG slave err */
#define XAXIDMA_ERR_SG_DEC_MASK		0x00000400  /**< SG decode err */
#define XAXIDMA_ERR_ALL_MASK		0x00000770  /**< All errors */

static int axidma_vfio_dma_transfer(acapd_chnl_t *chnl, acapd_shm_t *shm,
				    acapd_shape_t *stride, uint32_t auto_repeat,
				    acapd_fence_t *fence)
{
	acapd_vfio_chnl_t *vchnl_info;
	void *base_va; /**< AXI DMA reg mmaped base va address */
	uint64_t da;
	uint32_t v;

	(void)stride;
	(void)auto_repeat;
	(void)fence;
	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	base_va = vchnl_info->ios[0].va;

	da = vfio_va_to_da(chnl, shm->va);
	if (da == (uint64_t)(-1)) {
		acapd_perror("%s: failed to get da from va %p.\n",
			     __func__, shm->va);
		return -EINVAL;
	}
	if (chnl->dir == ACAPD_DMA_DEV_W) {
		acapd_debug("%s: data from da 0x%llx to stream.\n",
			    __func__, da);
		/* write to stream, setup tx DMA */
		base_va = (void *)((char *)base_va + XAXIDMA_TX_OFFSET);
		v = *((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET));
		v |= XAXIDMA_CR_RUNSTOP_MASK;
		*((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET)) = v;
		v = *((volatile uint32_t *)((char *)base_va + XAXIDMA_SR_OFFSET));
		if ((v & XAXIDMA_HALTED_MASK) != 0) {
			acapd_perror("%s: tx failed due to chnl is halted.\n",
				     __func__);
			return -EINVAL;
		}
		*((volatile uint32_t *)((char *)base_va + XAXIDMA_SRCADDR_OFFSET)) =
			(uint32_t)(da & 0xFFFFFFFF);
		*((uint32_t *)((char *)base_va + XAXIDMA_SRCADDR_MSB_OFFSET)) =
			(uint32_t)((da & 0xFFFFFFFF00000000) >> 32);
		*((uint32_t *)((char *)base_va + XAXIDMA_BUFFLEN_OFFSET)) =
			shm->size;

	} else {
		acapd_debug("%s: data from stream to da 0x%llx.\n",
			    __func__, da);
		/* read from stream, setup rx DMA */
		base_va = (void *)((char *)base_va + XAXIDMA_RX_OFFSET);
		v = *((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET));
		v |= XAXIDMA_CR_RUNSTOP_MASK;
		*((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET)) = v;
		v = *((uint32_t *)((char *)base_va + XAXIDMA_SR_OFFSET));
		if ((v & XAXIDMA_HALTED_MASK) != 0) {
			acapd_perror("%s: rx failed due to chnl is halted.\n",
				     __func__);
			return -EINVAL;
		}
		*((volatile uint32_t *)((char *)base_va + XAXIDMA_DESTADDR_OFFSET)) =
			(uint32_t)(da & 0xFFFFFFFF);
		*((uint32_t *)((char *)base_va + XAXIDMA_DESTADDR_MSB_OFFSET)) =
			(uint32_t)((da & 0xFFFFFFFF00000000) >> 32);
		*((uint32_t *)((char *)base_va + XAXIDMA_BUFFLEN_OFFSET)) =
			shm->size;
	}
	return shm->size;
}

static int axidma_vfio_dma_stop(acapd_chnl_t *chnl)
{
	acapd_vfio_chnl_t *vchnl_info;
	void *base_va; /**< AXI DMA reg mmaped base va address */
	uint32_t v;

	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	base_va = vchnl_info->ios[0].va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		/* write to stream, stop tx DMA */
		base_va = (void *)((char *)base_va + XAXIDMA_TX_OFFSET);
		v = *((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET));
		v &= (~XAXIDMA_CR_RUNSTOP_MASK);
		*((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET)) = v;
	} else {
		/* read from stream, stop rx DMA */
		base_va = (void *)((char *)base_va + XAXIDMA_RX_OFFSET);
		v = *((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET));
		v &= (~XAXIDMA_CR_RUNSTOP_MASK);
		*((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET)) = v;
	}
	return 0;
}

static acapd_chnl_status_t axidma_vfio_dma_poll(acapd_chnl_t *chnl)
{
	acapd_vfio_chnl_t *vchnl_info;
	void *base_va; /**< AXI DMA reg mmaped base va address */
	uint32_t v;

	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	base_va = vchnl_info->ios[0].va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		/* write to stream, read tx DMA status */
		base_va = (void *)((char *)base_va + XAXIDMA_TX_OFFSET);
		v = *((volatile uint32_t *)((char *)base_va + XAXIDMA_SR_OFFSET));
		if ((v & XAXIDMA_ERR_ALL_MASK) != 0) {
			acapd_perror("%s, tx channel of %s errors: 0x%x\n",
				     __func__, chnl->dev_name, v);
			return ACAPD_CHNL_ERRORS;
		} else if ((v & XAXIDMA_IDLE_MASK) != 0) {
			return ACAPD_CHNL_IDLE;
		} else {
			return ACAPD_CHNL_INPROGRESS;
		}
	} else {
		/* read from stream, read rx DMA status */
		base_va = (void *)((char *)base_va + XAXIDMA_RX_OFFSET);
		v = *((volatile uint32_t *)((char *)base_va + XAXIDMA_SR_OFFSET));
		if ((v & XAXIDMA_ERR_ALL_MASK) != 0) {
			acapd_perror("%s, rx channel of %s errors: 0x%x\n",
				     __func__, chnl->dev_name, v);
			return ACAPD_CHNL_ERRORS;
		} else if ((v & XAXIDMA_IDLE_MASK) != 0) {
			return ACAPD_CHNL_IDLE;
		} else {
			return ACAPD_CHNL_INPROGRESS;
		}
	}
}

static int axidma_vfio_dma_reset(acapd_chnl_t *chnl)
{
	acapd_vfio_chnl_t *vchnl_info;
	void *base_va; /**< AXI DMA reg mmaped base va address */

	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	base_va = vchnl_info->ios[0].va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		/* write to stream, stop tx DMA */
		base_va = (void *)((char *)base_va + XAXIDMA_TX_OFFSET);
		*((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET)) =
			XAXIDMA_CR_RESET_MASK;
	} else {
		/* read from stream, stop rx DMA */
		base_va = (void *)((char *)base_va + XAXIDMA_RX_OFFSET);
		*((volatile uint32_t *)((char *)base_va + XAXIDMA_CR_OFFSET)) =
			XAXIDMA_CR_RESET_MASK;
	}
	return 0;
}

static int axidma_vfio_dma_open(acapd_chnl_t *chnl)
{
	int ret;
	struct stat s;
	char tmpstr[256];
	acapd_vfio_chnl_t *vchnl_info;

	/* unbind the driver if required */
	sprintf(tmpstr, "/sys/bus/platform/drivers/xilinx-vdma/%s",
		chnl->dev_name);
	ret = stat(tmpstr, &s);
	if(ret >= 0) {
		/* Need to unbind the driver */
		sprintf(tmpstr,
			"echo %s > /sys/bus/platform/drivers/xilinx-vdma/unbind",
			chnl->dev_name);
		system(tmpstr);
	}
	sprintf(tmpstr, "/sys/bus/platform/drivers/vfio-platform/%s",
		chnl->dev_name);
	ret = stat(tmpstr, &s);
	if(ret < 0) {
		/* Need to bind the driver with vfio  */
		sprintf(tmpstr,
			"echo vfio-platform > /sys/bus/platform/devices/%s/driver_override",
			chnl->dev_name);
		system(tmpstr);
		sprintf(tmpstr,
			"echo %s > /sys/bus/platform/drivers_probe",
			chnl->dev_name);
		system(tmpstr);
	}
	ret = vfio_open_channel(chnl);
	if (ret < 0) {
		acapd_perror("%s: failed to open vfio channel.\n", __func__);
		return -EINVAL;
	}
	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	if (vchnl_info->refs == 1) {
		void *reg_va; /**< AXI DMA reg mmaped base va address */
		uint32_t v;

		reg_va = vchnl_info->ios[0].va;
		reg_va = (void *)((char *)reg_va + XAXIDMA_TX_OFFSET + XAXIDMA_CR_OFFSET);
		/* First time to open channel, reset it */
		*((volatile uint32_t *)reg_va) = XAXIDMA_CR_RESET_MASK;
		do {
			v = *((volatile uint32_t *)reg_va);
		} while ((v & XAXIDMA_CR_RESET_MASK) != 0);
	}
	return 0;
}

acapd_dma_ops_t axidma_vfio_dma_ops = {
	.name = "axidma_vfio_dma",
	.mmap = vfio_dma_mmap,
	.munmap = vfio_dma_munmap,
	.transfer = axidma_vfio_dma_transfer,
	.stop = axidma_vfio_dma_stop,
	.poll = axidma_vfio_dma_poll,
	.reset = axidma_vfio_dma_reset,
	.open = axidma_vfio_dma_open,
	.close = vfio_close_channel,
};
