/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	print.h
 * @brief	print definition
 */

#ifndef _ACAPD_PRINT_H
#define _ACAPD_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif
#define INFO(...) \
    fprintf(stderr, "Info: %s:\n%d:%s:\n ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);

#define INFOP(...) \
    fprintf(stderr, __VA_ARGS__);

#ifndef DEBUG
#define acapd_debug(...)
#define acapd_praw(...)
#else
void acapd_debug(const char *format, ...);
#define acapd_praw printf
#endif /* DEBUG */
void acapd_print(const char *format, ...);
void acapd_perror(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /*  _ACAPD_PRINT_H */
