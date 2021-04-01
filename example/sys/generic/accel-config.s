/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "accel-config.h"

.section .rodata

.global __START_SHELL_JSON
.global __END_SHELL_JSON
.global __START_ACCEL0_JSON
.global __END_ACCEL0_JSON
.global __START_ACCEL0_PDI
.global __END_ACCEL0_PDI
.global __START_ACCEL0
.global __END_ACCEL0

.align 16
__START_SHELL_JSON:
	.incbin ACAPD_SHELL_JSON
__END_SHELL_JSON:
.align 16
__START_ACCEL0_JSON:
	.incbin ACAPD_ACCEL0_JSON
__END_ACCEL0_JSON:
.align 16
__START_ACCEL0_PDI:
	.incbin ACAPD_ACCEL0_PDI
__END_ACCEL0_PDI:
.align 16
__START_ACCEL0:
	.8byte __START_ACCEL0_JSON,__END_ACCEL0_JSON-__START_ACCEL0_JSON
	.8byte __START_ACCEL0_PDI,__END_ACCEL0_PDI-__START_ACCEL0_PDI
__END_ACCEL0:
