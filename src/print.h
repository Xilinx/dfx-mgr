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

#define DEBUG
#ifndef DEBUG
#define acapd_debug(...)
#define acapd_praw(...)
#else
void acapd_debug(const char *format, ...);
#define acapd_praw printf
#endif /* DEBUG */
void acapd_print(const char *format, ...);
void acapd_perror(const char *format, ...);

/**
 * Convenience macros DFX_ERR, DFX_PR, DFX_DBG to add source
 * function name and the line number before the message.
 * Inspired by pr_err, etc. in the kernel's printk.h.
 */
#ifdef errno
#define DFX_ERR(fmt, args ...) do { fprintf(stderr, \
		"DFX-MGRD> ERROR:%s():%u " fmt ": %s\n", \
		__func__, __LINE__, ##args, errno ? strerror(errno) : ""); \
	} while (0)
#else /* errno */
#define DFX_ERR(fmt, args ...) fprintf(stderr, \
		"DFX-MGRD> ERROR:%s():%u " fmt "\n", \
		__func__, __LINE__, ##args)
#endif /* errno */
#define DFX_PR(fmt, args ...) printf("DFX-MGRD> %s():%u " fmt "\n", \
		__func__, __LINE__, ##args)
#ifdef DEBUG
#define DFX_DBG DFX_PR
#else
#define DFX_DBG(fmt, args ...)
#endif /* DEBUG */

#ifdef __cplusplus
}
#endif

#endif /*  _ACAPD_PRINT_H */
