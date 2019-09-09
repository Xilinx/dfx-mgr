/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/assert.h>

void acapd_assert(unsigned int cond)
{
#ifdef DEBUG
	if ((cond) == 0) {
		printf("%s, %d\n", __FILE__, __LINE__);
		while(1);
	}
#else
	(void)cond;
#endif
}
