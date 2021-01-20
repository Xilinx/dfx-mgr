/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/dma.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/shm.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "acapd-vfio-common.h"

#define MM2S_CR	0x0000 /*Control register*/
#define MM2S_SR	0x0004 /*Status register*/
#define MM2S_CHEN	0x0008 /*Channel enable/disable */
#define MM2S_CHSER	0x000C /*Channel in progress register*/
#define MM2S_ERR	0x0010 /*error register*/

#define MM2S_CH1CR	0x0040 /*CH1 control register*/
#define MM2S_CH1SR	0x0044 /*CH1 status register*/
#define MM2S_CH1CURDESC_LSB 0x0048 /*CH1 Current Descriptor (LSB)*/
#define MM2S_CH1CURDESC_MSB 0x004C /*CH1 Current Descriptor (MSB)*/
#define MM2S_CH1TAILDESC_LSB 0x0050 /*CH1 Tail Descriptor (LSB)*/
#define MM2S_CH1TAILDESC_MSB 0x0054 /*CH1 Tail Descriptor (MSB)*/

#define S2MM_CR	0x0500 /*Control register*/
#define S2MM_SR	0x0504 /*Status register*/
#define S2MM_CHEN	0x0508 /*Channel enable/disable */
#define S2MM_CHSER	0x050C /*Channel in progress register*/
#define S2MM_ERR	0x0510 /*error register*/

#define S2MM_CH1CR	0x0540 /*Ch1 control register*/
#define S2MM_CH1SR	0x0544 /*Ch1 status register*/
#define S2MM_CH1CURDESC_LSB 0x0548 /*CH1 Current Descriptor (LSB)*/
#define S2MM_CH1CURDESC_MSB 0x054C /*CH1 Current Descriptor (MSB)*/
#define S2MM_CH1TAILDESC_LSB 0x0550 /*CH1 Tail Descriptor (LSB)*/
#define S2MM_CH1TAILDESC_MSB 0x0554 /*CH1 Tail Descriptor (MSB)*/

#define MCDMA_BD_SIZE 0x40UL
#define MCDMA_RUNSTOP_MASK 0x1 /*0-stop,1-run*/
#define MCDMA_CHANNELS_MASK 0x1 /*set respective bit to enable each channel*/
#define MCDMA_CHANNEL_FETCH_MASK 0x1 /*start fetch of BDs for this channel*/
#define MCDMA_IDLE_MASK 0x2
#define MCDMA_HALTED_MASK 0x1
#define MCDMA_RESET_MASK 0x4
 
