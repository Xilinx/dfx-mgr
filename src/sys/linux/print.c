/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/print.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
void acapd_debug(const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	fprintf(stdout, "DFX-MGRD> DEBUG: " );
	vfprintf(stdout, format, argptr);
	fprintf(stdout, "\n" );
	va_end(argptr);
}
#endif

void acapd_print(const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	fprintf(stdout, "DFX-MGRD> " );
	vfprintf(stdout, format, argptr);
	fprintf(stdout, "\n" );
	va_end(argptr);
}

void acapd_perror(const char *format, ...)
{
	int errnum = errno;
	va_list argptr;
	va_start(argptr, format);
	fprintf(stderr, "DFX-MGRD> ERROR: " );
	vfprintf(stderr, format, argptr);
	fprintf(stderr, "%s\n", errnum ? strerror(errnum) : "");
	va_end(argptr);
}
