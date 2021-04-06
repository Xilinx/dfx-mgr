/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
/*#define INFO(...) \
    fprintf(stderr, "Info: %s:%d:%s: ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);
*/
#define INFO(...) \
    fprintf(stderr, "Info: %s:\n%d:%s:\n ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);

#define INFOP(...) \
    fprintf(stderr, __VA_ARGS__);

#define _unused(x) ((void)(x))