static int mcdma_vfio_dma_transfer(acapd_chnl_t *chnl, 
				    acapd_dma_config_t *config)
{
	acapd_device_t *dev;
	void *base_va; /**< AXI DMA reg mmaped base va address */
	uint64_t buf_addr, bd_pa, next_bd_pa;
	void *va;
	void *bd_va;
	void *next_bd_va;
	uint32_t offset = 0x40U * chnl->chnl_id;/* chnl_id start from 0*/
	acapd_shm_t bd_shm;
	int ret;
	void *retva;
	uint32_t buf_size, transfer_buf_size, tid;

	acapd_assert(chnl != NULL);
	acapd_assert(chnl->dev != NULL);
	acapd_assert(chnl->dev->va != NULL);
	acapd_assert(chnl->dev->ops != NULL);
	acapd_assert(chnl->dev->ops->va_to_da != NULL);
	
	dev = chnl->dev;
	base_va = dev->va;
	buf_size =  config->size;
	transfer_buf_size = buf_size;
	va = config->va;
	buf_addr = dev->ops->va_to_da(dev, va);
	if (buf_addr == (uint64_t)(-1)) {
		acapd_perror("%s: failed to get Buffer da from va %p.\n",
			     __func__, va);
		return -EINVAL;
	}
	
	memset(&bd_shm, 0, sizeof(bd_shm));
	
	if(bd_shm.va == NULL)
	{
		ret = acapd_alloc_shm(NULL, &bd_shm, 0x1000, 0);
	}	
	if (ret < 0) {
		acapd_perror("%s: failed to allocal BD shm memory.\n", __func__);
		return -EINVAL;
	}
	if (bd_shm.va == NULL) {
		acapd_perror("%s: BD shm va is NULL.\n", __func__);
		return -EINVAL;
	}
	retva = acapd_attach_shm(chnl, &bd_shm);
	if (retva == NULL) {
        acapd_perror("%s: failed to attach BD shm\n", __func__);
        return -EINVAL;
    }

	chnl->bd_va = bd_shm.va;
	bd_va = chnl->bd_va;
	bd_pa = dev->ops->va_to_da(dev, bd_va);
	if (bd_pa == (uint64_t)(-1)) {
		acapd_perror("%s: failed to get BD pa from va %p.\n",
			     __func__, bd_va);
		return -EINVAL;
	}

	next_bd_pa = bd_pa;
	next_bd_va = bd_va;
	tid = config->tid << 24;

	/* Program descriptors */
	for (; transfer_buf_size != 0; ) {
		uint32_t tmpbuf_size = transfer_buf_size;

		if (tmpbuf_size > chnl->max_buf_size)
			tmpbuf_size = chnl->max_buf_size;

		transfer_buf_size -= tmpbuf_size;
		if (transfer_buf_size != 0)
			next_bd_pa += MCDMA_BD_SIZE;
			
		/* Next descriptor address, desc are 64 bytes each*/
		*((volatile uint32_t *)((char *)next_bd_va+0x0)) = next_bd_pa & 0xFFFFFFC0;
		*((volatile uint32_t *)((char *)next_bd_va+0x4)) = next_bd_pa >> 32;

		/* Buffer Address*/
		*((volatile uint32_t *)((char *)next_bd_va+0x8)) = buf_addr & 0xFFFFFFFF;
		*((volatile uint32_t *)((char *)next_bd_va+0xC)) = buf_addr >> 32;

		/* buffer length bits 25:0| sof bit 31 | eof bit 30*/
		*((volatile uint32_t *)((char *)next_bd_va+0x14)) = (tmpbuf_size & 0x3FFFFFF) | 0x80000000 | 0x40000000;

		/* MM2S Program the tid bits 31:24*/
		if (chnl->dir == ACAPD_DMA_DEV_W) {
			*((volatile uint32_t *)((char *)next_bd_va+ 0x18)) = tid & 0xFF000000;
		}

		if (transfer_buf_size != 0)
		{
			next_bd_va += MCDMA_BD_SIZE;
			buf_addr += tmpbuf_size;
		}
	}
	if (chnl->dir == ACAPD_DMA_DEV_W) {
		uint32_t rx_status, rx_error;
		rx_status = *((volatile uint32_t *)((char *)base_va + S2MM_SR));
		rx_error = *((volatile uint32_t *)((char *)base_va + S2MM_ERR));

		if (((rx_status & MCDMA_HALTED_MASK) && (rx_error != 0)) ||
									(rx_status & MCDMA_IDLE_MASK))
		{
			acapd_debug("%s: MM2S Reset dma engine rx_status 0x%x rx_err 0x%x\n",
													__func__, rx_status, rx_error);
			*((volatile uint32_t *)((char *)base_va + MM2S_CR)) = MCDMA_RESET_MASK;
		}

		acapd_perror("%s: MM2S data from memory 0x%lx to stream, config->size %d, offset %d\n",
			    __func__, buf_addr,config->size, offset);
	
		//enable the channel
		*((volatile uint32_t *)((char *)base_va + MM2S_CHEN)) = MCDMA_CHANNELS_MASK;

		/* Program Current desc */
		*((volatile uint32_t *)((char *)base_va + offset + MM2S_CH1CURDESC_LSB)) = bd_pa & 0xFFFFFFC0;
		*((volatile uint32_t *)((char *)base_va + offset + MM2S_CH1CURDESC_MSB)) = bd_pa >> 32;
		
		//Enable fetch bit for channel
		*((volatile uint32_t *)((char *)base_va + offset + MM2S_CH1CR)) = MCDMA_CHANNEL_FETCH_MASK;

		//Run the device
		*((volatile uint32_t *)((char *)base_va + MM2S_CR)) = MCDMA_RUNSTOP_MASK;

		//program Tail desc. Each channel needs two desc-for data and control each
		*((volatile uint32_t *)((char *)base_va + offset + MM2S_CH1TAILDESC_LSB)) = next_bd_pa & 0xFFFFFFC0;
		*((volatile uint32_t *)((char *)base_va + offset + MM2S_CH1TAILDESC_MSB)) = next_bd_pa >> 32;

	} else {
		uint32_t tx_status, tx_error;
		tx_status = *((volatile uint32_t *)((char *)base_va + MM2S_SR));
		tx_error = *((volatile uint32_t *)((char *)base_va + MM2S_ERR));
		
		if (((tx_status & MCDMA_HALTED_MASK) && (tx_error != 0)) ||
									(tx_status & MCDMA_IDLE_MASK))
		{
			acapd_debug("%s: S2MM Reset dma engine tx_status 0x%x tx_err 0x%x\n",
													__func__, tx_status, tx_error);
			*((volatile uint32_t *)((char *)base_va + S2MM_CR)) = MCDMA_RESET_MASK;
		}
		acapd_perror("%s: S2MM data from stream to memory 0x%lx.\n",
			    __func__, buf_addr);

		//enable the channel
		*((volatile uint32_t *)((char *)base_va + S2MM_CHEN)) = MCDMA_CHANNELS_MASK;

		*((volatile uint32_t *)((char *)base_va + offset + S2MM_CH1CURDESC_LSB)) = bd_pa & 0xFFFFFFC0;
		*((volatile uint32_t *)((char *)base_va + offset + S2MM_CH1CURDESC_MSB)) = bd_pa >> 32;
		
		*((volatile uint32_t *)((char *)base_va + offset +  S2MM_CH1CR)) = MCDMA_CHANNEL_FETCH_MASK;

		*((volatile uint32_t *)((char *)base_va + S2MM_CR)) = MCDMA_RUNSTOP_MASK;
		/* program Tail desc. Each channel needs two desc-for data and control each*/
		*((volatile uint32_t *)((char *)base_va + offset + S2MM_CH1TAILDESC_LSB)) = next_bd_pa & 0xFFFFFFC0;
		*((volatile uint32_t *)((char *)base_va + offset + S2MM_CH1TAILDESC_MSB)) = next_bd_pa >> 32;
	}
	return buf_size;
}

