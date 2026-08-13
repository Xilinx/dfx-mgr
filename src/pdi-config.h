/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	pdi-config.h
 * @brief	Versal PDI-based design/accel metadata discovery helpers.
 *
 * The PDI counterpart to json-config.h: directory probes and the
 * partial-PDI -> accel_info_t derivation used when a Versal package ships a
 * .pdi instead of shell.json/accel.json.
 */

#ifndef _ACAPD_PDI_CONFIG_H
#define _ACAPD_PDI_CONFIG_H

#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/model.h>

/* One resolved RM slot directory (see scan_accel_slots()). */
#define PDI_SLOT_PATH_MAX 1024
struct pdi_slot {
	char path[PDI_SLOT_PATH_MAX]; /* the slot directory */
	int index;					  /* <accel>_slot<N> index, or -1 if not a slot dir */
	bool has_pdi; /* true only for a <accel>_slotN dir holding a PDI (register it) */
};

/**
 *
 * name_is_xclbin - test a filename for an xclbin extension
 * @name - filename to test
 *
 * This function checks whether @name ends in .xclbin or .XCLBIN
 *
 * return - true if @name has an xclbin extension
 * 	    false otherwise
 *
 * */
bool name_is_xclbin(const char *name);

/**
 *
 * dir_has_pdi - test for a PDI in a directory
 * @dir - directory to scan
 *
 * This function checks whether @dir directly contains a .pdi file
 *
 * return - true if a .pdi is present
 * 	    false otherwise
 *
 * */
bool dir_has_pdi(const char *dir);

/**
 *
 * dir_has_xclbin - test for an xclbin in a directory
 * @dir - directory to scan
 *
 * This function checks whether @dir directly contains an .xclbin file
 *
 * return - true if an .xclbin is present
 * 	    false otherwise
 *
 * */
bool dir_has_xclbin(const char *dir);

/**
 *
 * dir_has_shell_json - test for a shell.json in a directory
 * @dir - directory to check
 *
 * This function checks whether @dir directly contains a shell.json file
 *
 * return - true if shell.json is present
 * 	    false otherwise
 *
 * */
bool dir_has_shell_json(const char *dir);

/**
 *
 * slot_index_from_dir - decode the slot index a directory name encodes
 * @accel_name - accelerator the slot dir belongs to
 * @dir_name - candidate directory name
 *
 * This function returns the slot index N that @dir_name encodes when it matches
 * the <accel_name>_slot<N> layout with 0 <= N < RP_SLOTS_MAX
 *
 * return - slot index (>= 0) on match
 * 	    -1 if @dir_name is not a valid slot dir for @accel_name
 *
 * */
int slot_index_from_dir(const char *accel_name, const char *dir_name);

/**
 *
 * init_accel_from_pdi - fill an RM-slot accelerator from its partial PDI
 * @accel - accel entry to populate
 * @path - slot directory holding the partial .pdi
 *
 * This function is the Versal no-JSON counterpart to initAccel(): it derives
 * accel_type and the uid/pid the load path matches against from the RM's
 * partial PDI instead of accel.json
 *
 * return - 0 on success
 * 	    -1 if the PDI metaheader cannot be read
 *
 * */
int init_accel_from_pdi(accel_info_t *accel, const char *path);

/**
 *
 * scan_accel_slots - list the RM slot dirs under one accel dir
 * @accel_path - accel directory to scan
 * @accel_name - accel name used to match the <accel_name>_slot<N> layout
 * @out - filled with one entry per qualifying subdirectory
 * @max - capacity of @out
 *
 * This function records each immediate subdirectory of @accel_path in @out,
 * skipping ".", "..", over-long names and non-directories. An entry is a
 * loadable slot (has_pdi, index >= 0) only when the subdir holds a .pdi and is
 * named <accel_name>_slot<N>; otherwise it is recorded with index -1 for
 * watching only. Pure: no inotify
 *
 * return - number of entries written to @out (at most @max)
 *
 * */
int scan_accel_slots(const char *accel_path, const char *accel_name, struct pdi_slot *out, int max);

/**
 *
 * init_base_from_pdi - set a base's PDI-derived fields from its metaheader
 * @base - base design to populate
 *
 * This function sets base->uid from the PDI metaheader, marks the base loadable
 * (load_base_design), and zeroes type and slot counts so the registration walk
 * can finalize type and num_pl_slots via pdi_classify_base()
 *
 * return - 0 on success
 * 	    -1 if the PDI metaheader cannot be read
 *
 * */
int init_base_from_pdi(struct basePLDesign *base);

/**
 *
 * pdi_classify_base - finalize a base's type and slot count
 * @base - base design to classify
 * @slot_range - highest <accel>_slotN index + 1 the walk found (0 if none)
 * @base_path - base directory, probed for an xclbin
 *
 * This function marks the base flat with one slot by default (XRT_FLAT when an
 * xclbin is present, else PL_FLAT) and upgrades it to PL_DFX with @slot_range
 * slots when the walk found RM slot dirs (@slot_range > 0)
 *
 * return - void
 *
 * */
void pdi_classify_base(struct basePLDesign *base, int slot_range, const char *base_path);

#endif /* _ACAPD_PDI_CONFIG_H */
