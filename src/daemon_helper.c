/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/shell.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/json-config.h>
#include <dfx-mgr/daemon_helper.h>
#include <dfx-mgr/dfxmgr_client.h>
#include <dfx-mgr/eeprom.h>
#include <dfx-mgr/rpu.h>
#include <dfx-mgr/user_load.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <libdfx.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdatomic.h>

struct daemon_config config;
struct watch *active_watch = NULL;
struct basePLDesign *base_designs = NULL;
static int inotifyFd;
platform_info_t platform;
sem_t mutex;
static _Atomic bool pkg_listing_dirty = false;
static void firmware_dir_walk(void);
static void assign_list_ids(void);

bool is_pkg_listing_dirty(void)
{
	return pkg_listing_dirty;
}

struct watch {
	int wd;
	char name[ACCEL_NAME_MAX];
	char path[WATCH_PATH_LEN];
	char parent_name[ACCEL_NAME_MAX];
	char parent_path[WATCH_PATH_LEN];
};

/*
 * Filter: path a exceeds ACCEL_NAME_MAX-1, or is "." / ".."
 */
static int d_name_filter(char *a)
{
	return (strlen(a) > ACCEL_NAME_MAX - 1) ||
		   (a[0] == '.' && (a[1] == 0 || (a[1] == '.' && a[2] == 0)));
}

static int not_dir(char *path)
{
	struct stat sb;
	return stat(path, &sb) || !S_ISDIR(sb.st_mode);
}

struct basePLDesign *findBaseDesign(const char *name)
{
	int i, j;

	for (i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].base_path[0] != '\0') {
			if (!strcmp(base_designs[i].name, name)) {
				DFX_DBG("Found base design %s", base_designs[i].name);
				return &base_designs[i];
			}
			for (j = 0; j < RP_SLOTS_MAX; j++) {
				if (!strcmp(base_designs[i].accel_list[j].name, name)) {
					DFX_DBG("accel %s in base %s", name, base_designs[i].name);
					return &base_designs[i];
				}
			}
		}
	}
	DFX_ERR("No accel found for %s", name);
	return NULL;
}

struct basePLDesign *findBaseDesign_path(const char *path)
{
	int i, j;

	for (i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].base_path[0] != '\0') {
			if (!strcmp(base_designs[i].base_path, path)) {
				DFX_DBG("Found base design %s", base_designs[i].base_path);
				return &base_designs[i];
			}
			for (j = 0; j < RP_SLOTS_MAX; j++) {
				if (!strcmp(base_designs[i].accel_list[j].path, path)) {
					DFX_DBG("accel %s in base %s", path, base_designs[i].base_path);
					return &base_designs[i];
				}
			}
		}
	}
	DFX_ERR("No base design or accel found for %s", path);
	return NULL;
}

/**
 * find_slot_from_handle() - returns the slot number
 * @*base - Pointer to a base design
 * @slot_handle - slot handle number
 *
 * This function returns the slot number for the given
 * slot handle of a base
 *
 * Return: slot number
 *         -1 for slot not found
 */
int find_slot_from_handle(struct basePLDesign *base, int slot_handle)
{
	int slot = -1;
	for (int i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++) {
		if (base->slots[i]) {
			if (slot_handle == base->slots[i]->slot_handle) {
				slot = i;
				break;
			}
		}
	}
	DFX_DBG("slot %d slot_handle %d", slot, slot_handle);
	return slot;
}

/**
 * get_free_slot_handle() - get free slot handle
 *
 * This function returns a free slot handle from
 * available list
 *
 * Return: slot handle index on success
 *        -1 if no slots available
 */
int get_free_slot_handle()
{
	int slot_handle = -1;
	for (int index = 0; index < SLOT_HANDLE_MAX; index++) {
		if (platform.available_slot_handle[index] == 0) {
			platform.available_slot_handle[index] = 1;
			slot_handle = index;
			break;
		}
	}
	return slot_handle;
}

/*
 * Update Vitis AI Runtime (VART) env
 */
void update_env(char *path)
{
	DIR *FD;
	struct dirent *dir;
	int len, ret;
	char cmd[512];

	DFX_DBG("%s", path);
	FD = opendir(path);
	if (FD) {
		while ((dir = readdir(FD)) != NULL) {
			len = strlen(dir->d_name);
			if (len > 7) {
				if (!strcmp(dir->d_name + (len - 7), ".xclbin") ||
					!strcmp(dir->d_name + (len - 7), ".XCLBIN")) {
					snprintf(cmd, sizeof(cmd), "echo \"firmware: %s/%s\" > /etc/vart.conf", path,
							 dir->d_name);
					DFX_DBG("system %s", cmd);
					ret = system(cmd);
					if (ret)
						DFX_ERR("%s", cmd);
				}
			}
		}
		closedir(FD);
	}
}

/**
 * assign_slot() - Assign accelerator to slot and return handle
 * @base: Base design
 * @slot: Slot structure
 * @pl_accel: Accelerator structure
 * @accel_name: Accelerator name
 * @slot_num: Slot number
 *
 * Common slot bookkeeping for both user load and traditional paths.
 * Caller must set platform.active_base and update base->active before calling.
 *
 * Return: slot_handle on success
 */
static int assign_slot(struct basePLDesign *base, slot_info_t *slot, acapd_accel_t *pl_accel,
					   const char *accel_name, int slot_num)
{
	strncpy(slot->name, accel_name, sizeof(slot->name) - 1);
	slot->name[sizeof(slot->name) - 1] = '\0';
	slot->accel = pl_accel;
	slot->is_aie = 0;
	base->slots[slot_num] = slot;
	base->slots[slot_num]->slot_handle = get_free_slot_handle();
	DFX_PR("Loaded %s successfully to slot %d with slot_handle %d", accel_name, slot_num,
		   slot->slot_handle);

	return slot->slot_handle;
}

