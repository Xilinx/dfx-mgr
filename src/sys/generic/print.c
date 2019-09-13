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
	printf("ACAPD> DEBUG: " );
	vprintf(format, argptr);
	printf("\n" );
	va_end(argptr);
}
#endif

void acapd_print(const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	printf("ACAPD> " );
	vprintf(format, argptr);
	printf("\n" );
	va_end(argptr);
}

void acapd_perror(const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	printf("ACAPD> ERROR: " );
	vprintf(format, argptr);
	printf("\n" );
	va_end(argptr);
}
