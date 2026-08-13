/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	pdi-config.c
 * @brief	Versal PDI-based design/accel metadata discovery helpers.
 */

#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/pdi-config.h>
#include <dfx-mgr/pdi_parse.h>
#include <dfx-mgr/print.h>

/* RM slot dir layout, written once: <accel_path>/<accel_name>_slot<idx>. */
#define RM_SLOT_TAG "_slot"
#define RM_SLOT_TAG_LEN (sizeof(RM_SLOT_TAG) - 1)

bool name_is_xclbin(const char *name)
{
	size_t len = strlen(name);

	return len > 7 && (!strcmp(name + len - 7, ".xclbin") || !strcmp(name + len - 7, ".XCLBIN"));
}

int slot_index_from_dir(const char *accel_name, const char *dir_name)
{
	size_t nlen = strlen(accel_name);
	char *endptr;
	long idx;

	if (strncmp(dir_name, accel_name, nlen) ||
		strncmp(dir_name + nlen, RM_SLOT_TAG, RM_SLOT_TAG_LEN))
		return -1;
	/* strtol() would take "", "+1" or leading space; the index must be digits. */
	if (!isdigit((unsigned char)dir_name[nlen + RM_SLOT_TAG_LEN]))
		return -1;
	idx = strtol(dir_name + nlen + RM_SLOT_TAG_LEN, &endptr, 10);
	if (*endptr != '\0' || idx < 0 || idx >= RP_SLOTS_MAX)
		return -1;
	return (int)idx;
}

bool dir_has_pdi(const char *dir)
{
	struct dirent *dirent;
	DIR *d = opendir(dir);
	bool found = false;

	if (d == NULL)
		return false;

	while ((dirent = readdir(d)) != NULL) {
		if (name_is_pdi(dirent->d_name)) {
			found = true;
			break;
		}
	}
	closedir(d);
	return found;
}

bool dir_has_xclbin(const char *dir)
{
	struct dirent *dirent;
	bool found = false;
	DIR *d;

	d = opendir(dir);
	if (d == NULL)
		return false;

	while ((dirent = readdir(d)) != NULL) {
		if (name_is_xclbin(dirent->d_name)) {
			found = true;
			break;
		}
	}
	closedir(d);
	return found;
}

bool dir_has_shell_json(const char *dir)
{
	char fname[600];

	snprintf(fname, sizeof(fname), "%s/shell.json", dir);
	return access(fname, F_OK) == 0;
}

/*
 * pdi_accel_type() - accel_type string derived from the RM's PDI image classes:
 * "XRT_AIE_DFX" if any image is an AIE image, otherwise "XRT_PL_DFX". This is
 * the PDI equivalent of the accel_type that accel.json carries in the JSON flow.
 */
static const char *pdi_accel_type(const struct pdi_meta *meta)
{
	for (int i = 0; i < meta->num_images; i++) {
		uint32_t cls = meta->images[i].img_id & PDI_IMG_ID_CLASS_MASK;

		if (cls == PDI_IMG_ID_AIE) {
			DFX_DBG("PDI image %d: AIE class 0x%x", i, (unsigned)cls);
			return "XRT_AIE_DFX";
		}
		if (cls != PDI_IMG_ID_PL)
			DFX_ERR("PDI image %d: unknown class 0x%x, assuming PL", i, (unsigned)cls);
	}
	return "XRT_PL_DFX";
}

int init_accel_from_pdi(accel_info_t *accel, const char *path)
{
	struct pdi_meta meta;

	if (accel == NULL)
		return -1;

	if (pdi_parse_dir(path, &meta) < 0) {
		DFX_ERR("Failed to read PDI metaheader for %s", path);
		return -1;
	}

	strcpy(accel->accel_type, pdi_accel_type(&meta));
	accel->uid = meta.uid;
	accel->pid = meta.puid;

	DFX_DBG("PDI accel %s: type=%s uid=0x%x pid=0x%x", accel->name, accel->accel_type,
			(unsigned)accel->uid, (unsigned)accel->pid);
	return 0;
}

int scan_accel_slots(const char *accel_path, const char *accel_name, struct pdi_slot *out, int max)
{
	struct dirent *d;
	DIR *dir;
	int n = 0;

	if (max <= 0)
		return 0;

	dir = opendir(accel_path);
	if (dir == NULL)
		return 0;

	while ((d = readdir(dir)) != NULL && n < max) {
		const char *name = d->d_name;
		char sub[PDI_SLOT_PATH_MAX];
		struct stat sb;
		int idx;

		/* skip ".", "..", and over-long names */
		if (strlen(name) > ACCEL_NAME_MAX - 1 ||
			(name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))))
			continue;
		snprintf(sub, sizeof(sub), "%s/%s", accel_path, name);
		if (stat(sub, &sb) || !S_ISDIR(sb.st_mode))
			continue;

		/*
		 * A registerable slot needs both a partial PDI and a <accel>_slotN
		 * name; otherwise it is only watched (index -1), never registered as
		 * an accel the load path cannot address.
		 */
		idx = dir_has_pdi(sub) ? slot_index_from_dir(accel_name, d->d_name) : -1;
		out[n].has_pdi = idx >= 0;
		out[n].index = idx;
		snprintf(out[n].path, sizeof(out[n].path), "%s", sub);
		n++;
	}
	closedir(dir);
	return n;
}

int init_base_from_pdi(struct basePLDesign *base)
{
	struct pdi_meta meta;

	/*
	 * The PLM extracts the metaheader, so this only succeeds for a PDI built for
	 * this device: a design copied from another board is skipped, not listed.
	 */
	if (pdi_parse_dir(base->base_path, &meta) < 0) {
		DFX_ERR("Failed to read PDI metaheader for %s", base->base_path);
		return -1;
	}

	base->uid = meta.uid;
	base->num_aie_slots = 0; /* no PDI AIE-slot concept; AIE refused at load */
	base->load_base_design = 1;
	base->num_pl_slots = 0;
	base->type[0] = '\0'; /* decided by the registration walk */

	DFX_DBG("PDI base %s: uid=0x%x", base->name, (unsigned)base->uid);
	return 0;
}

void pdi_classify_base(struct basePLDesign *base, int slot_range, const char *base_path)
{
	strcpy(base->type, dir_has_xclbin(base_path) ? "XRT_FLAT" : "PL_FLAT");
	base->num_pl_slots = 1;
	if (slot_range > 0) {
		strcpy(base->type, "PL_DFX");
		base->num_pl_slots = slot_range;
	}
}