static int load_accelerator_core(struct basePLDesign *base, const char *accel_name, char *cma_path)
{
	int i, ret;
	int rv = -DFX_MGR_LOAD_ERROR;
	char path[1024];
	char shell_path[600];
	acapd_accel_t *pl_accel = (acapd_accel_t *)calloc(1, sizeof(acapd_accel_t));
	acapd_accel_pkg_hd_t *pkg = (acapd_accel_pkg_hd_t *)calloc(1, sizeof(acapd_accel_pkg_hd_t));
	slot_info_t *slot = (slot_info_t *)malloc(sizeof(slot_info_t));
	accel_info_t *accel_info = NULL;
	const char *resolved_cma = NULL;

	slot->accel = NULL;
	snprintf(shell_path, sizeof(shell_path), "%s/shell.json", base->base_path);

	/* Resolve CMA path priority: CLI > config > default */
	if (cma_path && cma_path[0] != '\0') {
		resolved_cma = cma_path;
	} else if (config.cma_path && config.cma_path[0] != '\0') {
		resolved_cma = config.cma_path;
	}
	DFX_PR("CMA path: %s", resolved_cma ? resolved_cma : "(default)");

	/* Flat shell design are treated as with one slot */
	if (!strcmp(base->type, "XRT_FLAT") || !strcmp(base->type, "PL_FLAT")) {
		if (base->slots == NULL)
			base->slots = calloc(base->num_pl_slots + base->num_aie_slots, sizeof(slot_info_t *));

		if (platform.active_base != NULL && platform.active_base->active > 0) {
			DFX_ERR("Unload previously loaded accelerator, no empty slot for %s", accel_name);
			rv = -DFX_MGR_NO_EMPTY_SLOT_ERROR;
			goto out;
		}
		snprintf(pkg->name, sizeof(pkg->name), "%s", accel_name);
		pkg->path = base->base_path;
		pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;

		/* Route to appropriate loading mechanism */
		if (platform.use_user_load_path) {
			DFX_PR("Using user load path for %s", accel_name);
			ret = user_load_from_dir(base->base_path, accel_name, 0);
			if (ret < 0) {
				DFX_ERR("load_accel %s", accel_name);
				base->active = 0;
				goto out;
			}
			user_load_init_accel(pl_accel, pkg, 0, base->type);
		} else {
			init_accel(pl_accel, pkg);
			strncpy(pl_accel->sys_info.tmp_dir, pkg->path, sizeof(pl_accel->sys_info.tmp_dir) - 1);
			strcpy(pl_accel->type, "XRT_FLAT");
			if (resolved_cma) {
				snprintf(pl_accel->cma_path, sizeof(pl_accel->cma_path), "%s", resolved_cma);
			}
			DFX_PR("load flat shell from %s", pkg->path);
			ret = load_accel(pl_accel, shell_path, 0);
			if (ret < 0) {
				DFX_ERR("load_accel %s", accel_name);
				base->active = 0;
				goto out;
			}
			pl_accel->rm_slot = 0;
			base->fpga_cfg_id = pl_accel->sys_info.fpga_cfg_id;
		}
		platform.active_base = base;
		base->active += 1;
		rv = assign_slot(base, slot, pl_accel, accel_name, 0);
		/* VART libary for SOM desings needs .xclbin path to be written to a file*/
		if (!strcmp(base->type, "XRT_FLAT"))
			update_env(pkg->path);
		return rv;
	}
	for (i = 0; i < RP_SLOTS_MAX; i++) {
		if (!strcmp(base->accel_list[i].name, accel_name)) {
			accel_info = &base->accel_list[i];
			DFX_DBG("Found accel %s base %s parent %s", accel_info->path, base->base_path,
					accel_info->parent_path);
		}
	}
	/* For slotted (PL_DFX/RPU) architecture */
	if (!strcmp(base->type, "PL_DFX") || !strcmp(base->type, "RPU")) {
		/* For base type PL_DFX active_base is used to store
		 * the current active PL base, below are checks for
		 * 1) Availability of slots
		 * 2) If the PL being loaded is of the active base
		 * 3) If base design needs to be loaded then its done
		 */
		if (!strcmp(base->type, "PL_DFX")) {
			if (platform.active_base != NULL && !platform.active_base->active &&
				strcmp(platform.active_base->base_path, accel_info->parent_path)) {
				DFX_PR("All slots for base are empty, loading new base design");
				remove_base(platform.active_base->fpga_cfg_id);
				free(platform.active_base->slots);
				platform.active_base->slots = NULL;
				platform.active_base = NULL;
			} else if (platform.active_base != NULL && platform.active_base->active > 0 &&
					   strcmp(platform.active_base->base_path, accel_info->parent_path)) {
				DFX_ERR("Active base design (%s) doesn't match accel base (%s)",
						platform.active_base->name, accel_info->parent_path);
				goto out;
			}
			if (platform.active_base == NULL) {
				snprintf(pkg->name, sizeof(pkg->name), "%s", base->name);
				pkg->path = base->base_path;
				pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
				if (base->load_base_design) {
					/* Route to appropriate loading mechanism for base design */
					if (platform.use_user_load_path) {
						DFX_PR("Using user load path for base design");
						ret = user_load_from_dir(base->base_path, base->name, 0);
						if (ret < 0) {
							DFX_ERR("load_accel %s", accel_name);
							base->active = 0;
							goto out;
						}
					} else {
						init_accel(pl_accel, pkg);
						strncpy(pl_accel->sys_info.tmp_dir, pkg->path,
								sizeof(pl_accel->sys_info.tmp_dir) - 1);
						strcpy(pl_accel->type, "PL_DFX");
						if (resolved_cma) {
							snprintf(pl_accel->cma_path, sizeof(pl_accel->cma_path), "%s",
									 resolved_cma);
						}
						DFX_PR("load from %s", pkg->path);
						ret = load_accel(pl_accel, shell_path, 0);
						if (ret < 0) {
							DFX_ERR("load_accel %s", accel_name);
							base->active = 0;
							goto out;
						}
						base->fpga_cfg_id = pl_accel->sys_info.fpga_cfg_id;
					}
					DFX_PR("Loaded %s successfully", base->name);
				}
				if (base->slots == NULL)
					base->slots =
						calloc(base->num_pl_slots + base->num_aie_slots, sizeof(slot_info_t *));
				platform.active_base = base;
			}
		} else if (!strcmp(base->type, "RPU")) {
			/* For base type RPU active_rpu_base is used to store
			 * the current active RPU base, below are checks for
			 * 1) Availability of slots
			 * 2) If the RPU being loaded is of the active RPU base
			 * base design loading is not supported in RPU case
			 */
			if (platform.active_rpu_base != NULL && !platform.active_rpu_base->active &&
				strcmp(platform.active_rpu_base->base_path, accel_info->parent_path)) {
				DFX_PR("All slots for rpu base are empty, loading new base design");
				remove_base(platform.active_rpu_base->fpga_cfg_id);
				free(platform.active_rpu_base->slots);
				platform.active_rpu_base->slots = NULL;
				platform.active_rpu_base = NULL;
			} else if (platform.active_rpu_base != NULL && platform.active_rpu_base->active > 0 &&
					   strcmp(platform.active_rpu_base->base_path, accel_info->parent_path)) {
				DFX_ERR("Active RPU base design (%s) doesn't match accel base (%s)",
						platform.active_rpu_base->name, accel_info->parent_path);
				goto out;
			}
			if (platform.active_rpu_base == NULL) {
				if (base->slots == NULL)
					base->slots =
						calloc(base->num_pl_slots + base->num_aie_slots, sizeof(slot_info_t *));
				platform.active_rpu_base = base;
			}
		}
		/*
		 * Check if base UID matches the parent_uid in accel_info
		 */
		DFX_DBG("base(%s).uid = %d does %s match accel(%s).pid = %d", base->name, base->uid,
				base->uid == accel_info->pid ? "" : "NOT", accel_info->name, accel_info->pid);

		/* For NEW RPU structure, slot is pre-determined during parse */
		if (!strcmp(base->type, RPU_TYPE_STR) && accel_info->rpu.slot_num >= 0) {
			/* Buffer holds accel name + ".elf" + NUL; sizeof(EXT) covers NUL. */
			char firmware_file[ACCEL_NAME_MAX + sizeof(RPU_FIRMWARE_EXT)];
			int target_slot = accel_info->rpu.slot_num;

			/* Validate slot availability using helper function */
			rv = validate_rpu_slot_availability(base, target_slot);
			if (rv < 0)
				goto out;

			snprintf(firmware_file, sizeof(firmware_file), "%s%s", accel_info->name,
					 RPU_FIRMWARE_EXT);
			ret = load_rpu(accel_info->path, target_slot, firmware_file);
			if (ret < 0) {
				DFX_ERR("load_rpu %s failed", accel_name);
				rv = -DFX_MGR_LOAD_ERROR;
				goto out;
			}

			free(pkg);
			rv = finalize_rpu_slot_setup(base, slot, pl_accel, accel_name, target_slot,
										 config.rpu_fw_uptime_msec);
			if (rv >= 0)
				platform.active_rpu_base->active += 1;
			return rv;
		}

		for (i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++) {
			DFX_DBG("Finding empty slot for %s i %d", accel_name, i);
			if (base->slots[i] == NULL) {
				snprintf(path, sizeof(path), "%s/%s_slot%d", accel_info->path, accel_info->name, i);
				if (access(path, F_OK) != 0) {
					continue;
				}

				if (!strcmp(base->type, "RPU")) {
					/*
					 * For basetype RPU
					 * Call load_rpu with path of firmware and rpu number(slot number)
					 * update slot with details
					 * increment active_rpu_base
					 */
					DFX_PR(
						"WARNING: Old RPU directory structure detected. "
						"Please migrate to the new numbered slot directory structure.");
					ret = load_rpu(path, i, NULL);
					if (ret < 0) {
						DFX_ERR("load_rpu %s failed", accel_name);
						goto out;
					}

					/* Complete RPU slot setup using common helper */
					free(pkg);
					rv = finalize_rpu_slot_setup(base, slot, pl_accel, accel_name, i,
												 config.rpu_fw_uptime_msec);
					if (rv >= 0)
						platform.active_rpu_base->active += 1;
					return rv;
				} else {
					/*
					 * For basetype PL_DFX
					 */
					if (!strcmp(accel_info->accel_type, "XRT_AIE_DFX")) {
						DFX_ERR("%s: XRT_AIE_DFX unsupported", accel_name);
						goto out;
					}
					strcpy(pkg->name, accel_name);
					pkg->path = path;
					pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;

					/* Route to appropriate loading mechanism for slot design */
					if (platform.use_user_load_path) {
						DFX_PR("Using user load path for slot design");
						ret = user_load_from_dir(path, accel_name, 1);
						if (ret < 0) {
							DFX_ERR("load_accel %s", accel_name);
							goto out;
						}
						user_load_init_accel(pl_accel, pkg, i, accel_info->accel_type);
					} else {
						init_accel(pl_accel, pkg);
						/* Set rm_slot before load_accel() so the appropriate slot is used */
						pl_accel->rm_slot = i;
						strcpy(pl_accel->type, accel_info->accel_type);
						strncpy(pl_accel->sys_info.tmp_dir, pkg->path,
								sizeof(pl_accel->sys_info.tmp_dir) - 1);
						if (resolved_cma) {
							snprintf(pl_accel->cma_path, sizeof(pl_accel->cma_path), "%s",
									 resolved_cma);
						}
						DFX_PR("load from %s", pkg->path);
						ret = load_accel(pl_accel, shell_path, 0);
						if (ret < 0) {
							DFX_ERR("load_accel %s", accel_name);
							goto out;
						}
					}
					platform.active_base->active += 1;
					return assign_slot(base, slot, pl_accel, accel_name, i);
				}
			}
		}
		if (i >= (base->num_pl_slots + base->num_aie_slots)) {
			DFX_ERR("No empty slot for %s", accel_name);
			rv = -DFX_MGR_NO_EMPTY_SLOT_ERROR;
		}
	} else {
		DFX_ERR("Check the supported type of base/accel");
	}
out:
	free(slot);
	free(pl_accel);
	free(pkg);
	return rv;
}

