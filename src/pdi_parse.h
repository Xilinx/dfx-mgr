/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    pdi_parse.h
 * @brief   Versal PDI metaheader parser via the PLM (Versal gen1/gen2 firmware sysfs).
 */

#ifndef _ACAPD_PDI_PARSE_H
#define _ACAPD_PDI_PARSE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Image Header Table (IHT): 128 bytes at the start of the buffer. */
#define PDI_IHT_LEN 0x80U
#define PDI_IHT_NUM_IMGS_OFF 0x04U

/*
 * Image Header (IH): 64 bytes each. In the PLM-normalised metaheader the first
 * IH sits immediately after the 128-byte IHT (the PLM zeroes the IHT's
 * optional-data length, so there is no optional-data block in between). Offsets
 * below are relative to the start of each image header.
 */
#define PDI_IH_LEN 0x40U
#define PDI_IH_IMG_ID_OFF 0x20U /* XilPM node ID; class+subclass in the top 12 bits */
#define PDI_IH_UID_OFF 0x24U	/* this image's UUID */
#define PDI_IH_PUID_OFF 0x28U	/* parent UUID (DFx compatibility) */

/* img_id class+subclass: XPM_NODESUBCL_DEV_PL and XPM_NODESUBCL_DEV_AIE. */
#define PDI_IMG_ID_CLASS_MASK 0xFFF00000U
#define PDI_IMG_ID_PL 0x18700000U
#define PDI_IMG_ID_AIE 0x18800000U

#define PDI_MAX_IMAGES 32

struct pdi_image {
	uint32_t img_id;
	uint32_t uid;
	uint32_t puid;
};

struct pdi_meta {
	uint32_t uid; /* primary (image 0) identity */
	uint32_t puid;
	int num_images;
	struct pdi_image images[PDI_MAX_IMAGES];
};

/*
 * pdi_parse_dir() - find a .pdi in @folder and decode its metaheader.
 *
 * Picks the lexicographically-first .pdi so the choice is stable when several
 * are present, then has the PLM extract its normalised metaheader (via the
 * Versal gen1/gen2 firmware sysfs nodes) and decodes that.
 *
 * Return: 0 on success, <0 when no .pdi is present or on a parse failure.
 */
int pdi_parse_dir(const char *folder, struct pdi_meta *out);

/*
 * versal_fw_init() - detect this SoC's firmware platform device (Versal gen1 vs
 * Versal gen2) and cache its sysfs base path.
 * Return: 0 on success, -1 if no supported firmware device node is present.
 */
int versal_fw_init(void);

/*
 * name_is_pdi() - true (non-zero) if @name ends with the ".pdi" extension.
 */
int name_is_pdi(const char *name);

#ifdef __cplusplus
}
#endif

#endif
