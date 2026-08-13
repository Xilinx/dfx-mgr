/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libdfx.h>
#include <dfx-mgr/pdi_parse.h>

/*
 * Scratch buffer for the PLM-normalised metaheader (IHT at offset 0), sized to
 * one 4 KB page. meta-header-read is a kernfs bin_attribute, so a single read()
 * returns at most PAGE_SIZE (4 KB); and the PLM metaheader itself (IHT + image
 * headers + partition headers, no optional data) tops out at
 * 128 + 64*XIH_MAX_IMGS + 128*XIH_MAX_PRTNS = 3328 B on Versal, so one page
 * always holds the complete metaheader in a single read.
 */
#define PDI_META_BUF_WORDS 1024 /* 4 KB (one page) */

/* Firmware device node attribute names (identical across all Versal generations). */
#define VERSAL_FW_TRIGGER_ATTR "firmware"
#define VERSAL_FW_METAHDR_ATTR "meta-header-read"

/* Firmware (PLM) sysfs node paths */
static struct versal_fw_paths {
	char trigger[PATH_MAX];
	char metahdr[PATH_MAX];
} versal_fw;

int versal_fw_init(void)
{
	static const char *const paths[] = {
		"/sys/devices/platform/firmware:versal-firmware", /* Versal gen1 */
		"/sys/devices/platform/firmware:versal-net-firmware",
		"/sys/devices/platform/firmware:versal2-firmware", /* Versal gen2 */
	};
	char trigger[PATH_MAX], metahdr[PATH_MAX];
	size_t i;
	int n;

	for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
		n = snprintf(metahdr, sizeof(metahdr), "%s/%s", paths[i], VERSAL_FW_METAHDR_ATTR);
		if (n < 0 || n >= (int)sizeof(metahdr))
			continue;
		n = snprintf(trigger, sizeof(trigger), "%s/%s", paths[i], VERSAL_FW_TRIGGER_ATTR);
		if (n < 0 || n >= (int)sizeof(trigger))
			continue;
		if (access(metahdr, F_OK) != 0)
			continue;
		snprintf(versal_fw.trigger, sizeof(versal_fw.trigger), "%s", trigger);
		snprintf(versal_fw.metahdr, sizeof(versal_fw.metahdr), "%s", metahdr);
		return 0;
	}
	return -1;
}

/* PDI header words are little-endian, and the buffer is unaligned. */
static uint32_t pdi_rd_u32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int pdi_parse_meta_buffer(const uint8_t *buf, int len, struct pdi_meta *out)
{
	int i, num, ih_base;

	if (buf == NULL || out == NULL || len < (int)PDI_IHT_LEN)
		return -1;

	memset(out, 0, sizeof(*out));

	num = (int)pdi_rd_u32(buf + PDI_IHT_NUM_IMGS_OFF);
	if (num < 0)
		return -1;
	if (num > PDI_MAX_IMAGES)
		num = PDI_MAX_IMAGES;

	/*
	 * The PLM emits IHT + image headers + partition headers back-to-back and
	 * zeroes the IHT optional-data length in its export mask, so the first
	 * image header sits immediately after the 128-byte IHT. There is no
	 * optional-data block to skip here (unlike a raw .pdi on flash).
	 */
	ih_base = (int)PDI_IHT_LEN;

	for (i = 0; i < num; i++) {
		int base = ih_base + i * (int)PDI_IH_LEN;
		struct pdi_image *img = &out->images[out->num_images];

		/*
		 * The count comes from the metaheader, so a truncated read or a bad header
		 * can claim more images than the buffer holds; stop at the last complete one.
		 */
		if (base + (int)PDI_IH_LEN > len)
			break;

		img->img_id = pdi_rd_u32(buf + base + PDI_IH_IMG_ID_OFF);
		img->uid = pdi_rd_u32(buf + base + PDI_IH_UID_OFF);
		img->puid = pdi_rd_u32(buf + base + PDI_IH_PUID_OFF);
		out->num_images++;
	}

	if (out->num_images == 0)
		return -1;

	/* Image 0 is the design image in a design or RM PDI (not in a boot PDI). */
	out->uid = out->images[0].uid;
	out->puid = out->images[0].puid;

	return 0;
}