int load_accelerator(const char *accel_name, char *cma_path)
{
	struct basePLDesign *base;

	firmware_dir_walk();
	base = findBaseDesign(accel_name);
	if (base == NULL) {
		DFX_ERR("No package found for %s", accel_name);
		return -DFX_MGR_NO_PACKAGE_FOUND_ERROR;
	}
	return load_accelerator_core(base, accel_name, cma_path);
}

static int unload_accel_base(void)
{
	struct basePLDesign *base = platform.active_base;
	int ret = -1;

	if (!base)
		DFX_PR("No base");
	else if (strcmp(base->type, "PL_DFX"))
		DFX_PR("Invalid base type: %s", base->type);
	else if (base->active)
		DFX_PR("Can't unload active base PL design: %s", base->name);
	else {
		DFX_PR("Unload base: %s", base->name);
		/* For user load path, remove base overlay and skip libdfx call */
		if (!platform.use_user_load_path) {
			ret = remove_base(base->fpga_cfg_id);
		} else {
			char ov_dir[512];
			snprintf(ov_dir, sizeof(ov_dir), "%s/%s", DTBO_ROOT_DIR, base->name);
			remove_overlay_dir(ov_dir);
			ret = 0;
		}
		if (ret == 0) {
			free(base->slots);
			base->slots = NULL;
			platform.active_base = NULL;
		}
	}
	return ret;
}

/**
 * unload_accelerator() - unload accel or rpu from given slot handle
 * @slot_handle - slot handle number
 *
 * This function unloads an accel/rpu firmware from provided slot_handle
 * the slot_handle is mapped to slots for each accel/rpu
 *
 * Return:  0 on success
 *         -1 on failure
 */
int unload_accelerator(int slot_handle)
{
	struct basePLDesign *base = platform.active_base;
	struct basePLDesign *rpu_base = platform.active_rpu_base; /* get active rpu base */
	acapd_accel_t *accel;
	int ret = -1;
	int slot = -1;

	/* slot -1 means unload base PL design */
	if (slot_handle == -1)
		return unload_accel_base();

	/* check if base for pl or rpu is active */
	if ((!base && !rpu_base) || slot_handle < 0) {
		DFX_ERR("No accel or invalid slot_handle %d", slot_handle);
		return -1;
	}

	/*
	 * if rpu base is active and slot_handle is found in active rpu base
	 * then unload the rpu firmware by getting the slot number mapped to
	 * slot_handle
	 */
	if (rpu_base) {
		slot = find_slot_from_handle(rpu_base, slot_handle);
		if (slot != -1) {
			DFX_DBG("Unloading rpu %s from slot %d slot_handle %d", rpu_base->slots[slot]->name,
					slot, rpu_base->slots[slot]->slot_handle);
			ret = remove_rpu(slot);
			platform.available_slot_handle[slot_handle] = 0;
			/* delete rpmsg_dev_list from slot */
			delete_rpmsg_dev_list(&rpu_base->slots[slot]->rpu.rpmsg_dev_list);
			free(rpu_base->slots[slot]->accel);
			free(rpu_base->slots[slot]);
			rpu_base->slots[slot] = NULL;
			platform.active_rpu_base->active -= 1;
			return ret;
		}
	}

	/*
	 * if pl base is active and slot_handle is found in active pl base
	 * then unload accel by getting the slot number mapped to slot_handle
	 */
	if (base) {
		/* Call user_unload() if the design is user managed one */
		if (base->is_user_load)
			return user_unload(slot_handle);

		slot = find_slot_from_handle(base, slot_handle);
		if (slot != -1) {
			if (base->slots[slot]->is_aie) {
				free(base->slots[slot]);
				base->slots[slot] = NULL;
				platform.active_base->active -= 1;
				return 0;
			}
			accel = base->slots[slot]->accel;
			DFX_PR("Unloading accel %s from slot %d", accel->pkg->name, slot);

			if (platform.use_user_load_path) {
				char ov_dir[512];
				snprintf(ov_dir, sizeof(ov_dir), "%s/%s", DTBO_ROOT_DIR, accel->pkg->name);
				remove_overlay_dir(ov_dir);
				ret = 0;
			} else {
				ret = remove_accel(accel, 0);
			}

			platform.available_slot_handle[slot_handle] = 0;
			free(accel->pkg);
			free(accel);
			free(base->slots[slot]);
			base->slots[slot] = NULL;
			platform.active_base->active -= 1;
			if (!strcmp(platform.active_base->type, "XRT_FLAT") ||
				!strcmp(platform.active_base->type, "PL_FLAT")) {
				DFX_PR("Clearing platform active base for flat designs.");
				platform.active_base = NULL;
			}
		}
	}

	return ret;
}

static int find_slot_handle_by_name(const char *name)
{
	struct basePLDesign *rpu_base = platform.active_rpu_base;
	struct basePLDesign *base = platform.active_base;

	if (rpu_base && rpu_base->slots) {
		for (int i = 0; i < (rpu_base->num_pl_slots + rpu_base->num_aie_slots); i++) {
			if (rpu_base->slots[i] && !strcmp(rpu_base->slots[i]->name, name))
				return rpu_base->slots[i]->slot_handle;
		}
	}
	for (int i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].is_user_load && !strcmp(base_designs[i].name, name))
			return base_designs[i].user_load_handle;
	}
	if (base && base->slots) {
		for (int i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++) {
			if (base->slots[i] && !strcmp(base->slots[i]->name, name))
				return base->slots[i]->slot_handle;
		}
	}
	return -1;
}

/**
 * find_accel_by_list_id() - Find accelerator entry by its listPackage ID
 * @id: The list_id to search for
 * @result: Output struct populated on success
 *
 * Return: 0 on success, -1 if not found
 */
static int find_accel_by_list_id(int id, list_id_result_t *result)
{
	for (int i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].base_path[0] != '\0' && base_designs[i].num_pl_slots > 0) {
			for (int j = 0; j < RP_SLOTS_MAX; j++) {
				if (base_designs[i].accel_list[j].path[0] != '\0' &&
					base_designs[i].accel_list[j].list_id == id) {
					result->base = &base_designs[i];
					result->accel = &base_designs[i].accel_list[j];
					result->is_user_load = 0;
					result->slot_handle = -1;
					if (base_designs[i].slots) {
						int total = base_designs[i].num_pl_slots + base_designs[i].num_aie_slots;
						for (int k = 0; k < total; k++) {
							if (base_designs[i].slots[k] &&
								!strcmp(base_designs[i].slots[k]->name,
										base_designs[i].accel_list[j].name)) {
								result->slot_handle = base_designs[i].slots[k]->slot_handle;
								break;
							}
						}
					}
					return 0;
				}
			}
		}
		if (base_designs[i].is_user_load && base_designs[i].list_id == id) {
			result->base = &base_designs[i];
			result->accel = NULL;
			result->is_user_load = 1;
			result->slot_handle = base_designs[i].user_load_handle;
			return 0;
		}
	}
	return -1;
}

/**
 * load_accelerator_by_id() - Load accelerator by its listPackage ID
 * @id: The list_id from listPackage
 * @cma_path: Optional CMA device path (can be NULL)
 *
 * Resolves the ID to the exact base design via find_accel_by_list_id(),
 * then calls load_accelerator_core() directly, bypassing name-based
 * lookup to avoid ambiguity when multiple accelerators share the same name.
 *
 * Return: slot_handle on success, negative error code on failure
 */
int load_accelerator_by_id(int id, char *cma_path)
{
	list_id_result_t result;

	firmware_dir_walk();

	if (find_accel_by_list_id(id, &result) != 0) {
		DFX_ERR("No package found for ID %d", id);
		return -DFX_MGR_NO_PACKAGE_FOUND_ERROR;
	}

	if (result.is_user_load) {
		DFX_ERR("ID %d is a user-loaded design, cannot load via -load", id);
		return -DFX_MGR_LOAD_ERROR;
	}

	return load_accelerator_core(result.base, result.accel->name, cma_path);
}

/**
 * unload_accelerator_by_id() - Unload accelerator by its listPackage ID
 * @id: The list_id from listPackage
 *
 * Resolves the ID to find the loaded instance's slot_handle,
 * then delegates to unload_accelerator().
 *
 * Return: 0 on success, -1 on failure
 */