static int mcdma_vfio_dma_stop(acapd_chnl_t *chnl)
{
	acapd_device_t *dev;
	void *base_va; /**< AXI DMA reg mmaped base va address */
	uint32_t v;

	acapd_assert(chnl != NULL);
	acapd_assert(chnl->dev != NULL);
	acapd_assert(chnl->dev->va != NULL);
	dev = chnl->dev;
	base_va = dev->va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		v = *((volatile uint32_t *)((char *)base_va + MM2S_CR));
		v &= (~MCDMA_RUNSTOP_MASK);
		*((volatile uint32_t *)((char *)base_va + MM2S_CR)) = v;
	} else {
		v = *((volatile uint32_t *)((char *)base_va + S2MM_CR));
		v &= (~MCDMA_RUNSTOP_MASK);
		*((volatile uint32_t *)((char *)base_va + S2MM_CR)) = v;
	}
	return 0;
}

static acapd_chnl_status_t mcdma_vfio_dma_poll(acapd_chnl_t *chnl)
{
	acapd_device_t *dev;
	void *base_va; /**< AXI DMA reg mmaped base va address */
	uint32_t v;

	acapd_assert(chnl != NULL);
	acapd_assert(chnl->dev != NULL);
	acapd_assert(chnl->dev->va != NULL);
	dev = chnl->dev;
	base_va = dev->va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		v = *((volatile uint32_t *)((char *)base_va + MM2S_SR));
		if ((v & MCDMA_HALTED_MASK) != 0) {
			return ACAPD_CHNL_STALLED;
		} else if ((v & MCDMA_IDLE_MASK) != 0) {
			return ACAPD_CHNL_IDLE;
		} else {
			return ACAPD_CHNL_INPROGRESS;
		}
	} else {
		v = *((volatile uint32_t *)((char *)base_va + S2MM_SR));
		if ((v & MCDMA_HALTED_MASK) != 0) {
			return ACAPD_CHNL_STALLED;
		} else if ((v & MCDMA_IDLE_MASK) != 0) {
			return ACAPD_CHNL_IDLE;
		} else {
			return ACAPD_CHNL_INPROGRESS;
		}
	}
}

static int mcdma_vfio_dma_reset(acapd_chnl_t *chnl)
{
	acapd_device_t *dev;
	void *base_va; /**< AXI DMA reg mmaped base va address */

	acapd_assert(chnl != NULL);
	acapd_assert(chnl->dev != NULL);
	acapd_assert(chnl->dev->va != NULL);
	dev = chnl->dev;
	base_va = dev->va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		*((volatile uint32_t *)((char *)base_va + MM2S_CR)) = MCDMA_RESET_MASK;

	} else {
		*((volatile uint32_t *)((char *)base_va + S2MM_CR)) = MCDMA_RESET_MASK;
	}
	return 0;
}

static int mcdma_vfio_dma_open(acapd_chnl_t *chnl)
{
	acapd_device_t *dev;
	int ret;

	acapd_assert(chnl != NULL);
	acapd_assert(chnl->dev != NULL);
	dev = chnl->dev;
	ret = acapd_device_get(dev);
	if (ret < 0) {
		acapd_perror("%s: failed to get device %s.\n",
			     __func__, dev->dev_name);
		return -EINVAL;
	}
	if (dev->refs == 1) {
		/* First time to open channel, reset it */
		void *base_va;

		base_va = dev->va;
		*((volatile uint32_t *)((char *)base_va + MM2S_CR)) = MCDMA_RESET_MASK;
		sleep(1);
		*((volatile uint32_t *)((char *)base_va + MM2S_CR)) = 0x0;
	}
	return 0;
}

static int mcdma_vfio_dma_close(acapd_chnl_t *chnl)
{
	acapd_device_t *dev;
	int ret;

	acapd_assert(chnl != NULL);
	acapd_assert(chnl->dev != NULL);
	dev = chnl->dev;
	if (dev->refs == 1) {
		/* last one to put device, reset it */
		void *base_va;

		base_va = dev->va;
		*((volatile uint32_t *)((char *)base_va + MM2S_CR)) = MCDMA_RESET_MASK;
		sleep(1);
		*((volatile uint32_t *)((char *)base_va + MM2S_CR)) = 0x0;
	}
	ret = acapd_device_put(dev);
	return ret;
}

acapd_dma_ops_t mcdma_vfio_dma_ops = {
	.name = "mcdma_vfio_dma",
	.transfer = mcdma_vfio_dma_transfer,
	.stop = mcdma_vfio_dma_stop,
	.poll = mcdma_vfio_dma_poll,
	.reset = mcdma_vfio_dma_reset,
	.open = mcdma_vfio_dma_open,
	.close = mcdma_vfio_dma_close,
};