int name_is_pdi(const char *name)
{
	size_t len;

	if (name == NULL)
		return 0;

	len = strlen(name);

	return len > 4 && (!strcmp(name + len - 4, ".pdi") || !strcmp(name + len - 4, ".PDI"));
}

/* Write @val to a sysfs control node. Returns 0 on success, -1 on error. */
static int pdi_sysfs_write(const char *node, const char *val)
{
	size_t len = strlen(val);
	int fd = open(node, O_WRONLY);
	ssize_t n;

	if (fd < 0)
		return -1;
	n = write(fd, val, len);
	close(fd);
	return (n == (ssize_t)len) ? 0 : -1;
}

/*
 * pdi_read_plm_metaheader() - copied from libdfx's dfx_get_meta_header() with
 * its read-loop bug corrected (a single bounded read() instead of a feof() loop
 * that never terminates and re-triggers the PLM, flooding the PMC console on a
 * rejected PDI).
 *
 * TODO: drop this and call dfx_get_meta_header() directly once the bug is fixed
 * in libdfx.
 *
 * Return: bytes read (>0) on success, -1 on error.
 */
static int pdi_read_plm_metaheader(const char *path, void *buffer, int buf_bytes)
{
	const char *slash, *base;
	int fd, ret = -1;
	ssize_t n;

	if (path == NULL || buffer == NULL || buf_bytes <= 0)
		return -1;

	/* Not resolved (non-Versal, or versal_fw_init() failed). */
	if (versal_fw.trigger[0] == '\0')
		return -1;

	slash = strrchr(path, '/');
	base = (slash != NULL) ? slash + 1 : path;

	if (dfx_set_firmware_search_path(path) < 0)
		return -1;

	if (pdi_sysfs_write(versal_fw.trigger, base) < 0)
		goto reset_path;

	fd = open(versal_fw.metahdr, O_RDONLY);
	if (fd < 0)
		goto reset_path;

	n = read(fd, buffer, (size_t)buf_bytes);
	close(fd);

	if (n > 0)
		ret = (int)n;

reset_path:
	dfx_set_firmware_search_path("");
	return ret;
}

static int pdi_parse_file(const char *path, struct pdi_meta *out)
{
	uint32_t buffer[PDI_META_BUF_WORDS];
	int ret;

	if (path == NULL || out == NULL)
		return -1;

	ret = pdi_read_plm_metaheader(path, buffer, (int)sizeof(buffer));
	if (ret < 0)
		return -1;

	return pdi_parse_meta_buffer((const uint8_t *)buffer, ret, out);
}

int pdi_parse_dir(const char *folder, struct pdi_meta *out)
{
	char best_name[NAME_MAX + 1] = "";
	char pdi_path[PATH_MAX];
	struct dirent *dirent;
	int found = 0;
	int n;
	DIR *d;

	if (folder == NULL || out == NULL)
		return -1;

	d = opendir(folder);
	if (d == NULL)
		return -1;

	/* readdir() order is unspecified; the lexicographic pick is stable. */
	while ((dirent = readdir(d)) != NULL) {
		if (!name_is_pdi(dirent->d_name))
			continue;
		if (!found || strcmp(dirent->d_name, best_name) < 0) {
			snprintf(best_name, sizeof(best_name), "%s", dirent->d_name);
			found = 1;
		}
	}
	closedir(d);

	if (!found)
		return -1;

	/* Reject a truncated path rather than open the wrong file. */
	n = snprintf(pdi_path, sizeof(pdi_path), "%s/%s", folder, best_name);
	if (n < 0 || n >= (int)sizeof(pdi_path))
		return -1;

	return pdi_parse_file(pdi_path, out);
}