int unload_accelerator_by_id(int id)
{
	list_id_result_t result;

	if (id == 0)
		return unload_accel_base();

	firmware_dir_walk();

	if (find_accel_by_list_id(id, &result) != 0) {
		DFX_ERR("No package found for ID %d", id);
		return -1;
	}
	if (result.slot_handle < 0) {
		DFX_ERR("ID %d (%s) is not currently loaded", id,
				result.accel ? result.accel->name : "user_load");
		return -1;
	}
	return unload_accelerator(result.slot_handle);
}

/**
 * unload_accelerator_by_name() - Unload accelerator by name (first match)
 * @name: Accelerator name to unload
 *
 * Walks active PL base, RPU base, and user_load entries to find the first
 * loaded instance whose name matches, then unloads it.
 *
 * Return: 0 on success, -1 on failure
 */
int unload_accelerator_by_name(const char *name)
{
	int handle = find_slot_handle_by_name(name);

	if (handle >= 0)
		return unload_accelerator(handle);

	DFX_ERR("No loaded accelerator found for name %s", name);
	return -1;
}

/*
 * list_accel_uio  list uio device name and its path
 *
 * Note: dev_name + path < 80 char, see: acapd_device_t
 * and device_name limits in json. Reduce buf sise by
 * one line (80 char) to prevent buffer overrun.
 */
void list_accel_uio(int slot_handle, char *buf, size_t sz)
{
	struct basePLDesign *base = platform.active_base;
	struct basePLDesign *rpu_base = platform.active_rpu_base; /* get active rpu base */
	acapd_accel_t *accel;
	char *end = buf + sz - 80;
	char *p = buf;
	int i;
	int slot = -1;
	char *rpmsg_ctrl_dev;
	int virtio_num;
	acapd_list_t *rpmsg_dev_list;
	acapd_list_t *rpmsg_dev_node;

	/*Check if rpu is active in slot_handle*/
	if (rpu_base) {
		slot = find_slot_from_handle(rpu_base, slot_handle);
		if (slot != -1) {
			/* get virtio number of slot */
			virtio_num = rpu_base->slots[slot]->rpu.virtio_num;

			/* get rpmgs ctrl device name */
			rpmsg_ctrl_dev = rpu_base->slots[slot]->rpu.rpmsg_ctrl_dev_name;

			/* get stored list */
			rpmsg_dev_list = &rpu_base->slots[slot]->rpu.rpmsg_dev_list;

			/* update rpmsg_dev_list
			 * This function updates rpmsg_dev_list by parsing through /sys/bus/rpmsg/devices
			 * directory and does the following:
			 * 1) remove deleted rpmsg dev from list
			 * 2) create new rpmsg_dev_t for new dev found
			 *    a) setups up rpmsg end point dev for new rpmsg virtio dev and ctrl
			 *    b) setup rpmsg_channel name
			 * 3) add new entry to rpmsg dev list
			 */
			update_rpmsg_dev_list(rpmsg_dev_list, rpmsg_ctrl_dev, virtio_num);

			/* traverse through list and return channel name and dev name*/
			acapd_list_for_each (rpmsg_dev_list, rpmsg_dev_node) {
				rpmsg_dev_t *rpmsg_dev;

				rpmsg_dev =
					(rpmsg_dev_t *)acapd_container_of(rpmsg_dev_node, rpmsg_dev_t, rpmsg_node);
				p += sprintf(p, "%-30s %-30s\n", rpmsg_dev->rpmsg_channel_name,
							 rpmsg_dev->ept_rpmsg_dev_name);
			}

			return;
		}
	}

	/* if base is active get the slot from slot handle */
	if (base) {
		slot = find_slot_from_handle(base, slot_handle);
		if (slot == -1) {
			DFX_ERR("slot not found for slot_handle %d", slot_handle);
			return;
		}
	}

	if (base == NULL || base->slots == NULL)
		p += sprintf(p, "No active design\n");
	else if (slot >= base->num_pl_slots || !base->slots[slot])
		p += sprintf(p, "slot %d is not used or invalid\n", slot);
	else if ((accel = base->slots[slot]->accel) == NULL)
		p += sprintf(p, "No Accel in slot %d\n", slot);
	if (p != buf)
		return;

	for (i = 0; i < accel->num_ip_devs && p < end; i++) {
		char *dname = accel->ip_dev[i].dev_name;
		if (dname)
			p += sprintf(p, "%-30s %s\n", dname, accel->ip_dev[i].path);
	}
	dfx_shell_uio_list(p, end - p);
}

char *get_accel_uio_by_name(int slot_handle, const char *name)
{
	struct basePLDesign *base = platform.active_base;
	struct basePLDesign *rpu_base = platform.active_rpu_base; /* get active rpu base */
	acapd_accel_t *accel;
	int slot = -1;
	char *ept_dev = NULL;
	acapd_list_t *rpmsg_dev_list;
	acapd_list_t *rpmsg_dev_node;
	char *rpmsg_ctrl_dev;
	int virtio_num;

	if (rpu_base) {
		slot = find_slot_from_handle(rpu_base, slot_handle);
		if (slot != -1) {
			/* get virtio number of slot */
			virtio_num = rpu_base->slots[slot]->rpu.virtio_num;

			/* get rpmgs ctrl device name */
			rpmsg_ctrl_dev = rpu_base->slots[slot]->rpu.rpmsg_ctrl_dev_name;

			/* get stored list */
			rpmsg_dev_list = &rpu_base->slots[slot]->rpu.rpmsg_dev_list;

			/* update rpmsg_dev_list
			 * This function updates rpmsg_dev_list by parsing through /sys/bus/rpmsg/devices
			 * directory and does the following:
			 * 1) remove deleted rpmsg dev from list
			 * 2) create new rpmsg_dev_t for new dev found
			 *    a) setups up rpmsg end point dev for new rpmsg virtio dev and ctrl
			 *    b) setup rpmsg_channel name
			 * 3) add new entry to rpmsg dev list
			 */
			update_rpmsg_dev_list(rpmsg_dev_list, rpmsg_ctrl_dev, virtio_num);

			/* traverse throught the list */
			acapd_list_for_each (rpmsg_dev_list, rpmsg_dev_node) {
				rpmsg_dev_t *rpmsg_dev;
				rpmsg_dev =
					(rpmsg_dev_t *)acapd_container_of(rpmsg_dev_node, rpmsg_dev_t, rpmsg_node);

				/* get end point dev for given rpmsg channel name */
				if ((strlen(name) == strlen(rpmsg_dev->rpmsg_channel_name)) &&
					!strncmp(name, rpmsg_dev->rpmsg_channel_name, strlen(name))) {
					ept_dev = rpmsg_dev->ept_rpmsg_dev_name;
					break;
				}
			}

			/* return endpoint dev name */
			return ept_dev;
		}
	}

	if (base) {
		slot = find_slot_from_handle(base, slot_handle);
		if (slot == -1) {
			DFX_ERR("slot not found for slot_handle %d", slot_handle);
			return NULL;
		}
	}

	// NULL if no active design, slot is not used or invalid
	if (!base || !base->slots || slot >= base->num_pl_slots || !base->slots[slot])
		return NULL;

	accel = base->slots[slot]->accel;
	if (!accel)
		return NULL;

	for (int i = 0; i < accel->num_ip_devs; i++)
		if (strstr(accel->ip_dev[i].dev_name, name))
			return accel->ip_dev[i].path;

	// Check if the name matches one of the shell devices
	return dfx_shell_uio_by_name(name);
}

/*
 * pid_uid_check - compare PID with base UID
 * @base: base to get its uid
 * @accel_idx: index of the accel: its pid should match uid of the base
 *
 * Returns:
 *	"id_ok"  - when PID and UID are present and match, or *FLAT shells
 *	"id_err" - when PID and UID are present, but do not match
 *	"no_id"  - when either PID or UID are not present.
 */
static const char *pid_uid_check(struct basePLDesign *base, int accel_idx)
{
	int base_uid = base->uid;
	int pid = base->accel_list[accel_idx].pid;
	static const char p_noid[] = "no_id";
	static const char p_err[] = "id_err";
	static const char p_ok[] = "id_ok";
	const char *str = p_noid;

	if (!strcmp(base->type, "XRT_FLAT") || !strcmp(base->type, "PL_FLAT"))
		str = p_ok;
	else if (base_uid && pid)
		str = (base_uid == pid) ? p_ok : p_err;

	return str;
}

/**
 * assign_list_ids() - Assign sequential IDs to all listPackage entries
 *
 * Iterates base_designs[] in the same order as listAccelerators() and stamps
 * a sequential list_id on each entry. Called after package list changes.
 */
static void assign_list_ids(void)
{
	int id = 1;

	if (base_designs == NULL)
		return;

	for (int i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].base_path[0] != '\0' && base_designs[i].num_pl_slots > 0) {
			for (int j = 0; j < RP_SLOTS_MAX; j++) {
				if (base_designs[i].accel_list[j].path[0] != '\0')
					base_designs[i].accel_list[j].list_id = id++;
			}
		}
		if (base_designs[i].is_user_load)
			base_designs[i].list_id = id++;
	}
}

