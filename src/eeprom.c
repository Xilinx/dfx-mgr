/*
 * Copyright (C) 2022 - 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>
#include <stdint.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/eeprom.h>

extern struct daemon_config config;

/**
 * str_trim() - Remove leading and trailing whitespace from string in-place
 * @str: String to trim
 *
 * Return: Pointer to trimmed string (within original buffer), or NULL if empty
 */
static char *str_trim(char *str)
{
	char *end;

	if (!str)
		return NULL;

	/* Trim leading whitespace */
	while (isspace((unsigned char)*str))
		str++;

	if (*str == '\0')
		return NULL;

	/* Trim trailing whitespace */
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		*end-- = '\0';

	return str;
}

static const char *known_separators[] = {": ", "-"};
#define NUM_SEPARATORS (sizeof(known_separators) / sizeof(known_separators[0]))

/**
 * try_read_board_from_eeprom() - Try to read board name from an EEPROM path
 * @path: Full sysfs glob path to EEPROM device
 * @board_name: Buffer to store the extracted board name
 * @size: Size of the buffer
 *
 * Finds EEPROM device using glob, then runs ipmi-fru with each known awk
 * separator until a board name is successfully extracted.
 *
 * Return: 0 on success (board name found), -1 on failure
 */
static int try_read_board_from_eeprom(const char *path, char *board_name, size_t size)
{
	glob_t glob_result;
	char cmd[512];
	char line[256];
	FILE *fp;
	char *trimmed;
	int glob_ret;
	size_t s;

	if (!path || !board_name || size == 0)
		return -1;

	memset(&glob_result, 0, sizeof(glob_result));
	glob_ret = glob(path, GLOB_NOSORT, NULL, &glob_result);
	if (glob_ret != 0 || glob_result.gl_pathc == 0) {
		if (glob_ret == 0)
			globfree(&glob_result);
		return -1;
	}

	for (s = 0; s < NUM_SEPARATORS; s++) {
		snprintf(cmd, sizeof(cmd),
				 "ipmi-fru --fru-file=%s --interpret-oem-data 2>/dev/null | "
				 "awk -F\"%s\" '/FRU Board Product/ {print $2}'",
				 glob_result.gl_pathv[0], known_separators[s]);

		fp = popen(cmd, "r");
		if (!fp)
			continue;

		if (fgets(line, sizeof(line), fp)) {
			trimmed = str_trim(line);
			if (trimmed && *trimmed != '\0') {
				snprintf(board_name, size, "%s", trimmed);
				pclose(fp);
				globfree(&glob_result);
				return 0;
			}
		}
		pclose(fp);
	}

	globfree(&glob_result);
	return -1;
}

/**
 * lookup_board_name() - Map a FRU product name through eeprom_name_map
 * @raw: FRU "Board Product Name"
 * @out: mapped token, or @raw when no key matches
 * @size: size of @out
 *
 * Return: 1 if a key is a case-insensitive substring of @raw, else 0.
 */
static int lookup_board_name(const char *raw, char *out, size_t size)
{
	int i;

	if (!raw || !out || size == 0)
		return 0;

	for (i = 0; i < config.eeprom_name_map_count; i++) {
		const char *key = config.eeprom_name_map_keys[i];
		const char *val = config.eeprom_name_map_vals[i];

		if (key && val && *key && strcasestr(raw, key) != NULL) {
			snprintf(out, size, "%s", val);
			return 1;
		}
	}

	snprintf(out, size, "%s", raw);
	return 0;
}

/**
 * append_token() - Append @tok to a space-separated list in @buf
 *
 * Skips an empty token, one already present as a whole word (overlapping
 * EEPROM globs), and one that does not fit. A short prefix is never stored.
 */
static void append_token(char *buf, size_t size, const char *tok)
{
	size_t tl;
	size_t len;
	const char *p;

	if (!tok || !tok[0])
		return;

	tl = strlen(tok);
	len = strlen(buf);
	p = buf;

	while ((p = strcasestr(p, tok)) != NULL) {
		if ((p == buf || p[-1] == ' ') && (p[tl] == '\0' || p[tl] == ' '))
			return;
		p++;
	}

	if (len == 0) {
		if (tl >= size)
			return;
		snprintf(buf, size, "%s", tok);
	} else if (len + 1 + tl < size) {
		snprintf(buf + len, size - len, " %s", tok);
	}
}

/**
 * read_board_name_from_eeprom() - Read board tokens from every configured EEPROM
 * @board_name: space-separated tokens, e.g. "k26 kr260"
 * @size: size of @board_name
 *
 * Every mapped EEPROM is kept, so probe order cannot drop the Kria carrier.
 * If none map, the first raw FRU string is kept (vek280, zcu10x).
 *
 * Return: 0 if a name was resolved, -1 otherwise.
 */
int read_board_name_from_eeprom(char *board_name, size_t size)
{
	char raw[128];
	char token[128];
	char fallback[128] = "";
	int i;

	if (!board_name || size == 0)
		return -1;

	if (config.num_eeprom_location <= 0 || config.eeprom_location == NULL) {
		DFX_PR("WARNING: No eeprom_location in daemon.conf, skipping EEPROM board detection");
		return -1;
	}

	board_name[0] = '\0';

	for (i = 0; i < config.num_eeprom_location; i++) {
		if (try_read_board_from_eeprom(config.eeprom_location[i], raw, sizeof(raw)) != 0)
			continue;

		if (lookup_board_name(raw, token, sizeof(token))) {
			DFX_PR("Board token from %s: %s (raw: %s)", config.eeprom_location[i], token, raw);
			append_token(board_name, size, token);
		} else if (!fallback[0]) {
			snprintf(fallback, sizeof(fallback), "%s", raw);
		}
	}

	if (board_name[0]) {
		DFX_PR("Board name: %s", board_name);
		return 0;
	}

	if (fallback[0]) {
		snprintf(board_name, size, "%s", fallback);
		DFX_PR("Board name (passthrough): %s", board_name);
		return 0;
	}

	DFX_DBG("Could not read board name from EEPROM");
	return -1;
}
