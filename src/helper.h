/*
 * Copyright (c) 2020, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	helper.h
 * @brief	helper types functions definition.
 */

#ifndef _ACAPD_HELPER_H
#define _ACAPD_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <metal/list.h>

typedef struct metal_list acapd_list_t;

#define offset_of(structure, member)    \
    ((uintptr_t)&(((structure *)0)->member))

#define acapd_container_of(ptr, structure, member)  \
    (void *)((uintptr_t)(ptr) - offset_of(structure, member))

#endif /*  _ACAPD_HELPER_H */
