/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/print.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef DEBUG
void acapd_debug(const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	fprintf(stdout, "ACAPD> DEBUG: " );
	vfprintf(stdout, format, argptr);
	fprintf(stdout, "\n" );
	va_end(argptr);
}
#endif

void acapd_print(const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	fprintf(stdout, "ACAPD> " );
	vfprintf(stdout, format, argptr);
	fprintf(stdout, "\n" );
	va_end(argptr);
}

void acapd_perror(const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	fprintf(stderr, "ACAPD> ERROR: " );
	vfprintf(stderr, format, argptr);
	fprintf(stderr, "\n" );
	va_end(argptr);
}