static void mark_pkg_listing_dirty(void)
{
	assign_list_ids();
	pkg_listing_dirty = true;
	DFX_PR("Package list updated - IDs have been reassigned");
}

char *listAccelerators(int flag)
{
	int i, j;
	uint8_t slot;
	char msg[350];
	char res[8 * 1024];
	char show_slots[16];
	char truncated_base[TRUNCATED_BASE_BUFFER_SIZE];
	int show_all = flag & LIST_PKG_SHOW_ALL;
	int use_filter = flag & LIST_PKG_FILTER;

	const char *header_format, *sub_header_format, *entry_format;

	if (show_all) {
		header_format = "%-3s%-12s%-10s%-10s%-12s%-6s%-13s%-9s%-7s%s\n";
		sub_header_format = "%-3s%-12s%-10s%-10s%-12s%-6s%-13s%-9s%-7s%s\n";
		entry_format = "%2d %-12s%-10s%-10s%-12s%-6s%-13s%-9s%-7s%s\n";
	} else {
		header_format = "%-3s%-12s%-12s%-8s%s\n";
		sub_header_format = NULL;
		entry_format = "%2d %-12s%-12s%-8s%s\n";
	}

	memset(res, 0, sizeof(res));
	firmware_dir_walk();
	assign_list_ids();
	pkg_listing_dirty = false;

	/* Check if filtering should be applied */
	if (use_filter &&
		(!platform.boardName[0] || strcasecmp(platform.boardName, DEFAULT_BOARD_NAME) == 0)) {
		snprintf(
			msg, sizeof(msg),
			"Error: Unable to read board name from EEPROM. Filter requires valid board name.\n");
		strcat(res, msg);
		return strdup(res);
	}

	if (show_all) {
		/* Row 1: Main headers */
		snprintf(msg, sizeof(msg), header_format, "ID", "accelType", "userLoad", "userLoad", "Base",
				 "Pid", "#slots", "slot", "load", "Accelerator");
		strcat(res, msg);
		/* Row 2: Sub-headers */
		snprintf(msg, sizeof(msg), sub_header_format, "", "", "type", "Region", "", "",
				 "(RPU+PL+AIE)", "Location", "Handle", "");
		strcat(res, msg);
		/* Separator line */
		snprintf(msg, sizeof(msg), header_format, "--", "-----------", "---------", "---------",
				 "-----------", "-----", "------------", "--------", "------", "-----------");
		strcat(res, msg);
	} else {
		snprintf(msg, sizeof(msg), header_format, "ID", "accelType", "Base", "slotLoc",
				 "Accelerator");
		strcat(res, msg);
		snprintf(msg, sizeof(msg), header_format, "--", "-----------", "-----------", "-------",
				 "-----------");
		strcat(res, msg);
	}
	for (i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].base_path[0] != '\0' && base_designs[i].num_pl_slots > 0) {
			for (j = 0; j < RP_SLOTS_MAX; j++) {
				if (base_designs[i].accel_list[j].path[0] != '\0') {
					char slot_locs[16] = "";
					char load_handles[16] = "";
					const char *accel_name = base_designs[i].accel_list[j].name;

					/* Apply filter: skip if boardName not found in accelerator name */
					if (use_filter && !strcasestr(accel_name, platform.boardName))
						continue;

					/*
					 * For RPU num_pl_slots is used for RPU slot number
					 * For PL  num_pl_slots is used for PL slot number
					 */
					if (!strcmp(base_designs[i].type, "RPU")) {
						sprintf(show_slots, "(%d+0+0)", base_designs[i].num_pl_slots);
					} else {
						/*
						 * Internally flat shell is treated as one slot to make
						 * the code generic and save info of the active design
						 */
						sprintf(show_slots, "(0+%d+%d)",
								!strcmp(base_designs[i].type, "XRT_FLAT") ||
										!strcmp(base_designs[i].type, "PL_FLAT")
									? 0
									: base_designs[i].num_pl_slots,
								base_designs[i].num_aie_slots);
					}

					if (base_designs[i].active) {
						for (slot = 0;
							 slot < (base_designs[i].num_pl_slots + base_designs[i].num_aie_slots);
							 slot++) {
							slot_info_t *slot_info = base_designs[i].slots[slot];
							int matched = 0;

							if (slot_info == NULL)
								continue;

							if ((slot_info->is_aie || slot_info->is_rpu) &&
								!strcmp(slot_info->name, accel_name)) {
								matched = 1;
							} else if (!slot_info->is_aie && !slot_info->is_rpu &&
									   slot_info->accel != NULL &&
									   !strcmp(slot_info->accel->pkg->name, accel_name)) {
								matched = 1;
							}

							if (matched) {
								int len = strlen(slot_locs);
								snprintf(slot_locs + len, sizeof(slot_locs) - len, "%d,", slot);
								len = strlen(load_handles);
								snprintf(load_handles + len, sizeof(load_handles) - len, "%d,",
										 slot_info->slot_handle);
							}
						}
					}
					if (slot_locs[0])
						slot_locs[strlen(slot_locs) - 1] = '\0';
					if (load_handles[0])
						load_handles[strlen(load_handles) - 1] = '\0';
					if (strlen(base_designs[i].name) > MAX_BASE_NAME_DISPLAY_LEN) {
						snprintf(truncated_base, sizeof(truncated_base), "%.*s...",
								 TRUNCATED_BASE_NAME_LEN, base_designs[i].name);
					} else {
						strncpy(truncated_base, base_designs[i].name, sizeof(truncated_base) - 1);
					}

					if (show_all) {
						snprintf(msg, sizeof(msg), entry_format,
								 base_designs[i].accel_list[j].list_id,
								 base_designs[i].accel_list[j].accel_type, "-", "-", truncated_base,
								 pid_uid_check(&base_designs[i], j), show_slots,
								 slot_locs[0] ? slot_locs : "-1",
								 load_handles[0] ? load_handles : "-1", accel_name);
					} else {
						snprintf(msg, sizeof(msg), entry_format,
								 base_designs[i].accel_list[j].list_id,
								 base_designs[i].accel_list[j].accel_type, truncated_base,
								 slot_locs[0] ? slot_locs : "-1", accel_name);
					}
					strcat(res, msg);
				}
			}
		}
		if (base_designs[i].is_user_load) {
			char handle_str[16];
			snprintf(handle_str, sizeof(handle_str), "%d", base_designs[i].user_load_handle);

			if (show_all) {
				snprintf(msg, sizeof(msg), entry_format, base_designs[i].list_id, "-",
						 base_designs[i].user_load_type ? "Partial" : "Full",
						 base_designs[i].user_load_region, "-", "-", "-", "-", handle_str,
						 base_designs[i].name);
			} else {
				snprintf(msg, sizeof(msg), entry_format, base_designs[i].list_id, "-", "-", "-",
						 base_designs[i].name);
			}
			strcat(res, msg);
		}
	}

	return strdup(res);
}

void add_to_watch(int wd, char *name, char *path, char *parent_name, char *parent_path)
{
	int i;
	for (i = 0; i < MAX_WATCH; i++) {
		if (active_watch[i].wd == -1) {
			active_watch[i].wd = wd;
			strncpy(active_watch[i].name, name, sizeof(active_watch[i].name) - 1);
			strncpy(active_watch[i].path, path, WATCH_PATH_LEN - 1);
			strncpy(active_watch[i].parent_name, parent_name,
					sizeof(active_watch[i].parent_name) - 1);
			strncpy(active_watch[i].parent_path, parent_path, WATCH_PATH_LEN - 1);
			return;
		}
	}
	DFX_ERR("no room to add more watch for path %s", path);
}

