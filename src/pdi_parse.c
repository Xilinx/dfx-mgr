/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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

	/*
	 * Take uid/puid from the PL image.
	 * If none match, use images[0].
	 */
	int pl_idx = 0;

	for (i = 0; i < out->num_images; i++) {
		if ((out->images[i].img_id & PDI_IMG_ID_CLASS_MASK) == PDI_IMG_ID_PL) {
			pl_idx = i;
			break;
		}
	}
	out->uid = out->images[pl_idx].uid;
	out->puid = out->images[pl_idx].puid;

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

static int pdi_parse_file(const char *path, struct pdi_meta *out)
{
	uint32_t buffer[PDI_META_BUF_WORDS];
	int ret;

	if (path == NULL || out == NULL)
		return -1;

	ret = dfx_get_meta_header((char *)path, (int *)buffer, (int)sizeof(buffer),
							  DFX_META_HEADER_IDCODE_BYPASS_FLAG);
	if (ret <= 0)
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

int pdi_filter_boot_pl_uid(const int *entries, int nbytes)
{
	int num_entries, i;

	if (entries == NULL || nbytes <= 0)
		return 0;

	num_entries = nbytes / (int)(PDI_UID_ENTRY_WORDS * sizeof(int));
	for (i = 0; i < num_entries; i++) {
		const int *entry = &entries[i * PDI_UID_ENTRY_WORDS];
		uint32_t img_id = (uint32_t)entry[0];
		uint32_t uid = (uint32_t)entry[1];
		uint32_t parent_uid = (uint32_t)entry[2];

		/*
		 * The boot.pdi PL image is the root PL entry: a populated UID, no parent
		 * (PUID == 0), and the PL image-subsystem class in ImgID. A loaded DFx
		 * child carries the boot PL image as its parent, so it fails the
		 * PUID == 0 test - this stays correct even after an overlay is loaded.
		 */
		if (parent_uid == 0 && uid != 0 && (img_id & PDI_IMG_ID_CLASS_MASK) == PDI_IMG_ID_PL)
			return (int)uid;
	}
	return 0;
}

int pdi_boot_pl_uid(void)
{
	int buffer[PDI_META_BUF_WORDS];
	int ret;

	ret = dfx_get_active_uid_list(buffer);
	if (ret <= 0)
		return 0;

	return pdi_filter_boot_pl_uid(buffer, ret);
}
