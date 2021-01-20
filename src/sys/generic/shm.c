/*
 * Copyright (c) 2020, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/print.h>
#include <dfx-mgr/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int acapd_generic_alloc_shm(acapd_shm_allocator_t *allocator,
				   acapd_shm_t *shm, size_t size, uint32_t attr)
{
	(void)allocator;
	(void)attr;
	if (shm == NULL) {
		acapd_perror("%s: shm is NULL.\n", __func__);
		return -EINVAL;
	}
	if (shm->id < 0) {
		shm->id = 0;
	}
	shm->va = malloc(size);
	if (shm->va == NULL) {
		acapd_perror("%s: failed to allocate memory.\n",
			     __func__);
	} else {
		acapd_debug("%s: allocated memory %p, size 0x%x.\n",
			    __func__, shm->va, size);
	}
	shm->size = size;
	return 0;
}

static void acapd_generic_free_shm(acapd_shm_allocator_t *allocator,
				   acapd_shm_t *shm)
{
	(void)allocator;
	if (shm == NULL) {
		acapd_perror("%s: error, shm is NULL.\n", __func__);
		return;
	}
	if (shm->va != NULL) {
		free(shm->va);
		shm->va = NULL;
		shm->size = 0;
	}
}

acapd_shm_allocator_t acapd_default_shm_allocator = {
	.name = "shm_allocator",
	.priv = NULL,
	.alloc = acapd_generic_alloc_shm,
	.free = acapd_generic_free_shm,
};