struct watch *get_watch(int wd)
{
	int i;
	for (i = 0; i < MAX_WATCH; i++) {
		if (active_watch[i].wd == wd)
			return &active_watch[i];
	}
	return NULL;
}
struct watch *path_to_watch(char *path)
{
	int i;
	for (i = 0; i < MAX_WATCH; i++) {
		if (!strcmp(path, active_watch[i].path))
			return &active_watch[i];
	}
	return NULL;
}
void remove_watch(char *path)
{
	int i;
	for (i = 0; i < MAX_WATCH; i++) {
		if (strcmp(active_watch[i].path, path) == 0) {
			inotify_rm_watch(inotifyFd, active_watch[i].wd);
			active_watch[i].wd = -1;
			active_watch[i].path[0] = '\0';
		}
	}
}
accel_info_t *add_accel_to_base(struct basePLDesign *base, char *name, char *path,
								char *parent_path)
{
	int j;

	for (j = 0; j < RP_SLOTS_MAX; j++) {
		/*
		 * Check for duplicates: same name AND same path
		 *
		 * Changed from path-only check to name+path check to support new RPU structure
		 * where multiple firmware files (.elf) can exist in the same slot directory.
		 * Old structure: each firmware in separate directory (path uniquely identifies entry)
		 * New structure: multiple firmwares per slot dir (need name+path for uniqueness)
		 */
		if (base->accel_list[j].path[0] != '\0' && !strcmp(base->accel_list[j].path, path) &&
			!strcmp(base->accel_list[j].name, name)) {
			DFX_DBG("%s at %s already exists", name, path);
			break;
		}
		if (base->accel_list[j].path[0] == '\0') {
			DFX_DBG("adding %s to base %s", path, parent_path);
			snprintf(base->accel_list[j].name, sizeof(base->accel_list[j].name), "%s", name);
			strcpy(base->accel_list[j].path, path);
			base->accel_list[j].path[sizeof(base->accel_list[j].path) - 1] = '\0';
			strcpy(base->accel_list[j].parent_path, parent_path);
			base->accel_list[j].parent_path[sizeof(base->accel_list[j].parent_path) - 1] = '\0';
			base->accel_list[j].rpu.slot_num = -1; /* Initialize to -1 for old structure */
			break;
		}
	}
	return &base->accel_list[j];
}

/*
 * The present limitation is only one level of hierarchy (dir1 and dir2).
 * In future when we face hierarchical designs that may have RPs within
 * RMs we will need this scalability.
 */
void parse_packages(struct basePLDesign *base, char *fname, char *path)
{
	DIR *dir1 = NULL, *dir2 = NULL;
	struct dirent *d1, *d2;
	char first_level[512], second_level[800], filename[811];
	struct stat stat_info;
	accel_info_t *accel;
	int wd;

	/* For flat shell design there is no subfolder so assign the base path as the accel path */
	if (!strcmp(base->type, "XRT_FLAT") || !strcmp(base->type, "PL_FLAT")) {
		DFX_DBG("%s : %s", base->name, base->type);
		snprintf(base->accel_list[0].name, sizeof(base->accel_list[0].name), "%s", base->name);
		strcpy(base->accel_list[0].path, base->base_path);
		strcpy(base->accel_list[0].parent_path, base->base_path);
		strcpy(base->accel_list[0].accel_type, base->type);
		return;
	}

	/* For RPU, detect structure type on-the-fly during first scan */
	dir1 = opendir(path);
	if (dir1 == NULL) {
		DFX_ERR("Directory %s not found", path);
		return;
	}
	while ((d1 = readdir(dir1)) != NULL) {
		sprintf(first_level, "%s/%s", path, d1->d_name);
		if (d_name_filter(d1->d_name) || not_dir(first_level))
			continue;

		/* For RPU: detect new structure (numeric dir with .elf files) inline */
		if (!strcmp(base->type, RPU_TYPE_STR) && isdigit(d1->d_name[0])) {
			char *endptr;
			long slot_num = strtol(d1->d_name, &endptr, 10);
			if (*endptr == '\0') {
				parse_rpu_slot_dir(base, first_level, (int)slot_num, path);
				continue;
			}
		}

		if (path_to_watch(first_level) == NULL) {
			wd = inotify_add_watch(inotifyFd, first_level, IN_ALL_EVENTS);
			if (wd == -1) {
				DFX_ERR("inotify_add_watch failed on %s", first_level);
				goto close_dir;
			}
			add_to_watch(wd, d1->d_name, first_level, fname, path);
		}

		sprintf(filename, "%s/accel.json", first_level);
		/* For pl slots we need to traverse next level to find accel.json*/
		if (stat(filename, &stat_info)) {
			dir2 = opendir(first_level);
			if (dir2 == NULL) {
				DFX_ERR("opendir failed on %s", first_level);
				continue;
			}
			while ((d2 = readdir(dir2)) != NULL) {
				sprintf(second_level, "%s/%s", first_level, d2->d_name);
				if (d_name_filter(d2->d_name) || not_dir(second_level))
					continue;

				if (path_to_watch(second_level) == NULL) {
					wd = inotify_add_watch(inotifyFd, second_level, IN_ALL_EVENTS);
					if (wd == -1) {
						DFX_ERR("inotify_add_watch failed on %s", second_level);
						goto close_dir;
					}
					add_to_watch(wd, d2->d_name, second_level, d1->d_name, first_level);
				}

				sprintf(filename, "%s/accel.json", second_level);
				if (!stat(filename, &stat_info)) {
					accel = add_accel_to_base(base, d1->d_name, first_level, path);
					initAccel(accel, second_level);
				}
			}
			closedir(dir2);
			dir2 = NULL;
			/* accel.json file not there for RPU
			 * Check if base-type is "RPU"
			 */
			if (!strcmp(base->type, "RPU")) {
				accel = add_accel_to_base(base, d1->d_name, first_level, path);
				/* accel_type is set inside initAccel in the case of PL
				 * by reading the accel.json file, since for RPU we do not
				 * have accel.json file, we set the accel_type to RPU here
				 */
				strcpy(accel->accel_type, base->type);
			}
		}
		/* Found accel.json so add it*/
		else {
			accel = add_accel_to_base(base, d1->d_name, first_level, path);
			initAccel(accel, first_level);
		}
	}
close_dir:
	if (dir1)
		closedir(dir1);
	if (dir2)
		closedir(dir2);
}

void add_base_design(char *name, char *path, char *parent, int wd)
{
	int i;

	for (i = 0; i < MAX_WATCH; i++) {
		if (!strcmp(base_designs[i].base_path, path)) {
			DFX_DBG("Base design %s already exists", path);
			return;
		}
	}

	// Now find the fist unsued base_designs[] element
	for (i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].base_path[0] == '\0') {
			DFX_DBG("Adding base design %s", path);
			strncpy(base_designs[i].name, name, sizeof(base_designs[i].name) - 1);
			base_designs[i].name[sizeof(base_designs[i].name) - 1] = '\0';
			strncpy(base_designs[i].base_path, path, sizeof(base_designs[i].base_path) - 1);
			base_designs[i].base_path[sizeof(base_designs[i].base_path) - 1] = '\0';
			strncpy(base_designs[i].parent_path, parent, sizeof(base_designs[i].parent_path) - 1);
			base_designs[i].parent_path[sizeof(base_designs[i].parent_path) - 1] = '\0';
			base_designs[i].active = 0;
			base_designs[i].wd = wd;
			return;
		}
	}
}
void remove_base_design(char *path, char *parent, int is_base)
{
	int i, j;

	if (!is_base) {
		DFX_DBG("Removing accel %s from base %s", path, parent);
		for (i = 0; i < MAX_WATCH; i++) {
			if (!strcmp(base_designs[i].base_path, parent)) {
				for (j = 0; j < RP_SLOTS_MAX; j++) {
					if (!strcmp(base_designs[i].accel_list[j].path, path)) {
						remove_watch(path);
						base_designs[i].accel_list[j].name[0] = '\0';
						base_designs[i].accel_list[j].path[0] = '\0';
						base_designs[i].accel_list[j].parent_path[0] = '\0';
						base_designs[i].accel_list[j].wd = -1;
						break;
					}
				}
			}
		}
		return;
	}
	for (i = 0; i < MAX_WATCH; i++) {
		if (strcmp(base_designs[i].base_path, path) == 0) {
			DFX_DBG("Removing base design %s", path);
			for (j = 0; j < RP_SLOTS_MAX; j++) {
				if (base_designs[i].accel_list[j].path[0] != '\0') {
					remove_watch(path);
					base_designs[i].accel_list[j].name[0] = '\0';
					base_designs[i].accel_list[j].path[0] = '\0';
					base_designs[i].accel_list[j].parent_path[0] = '\0';
					base_designs[i].accel_list[j].wd = -1;
				}
			}
			base_designs[i].name[0] = '\0';
			base_designs[i].type[0] = '\0';
			base_designs[i].base_path[0] = '\0';
			base_designs[i].num_pl_slots = 0;
			base_designs[i].active = 0;
			base_designs[i].wd = -1;
			return;
		}
	}
}

#define MAX_RECURSION_DEPTH 5
/*
 * accel_dir_add() - set up inotify watches for a directory, and check for base designs
 *
 * @cpath: The path for parent directory.
 * @dirent: Pointer to a struct dirent representing the directory entry to add.
 * @recursion_depth: The current recursion depth, used to prevent excessive recursion.
 *
 * This function is called recursively to handle subdirectories.
 * It checks if the directory is already being watched, adds a watch if not,
 * and initializes base designs if applicable.
 */
