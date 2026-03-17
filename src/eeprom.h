/*
 * Copyright (C) 2022 - 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    eeprom.h
 */
#ifndef _ACAPD_EEPROM_H
#define _ACAPD_EEPROM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int read_board_name_from_eeprom(char *board_name, size_t size);

#ifdef __cplusplus
}
#endif

#endif
