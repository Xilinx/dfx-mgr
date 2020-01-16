/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/dma.h>
#include <acapd/assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int acapd_accel_alloc_shm(acapd_accel_t *accel, size_t size, acapd_shm_t *shm)
{
	int ret;

	if (shm == NULL) {
		acapd_perror("%s: failed due to shm is NULL\n", __func__);
		return -EINVAL;
	}
	return acapd_alloc_shm(NULL, shm, size, 0);
}

int acapd_accel_transfer_data(acapd_accel_t *accel, acapd_shm_t *tx_shm,
			      acapd_shm_t *rx_shm)
{
	acapd_chnl_t *tx_chnl, *rx_chnl;
	acapd_list_t *node;
	acapd_dim_t dim;
	int ret;
	size_t tx_remain, rx_remain;
	acapd_shm_t ltx_shm, lrx_shm;

	if (accel == NULL) {
		acapd_perror("%s: fafiled due to accel is NULL.\n", __func__);
		return -EINVAL;
	}
	/* Assuming only two channels, tx and rx in the list */
	acapd_list_for_each(accel->chnls, node) {
		acapd_chnl_t *chnl;

		chnl = (acapd_chnl_t *)acapd_container_of(node, acapd_chnl_t,
							  node);
		if (chnl->dir == ACAPD_DMA_DEV_W) {
			tx_chnl = chnl;
		} else {
			rx_chnl = chnl;
		}
	}
	dim.number_of_dims = 1;
	dim.strides[0] = 1;
	if (tx_shm != NULL) {
		tx_remain = tx_shm->size;
		memcpy(&ltx_shm, tx_shm, sizeof(ltx_shm));
		ret = acapd_attach_shm(tx_chnl, tx_shm);
		if (ret != 0) {
			acapd_perror("%s: failed to attach tx shm\n", __func__);
			return -EINVAL;
		}
	}
	if (rx_shm != NULL) {
		rx_remain = rx_shm->size;
		memcpy(&lrx_shm, rx_shm, sizeof(lrx_shm));
		if (tx_shm != rx_shm) {
			ret = acapd_attach_shm(rx_chnl, rx_shm);
			if (ret != 0) {
				acapd_perror("%s: failed to attach rx shm\n",
					     __func__);
				return -EINVAL;
			}
		}
	}
	while (tx_remain != 0 && rx_remain != 0) {
		if (tx_remain != 0) {
			/* Check if it is ok to transfer data */
			ret = acapd_dma_poll(tx_chnl, 1);
			if (ret < 0) {
				acapd_perror("%s: tx_chnl errors\n", __func__);
				return -EINVAL;
			}
			ret = acapd_dma_config(&ltx_shm, tx_chnl, &dim, 0);
			if (ret != 0) {
				acapd_perror("%s: failed to config tx chnl\n",
					     __func__);
				return -EINVAL;
			}
			tx_remain -= ret;
			if (tx_remain != 0) {
				ltx_shm.va = (char *)(ltx_shm.va) + ret;
				ltx_shm.size = tx_remain;
			}
			ret = acapd_dma_start(tx_chnl, NULL);
			if (ret != 0) {
				acapd_perror("%s: failed to start tx\n",
					     __func__);
				return -EINVAL;
			}
		}
		if (rx_remain != 0) {
			/* Check if it is ok to transfer data */
			if (tx_remain != 0) {
				ret = acapd_dma_poll(rx_chnl, 0);
				if (ret == DMA_INPROGRESS) {
					continue;
				}
			} else {
				ret = acapd_dma_poll(rx_chnl, 1);
			}
			if (ret < 0) {
				acapd_perror("%s: rx_chnl errors\n", __func__);
				return -EINVAL;
			}
			ret = acapd_dma_config(&lrx_shm, rx_chnl, &dim, 0);
			if (ret != 0) {
				acapd_perror("%s: failed to config rx chnl\n",
					     __func__);
				return -EINVAL;
			}
			rx_remain -= ret;
			if (rx_remain != 0) {
				lrx_shm.va = (char *)(lrx_shm.va) + ret;
				lrx_shm.size = rx_remain;
			}
			ret = acapd_dma_start(rx_chnl, NULL);
			if (ret != 0) {
				acapd_perror("%s: failed to start rx\n",
					     __func__);
				return -EINVAL;
			}
		}
	}
	/* Finish sending and receiving data.
	 * Make sure DMA transaction has complete.
	 */
	if (tx_shm != 0) {
		ret = acapd_dma_poll(tx_chnl, 1);
		if (ret < 0) {
			acapd_perror("%s: tx failed\n", __func__);
			return -EINVAL;
		}
	}
	if (rx_shm != 0) {
		ret = acapd_dma_poll(rx_chnl, 1);
		if (ret < 0) {
			acapd_perror("%s: rx failed\n", __func__);
			return -EINVAL;
		}
	}
}