static void accel_dir_add(char *cpath, struct dirent *dirent, int recursion_depth)
{
	char new_dir[512], fname[600];
	struct basePLDesign *base;
	char *d_name;
	int wd;
	struct watch *watch = NULL;

	if (!cpath || !dirent || d_name_filter(dirent->d_name))
		return;

	d_name = dirent->d_name;
	sprintf(new_dir, "%s/%s", cpath, d_name);
	if (not_dir(new_dir))
		return;

	if (recursion_depth > MAX_RECURSION_DEPTH) {
		DFX_PR("Max recursion depth reached for %s. Not processing files any deeper.", new_dir);
		return;
	}

	DFX_DBG("Found dir %s", new_dir);

	watch = path_to_watch(new_dir);

	if (watch) {
		wd = watch->wd;
		if (strcmp(watch->parent_name, "")) {
			/* Update parent_name as empty string for base and parent directories */
			strcpy(watch->parent_name, "");
		}
	} else {
		wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
		if (wd == -1) {
			DFX_ERR("inotify_add_watch %s", new_dir);
			return;
		}
		add_to_watch(wd, d_name, new_dir, "", cpath);
	}

	/* add base design if shell.json exists or if the directory name is RPU or rpu */
	sprintf(fname, "%s/shell.json", new_dir);
	if (access(fname, F_OK) == 0 || !strcmp(d_name, "RPU") || !strcmp(d_name, "rpu")) {
		/* add the base design */
		add_base_design(d_name, new_dir, cpath, wd);

		/* initialize the base design */
		base = findBaseDesign_path(new_dir);
		if (base) {
			if (initBaseDesign(base, fname) == 0) {
				parse_packages(base, base->name, base->base_path);
			} else {
				DFX_ERR("Failed to init base design %s", d_name);
			}
		} else {
			DFX_ERR("Base design %s not found", d_name);
		}
	} else {
		/* check for subdirectories recursively */
		DIR *d = opendir(new_dir);

		if (d != NULL) {
			struct dirent *subdir;

			while ((subdir = readdir(d)) != NULL) {
				accel_dir_add(new_dir, subdir, recursion_depth + 1);
			}
			closedir(d);
		}
	}
}

/*
 * firmware_dir_walk()
 * For each config.firmware_locations, add a watch for all events
 */
static void firmware_dir_walk(void)
{
	int k, wd;
	struct dirent *dirent;

	for (k = 0; k < config.number_locations; k++) {
		char *fwdir = config.firmware_locations[k];
		DIR *d = opendir(fwdir);

		if (d == NULL) {
			DFX_ERR("opendir(%s)", fwdir);
			continue;
		}
		if (!path_to_watch(fwdir)) {
			wd = inotify_add_watch(inotifyFd, fwdir, IN_ALL_EVENTS);
			if (wd == -1) {
				DFX_ERR("inotify_add_watch(%s)", fwdir);
				closedir(d);
				continue;
			}
			add_to_watch(wd, "", fwdir, "", "");
		}
		/* Add packages to inotify */
		while ((dirent = readdir(d)) != NULL)
			accel_dir_add(fwdir, dirent, 1);
		closedir(d);
	}
}

/**
 * user_load() - load pl bitstream with optional device tree overlay.
 *
 * @flag:     Integer flag to control loading behavior.
 *                 - Bit 0: 0 = full bitstream, 1 = partial bitstream.
 *                 - Bit 1: 1 = load bitstream along with device tree overlay,
 *                          0 = load bitstream alone via sysfs interface.
 * @binfile:  Path to the bitstream file to be loaded.
 * @overlay:  Path to the device tree overlay file (if applicable).
 * @region:   Target region for the overlay (if applicable).
 *
 * This function handles the loading of a pl bitstream to the FPGA.
 * It uses the sysfs interface for loading bitstream alone, and the configfs interface
 * for loading bitstream with a device tree overlay.
 * It also add the loaded design to the base_designs array for tracking.
 * It also handles cleanup of temporary files and error conditions.
 *
 * This function is used by the daemon to load user-managed designs while it process
 * the request for USER_LOAD from client application.
 *
 * Return: An integer unique handle id on success,
 *        -1 on failure or if constraints are violated.
 */
int user_load(const int flag, const char *binfile, const char *overlay, const char *region)
{
	const char *bin;
	int rv = -1;

	if (platform.active_base != NULL) {
		if (platform.active_base->is_user_load) {
			if (!(flag & USER_LOAD_PARTIAL)) {
				DFX_ERR("Unload previously loaded full bitstream, no empty slot.");
				goto ret;
			} else if (platform.active_base->active >= MAX_WATCH) {
				DFX_ERR("Unload previously loaded partial bitstream, no empty slot.");
				goto ret;
			}
		}

		if (!strcmp(platform.active_base->type, "XRT_FLAT") ||
			!strcmp(platform.active_base->type, "PL_FLAT")) {
			DFX_ERR("Unload previously loaded FLAT design, no empty slot.");
			goto ret;
		}

		if (!strcmp(platform.active_base->type, "PL_DFX")) {
			if (platform.active_base->active > 0) {
				DFX_ERR("Unload previously loaded DFX designs, no empty slot.");
				goto ret;
			} else {
				/* Remove existing DFX base design before loading new user managed design */
				remove_base(platform.active_base->fpga_cfg_id);
				free(platform.active_base->slots);
				platform.active_base->slots = NULL;
				platform.active_base = NULL;
			}
		}
	}

	if ((platform.active_base == NULL) && (flag & USER_LOAD_PARTIAL)) {
		DFX_ERR(
			"Load Full bitstream before loading bitstream to any partial reconfiguration region.");
		goto ret;
	}

	if (binfile == NULL) {
		DFX_ERR("Provide path for bitstream");
		goto ret;
	}

	/* Validate overlay requirements if flag indicates overlay loading */
	if ((flag >> 1) & 1) {
		if (region == NULL) {
			DFX_ERR("Provide overlay region name");
			goto ret;
		}
		if (overlay == NULL) {
			DFX_ERR("Error: Provide path for device tree overlay file\n");
			goto ret;
		}
	}

	bin = path_basename(binfile);

	rv = user_load_bitstream(binfile, overlay, region, flag & USER_LOAD_PARTIAL);

	if (!rv) {
		int i;

		/* get the free space in array to add new entry */
		for (i = 0; (i < MAX_WATCH) && (base_designs[i].base_path[0] != '\0'); i++)
			;

		if (i == MAX_WATCH) {
			DFX_ERR("Unable to add new design, MAX limit (%d) reached.", MAX_WATCH);
			user_unload_overlay(region);
			rv = -1;
		} else {
			DFX_DBG("Adding user managed design %s", bin);
			strncpy(base_designs[i].name, bin, sizeof(base_designs[i].name) - 1);
			base_designs[i].name[sizeof(base_designs[i].name) - 1] = '\0';
			strncpy(base_designs[i].base_path, "User", sizeof(base_designs[i].base_path) - 1);
			base_designs[i].base_path[sizeof(base_designs[i].base_path) - 1] = '\0';
			base_designs[i].active = 0;
			base_designs[i].is_user_load = 1;
			base_designs[i].user_load_type = flag & USER_LOAD_PARTIAL;
			rv = base_designs[i].user_load_handle = get_free_slot_handle();
			strncpy(base_designs[i].user_load_region, region ? region : "",
					sizeof(base_designs[i].user_load_region) - 1);
			base_designs[i].user_load_region[sizeof(base_designs[i].user_load_region) - 1] = '\0';

			if (base_designs[i].user_load_type && platform.active_base)
				platform.active_base->active += 1;
			else
				platform.active_base = &base_designs[i];

			/* User load: new user-loaded design is added to base_designs.
			 * Reassign list IDs and mark the package listing dirty so clients know
			 * to re-list before using load/unload-by-ID.
			 */
			mark_pkg_listing_dirty();
		}
	}

ret:
	return rv;
}

/**
 * user_unload_overlay() - unload the device tree overlay associated with the specified region.
 *
 * @region: Name of the overlay region to unload.
 *
 * This function unloads the specified overlay from the configfs interface.
 * It also cleans up the corresponding entry in the base_designs array.
 *
 * This function is used by the daemon to unload device tree overlays for user-managed designs.
 *
 * Return: 0 on success,
 *        -1 on failure or if the overlay does not exist.
 */
int user_unload_overlay(const char *region)
{
	char ov_dir[512];
	struct stat sb;
	int i;

	if (region == NULL) {
		DFX_ERR("Provide overlay region");
		return -1;
	}

	snprintf(ov_dir, sizeof(ov_dir), "%s/%s", DTBO_ROOT_DIR, region);
	if (((stat(ov_dir, &sb) == 0) && S_ISDIR(sb.st_mode))) {
		remove_overlay_dir(ov_dir);

		for (i = 0; (i < MAX_WATCH) &&
					strncmp(base_designs[i].user_load_region, region, strlen(region) + 1);
			 i++)
			;
		if (i == MAX_WATCH) {
			DFX_ERR("No entry found for user_load_region: %s.", region);
		} else {
			base_designs[i].user_load_region[0] = '\0';
		}

		return 0;
	} else {
		DFX_ERR("Overlay directory %s doesn't exist for region %s", ov_dir, region);
		return -1;
	}
}

/**
 * user_unload() - unload the user managed design associated with the specified handle.
 *
 * @handle: An integer handle that uniquely identifies the user managed design to be unloaded.
 *
 * This function unloads the user managed design from the base_designs array and
 * unloads the associated device tree overlay if it exists.
 * It also decrements the active count of the base design if it is a partial load.
 *
 * This function is used by the daemon to unload user-managed designs when requested by the
 * client application.
 *
 * Return: 0 on success,
 *        -1 on failure or if the handle does not correspond to any loaded design.
 */
int user_unload(const int handle)
{
	int i;

	for (i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].is_user_load && base_designs[i].user_load_handle == handle)
			break;
	}
	if (i == MAX_WATCH) {
		DFX_ERR("No entry found for handle: %d", handle);
		return -1;
	}

	if (!base_designs[i].user_load_type && (platform.active_base->active > 0)) {
		DFX_ERR("Unload all partial bitstreams before unloading full bitstream.");
		return -1;
	}

	if (base_designs[i].user_load_region[0])
		user_unload_overlay(base_designs[i].user_load_region);

	if (base_designs[i].user_load_type && platform.active_base)
		platform.active_base->active -= 1;
	else
		platform.active_base = NULL;

	base_designs[i].name[0] = '\0';
	base_designs[i].base_path[0] = '\0';
	base_designs[i].is_user_load = 0;
	base_designs[i].user_load_handle = -1;
	base_designs[i].list_id = 0;
	platform.available_slot_handle[handle] = 0;

	/* User unload: user-loaded entry is removed from base_designs.
	 * Reassign list IDs and mark the package listing dirty so clients know
	 * to re-list before using load/unload-by-ID.
	 */
	mark_pkg_listing_dirty();

	return 0;
}

/**
 * init_user_load() - initializes the environment for user managed design.
 *
 * This static function sets up the necessary directories and mounts
 * the configfs interface for managing user load operations.
 */
static void init_user_load(void)
{
	DIR *FD;

	FD = opendir(DTBO_ROOT_DIR);
	if (FD)
		closedir(FD);
	else {
		DFX_ERR(
			"%s not present on the"
			" system. Is configfs enabled in kernel config?",
			DTBO_ROOT_DIR);
	}
}

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
void *threadFunc([[maybe_unused]] void *_)
{
	int wd, j, ret;
	char buf[BUF_LEN] __attribute__((aligned(8)));
	ssize_t numRead;
	char *p;
	char new_dir[512], fname[600];
	struct inotify_event *event;
	struct basePLDesign *base;

	active_watch = (struct watch *)calloc(MAX_WATCH, sizeof(struct watch));
	base_designs = (struct basePLDesign *)calloc(MAX_WATCH, sizeof(struct basePLDesign));

	for (j = 0; j < MAX_WATCH; j++) {
		active_watch[j].wd = -1;
	}

	inotifyFd = inotify_init(); /* Create inotify instance */
	if (inotifyFd == -1)
		DFX_ERR("inotify_init");

	firmware_dir_walk();
	assign_list_ids();
	/* Done parsing on target accelerators, now load a default one if present in config file */
	sem_post(&mutex);

	init_user_load();

	/* Listen for new updates in firmware path*/
	for (;;) {
		numRead = read(inotifyFd, buf, BUF_LEN);
		if (numRead <= 0 && errno != EAGAIN)
			DFX_ERR("read() from inotify failed");

		/* Process all of the events in buffer returned by read() */

		for (p = buf; p < buf + numRead;) {
			event = (struct inotify_event *)p;
			if (event->mask & IN_CREATE || event->mask & IN_CLOSE_WRITE) {
				if (event->mask & IN_ISDIR) {
					struct watch *w = get_watch(event->wd);
					if (w == NULL || strstr(event->name, ".dpkg-new"))
						break;
					sprintf(new_dir, "%s/%s", w->path, event->name);
					if (path_to_watch(new_dir) != NULL) {
						break;
					}
					DFX_DBG("add inotify watch on %s w->name %s parent %s", new_dir, w->name,
							w->parent_path);
					wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
					if (wd == -1)
						DFX_PR("inotify_add_watch failed on %s", new_dir);
					add_to_watch(wd, event->name, new_dir, w->name, w->path);
					/* new addition of base design will be managed by firmware_dir_walk() */
				} else if (!strcmp(event->name, "shell.json")) {
					struct watch *w = get_watch(event->wd);
					if (w == NULL)
						break;
					mark_pkg_listing_dirty();
					base = findBaseDesign_path(w->path);
					if (base == NULL)
						break;
					sprintf(fname, "%s/%s", w->path, "shell.json");
					ret = initBaseDesign(base, fname);
					if (ret == 0) {
						parse_packages(base, w->name, w->path);
					}
				} else if (!strcmp(event->name, "accel.json")) {
					struct watch *w, *parent_watch;
					accel_info_t *accel;

					w = get_watch(event->wd);
					if (w == NULL) {
						break;
					}

					parent_watch = path_to_watch(w->parent_path);
					if (parent_watch == NULL) {
						break;
					}

					mark_pkg_listing_dirty();
					base = findBaseDesign_path(parent_watch->parent_path);
					if (base == NULL) {
						break;
					}

					DFX_DBG("Add accel %s to base %s", w->path, base->base_path);
					accel =
						add_accel_to_base(base, w->parent_name, w->parent_path, base->base_path);
					initAccel(accel, w->path);
				}
			} else if ((event->mask & IN_DELETE) && (event->mask & IN_ISDIR)) {
				struct watch *w = get_watch(event->wd);
				if (w == NULL)
					break;
				sprintf(new_dir, "%s/%s", w->path, event->name);
				DFX_DBG("Removing watch on %s parent_path %s", new_dir, w->parent_path);
				if (!strcmp(w->parent_name, ""))
					remove_base_design(new_dir, w->path, 1);
				else
					remove_base_design(new_dir, w->path, 0);
				remove_watch(new_dir);
				mark_pkg_listing_dirty();
			} else if ((event->mask & IN_MOVED_FROM) && (event->mask & IN_ISDIR)) {
				struct watch *w = get_watch(event->wd);
				if (w == NULL)
					break;
				sprintf(new_dir, "%s/%s", w->path, event->name);
				if (!strcmp(w->parent_name, ""))
					remove_base_design(new_dir, w->path, 1);
				else
					remove_base_design(new_dir, w->path, 0);
				remove_watch(new_dir);
				mark_pkg_listing_dirty();
			} else if (event->mask & IN_MOVED_TO) {
				/*
				 * 'dnf install' creates tmp filenames and then does 'mv' to desired filenames.
				 * Hence IN_CREATE notif will be on tmp filenames, add logic in MOVED_TO
				 * notification for shell.json
				 */
				if (event->mask & IN_ISDIR || !strcmp(event->name, "shell.json")) {
					struct watch *w = get_watch(event->wd);
					if (w == NULL || strstr(event->name, ".dpkg-new"))
						break;
					if (event->mask & IN_ISDIR) {
						sprintf(new_dir, "%s/%s", w->path, event->name);
						/* new addition of base design will be managed by firmware_dir_walk() */
					} else {
						sprintf(new_dir, "%s", w->path);
					}
					mark_pkg_listing_dirty();
					base = findBaseDesign_path(new_dir);
					if (base == NULL)
						break;
					sprintf(fname, "%s/%s", new_dir, "shell.json");
					ret = initBaseDesign(base, fname);
					if (ret == 0) {
						parse_packages(base, w->name, new_dir);
					}
				}
			}
			p += sizeof(struct inotify_event) + event->len;
		}
	}
}

int dfx_init()
{
	pthread_t t;

	strcpy(config.defaul_accel_name, "");
	strcpy(platform.boardName, DEFAULT_BOARD_NAME);
	sem_init(&mutex, 0, 0);

	parse_config(CONFIG_PATH, &config);

	/* Read board name from EEPROM and store directly in platform.boardName */
	if (read_board_name_from_eeprom(platform.boardName, sizeof(platform.boardName)) == 0) {
		DFX_PR("===================================");
		DFX_PR("Board Name: %s", platform.boardName);
		DFX_PR("===================================");
	} else {
		DFX_PR("Using default board name: %s", platform.boardName);
	}
	/* Detect if platform requires user load path */
	if (is_user_load_platform()) {
		platform.use_user_load_path = 1;
		DFX_PR("Platform requires user load path");
	}

	pthread_create(&t, NULL, threadFunc, NULL);
	sem_wait(&mutex);
	// TODO Save active design on filesytem and on reboot read that

	return 0;
}
