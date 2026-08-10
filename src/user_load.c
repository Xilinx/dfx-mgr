/*
 * Copyright (C) 2022 - 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/user_load.h>
#include <libdfx.h>

/**
 * fpga_state() - check the current state of the FPGA.
 *
 * This static function checks the operational state of the FPGA by reading
 * the state from the sysfs interface.
 *
 * Return: 0 if the FPGA is in a valid state,
 *        -1 on error or if the state is not valid.
 */
static int fpga_state(void)
{
	const char *state_operating = "operating";
	const char *state_unknown = "unknown";
	char read_buf[128];

	if (dfx_get_fpga_state(read_buf, sizeof(read_buf)) < 0) {
		DFX_ERR(
			"Failed to determine the fpga state -"
			" could not read state file");
		return -1;
	}

	DFX_DBG("FPGA state read as: `%s`", read_buf);

	if (strcmp(read_buf, state_operating) == 0 || strcmp(read_buf, state_unknown) == 0) {
		return 0;
	}

	DFX_ERR("FPGA is in a bad state. State: `%s`", read_buf);
	return -1;
}

/**
 * check_overlay_was_applied(...) - check that an overlay was applied
 *
 *
 * @overlay_dir:      Path to the overlay directory in configfs.
 * @requested_path:   The overlay path to write to the `path` attribute.
 *
 * This function checks that an overlay was properly applied by reading
 * the overlay status and the overlay path by checking files in the configfs.
 * It checks that "applied" is in the overlay's `status` attribute
 * and that the `path` attribute contains the requested_path, i.e. the overlay
 * file which was requested
 *
 * Return: 0 if the overlay status "applied" and path matches requested_path,
 *        -1 on error or if either assertion is false
 */
static int check_overlay_was_applied(const char *overlay_dir, const char *requested_path)
{
	char read_buf[128];
	const char *state_applied = "applied";

	/* Check overlay path */
	if (dfx_get_overlay_path(overlay_dir, read_buf, sizeof(read_buf)) < 0) {
		DFX_ERR(
			"Failed to check overlay was applied for %s"
			" - could not read path file",
			overlay_dir);
		return -1;
	}

	DFX_DBG("Overlay path read as: `%s`", read_buf);

	if (strcmp(read_buf, requested_path) != 0) {
		DFX_ERR(
			"Overlay path does not match written path:\n"
			"\tRequested: `%s`\n"
			"\tCurrent:   `%s`",
			requested_path, read_buf);
		return -1;
	}

	/* Check overlay status */
	if (dfx_get_overlay_status(overlay_dir, read_buf, sizeof(read_buf)) < 0) {
		DFX_ERR(
			"Failed to check overlay was applied for %s"
			" - could not read status file",
			overlay_dir);
		return -1;
	}

	DFX_DBG("Overlay status read as: `%s`", read_buf);

	if (strcmp(read_buf, state_applied) != 0) {
		DFX_ERR("Overlay status is `%s`, expected `%s`", read_buf, state_applied);
		return -1;
	}

	return 0;
}

/**
 * user_load_sysfs() - load FPGA firmware via sysfs.
 * @bin: name of the bitstream file to load.
 *
 * This static function loads the FPGA by writing the provided bitstream
 * file name to the sysfs interface at /sys/class/fpga_manager/fpga0/firmware.
 * It then checks the FPGA state to ensure it is in a valid state after loading.
 *
 * Return: 0 on success,
 *        -1 on failure.
 */
static int user_load_sysfs(const char *bin)
{
	if (dfx_set_fpga_firmware(bin)) {
		DFX_ERR(
			"Failed to load firmware '%s'"
			" - failed to request bitstream load",
			bin);
		return -1;
	}
	if (fpga_state()) {
		DFX_ERR(
			"Failed to load firmware '%s' - write succeeded,"
			" but fpga reports bad state"
			" (or state could not be determined)",
			bin);
		return -1;
	}
	return 0;
}

/**
 * remove_overlay_dir() - remove device tree overlay from configfs interface.
 *
 * @dir: the overlay directory to be removed
 *
 * This function attempts to remove the directory provided, with additional logging.
 *
 */
void remove_overlay_dir(const char *dir)
{
	if (rmdir(dir) != 0) {
		DFX_ERR("Failed to remove directory `%s`", dir);
	} else {
		DFX_DBG("Directory `%s` removed", dir);
	}
}

/**
 * user_load_overlay() - Load device tree overlay using configfs interface.
 *
 * @ov:    Name of the device tree overlay file to load.
 * @region: Name of the region where the overlay should be loaded.
 *
 * This function handles the process of loading device tree overlays to the
 * specified region using the configfs interface.
 *
 * Return: 0 on success,
 *        -1 on failure.
 */
static int user_load_overlay(const char *ov, const char *region)
{
	char ov_dir[512];
	struct stat sb;

	snprintf(ov_dir, sizeof(ov_dir), "%s/%s", DTBO_ROOT_DIR, region);
	if (((stat(ov_dir, &sb) == 0) && S_ISDIR(sb.st_mode))) {
		DFX_ERR("Overlay already exists in the live tree");
		return -1;
	}

	// here, mode (0755) is ignored by the kernel so can be anything.
	if (mkdir(ov_dir, 0755)) {
		DFX_ERR("Failed to create overlay dir %s", ov_dir);
		return -1;
	}

	if (dfx_set_overlay_path(ov_dir, ov)) {
		DFX_ERR("Failed to set overlay source path for %s in region %s", ov, region);
		remove_overlay_dir(ov_dir);
		return -1;
	}

	if (check_overlay_was_applied(ov_dir, ov)) {
		DFX_ERR(
			"Overlay %s failed to apply in region %s"
			" - state or path was wrong",
			ov, region);
		remove_overlay_dir(ov_dir);
		return -1;
	}

	if (fpga_state()) {
		DFX_ERR(
			"Bitstream loading failed during overlay %s"
			" application in region %s",
			ov, region);
		remove_overlay_dir(ov_dir);
		return -1;
	}

	return 0;
}

/**
 * find_file_with_extensions() - Generic file finder with extension matching
 * @path: directory path to search
 * @extensions: array of file extensions to match (e.g., ".bin", ".dtbo")
 * @num_ext: number of extensions in the array
 * @file_type: descriptive name for logging (e.g., "bitstream", "overlay")
 * @is_error: if true, log error when not found; if false, log debug
 *
 * Searches for files matching any of the provided extensions.
 * Returns the first match found.
 *
 * Return: allocated string with full file path on success,
 *         NULL if no matching file found
 */
static char *find_file_with_extensions(const char *path, const char **extensions, int num_ext,
									   const char *file_type, int is_error)
{
	DIR *dir;
	struct dirent *entry;
	char *result = NULL;

	dir = opendir(path);
	if (!dir) {
		DFX_ERR("Cannot open directory %s", path);
		return NULL;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type != DT_REG)
			continue;

		int len = strlen(entry->d_name);
		for (int i = 0; i < num_ext; i++) {
			int ext_len = strlen(extensions[i]);
			if (len > ext_len && strcmp(entry->d_name + len - ext_len, extensions[i]) == 0) {
				result = malloc(strlen(path) + strlen(entry->d_name) + 2);
				if (result) {
					snprintf(result, strlen(path) + strlen(entry->d_name) + 2, "%s/%s", path,
							 entry->d_name);
					DFX_DBG("Found %s: %s", file_type, result);
				}
				closedir(dir);
				return result;
			}
		}
	}

	closedir(dir);

	if (is_error)
		DFX_ERR("No %s file found in %s", file_type, path);
	else
		DFX_DBG("No %s file found in %s", file_type, path);

	return NULL;
}

/**
 * find_bitstream_file() - find first bitstream file in directory
 * @path: directory path to search
 *
 * Searches for bitstream files with extensions: .bin, .bit, .pdi
 *
 * Return: allocated string with full file path on success,
 *         NULL if no bitstream file found
 */
static char *find_bitstream_file(const char *path)
{
	static const char *extensions[] = {".bin", ".bit", ".pdi"};
	return find_file_with_extensions(path, extensions, 3, "bitstream", 1);
}

/**
 * find_overlay_file() - find first device tree overlay file in directory
 * @path: directory path to search
 *
 * Searches for overlay files with extension: .dtbo
 *
 * Return: heap-allocated full path on success, NULL if no overlay file found.
 *         Caller must release with free_overlay_file_path() only (pairs with
 *         this allocator).
 */
char *find_overlay_file(const char *path)
{
	static const char *extension = ".dtbo";
	return find_file_with_extensions(path, &extension, 1, "overlay", 0);
}

/**
 * free_overlay_file_path() - release path from find_overlay_file()
 * @path: pointer returned by find_overlay_file(), or NULL
 */
void free_overlay_file_path(char *path)
{
	free(path);
}

/**
 * path_basename() - Extract filename component from a path
 * @path: Full or relative file path
 *
 * Returns a pointer to the character following the last '/' in @path,
 * or @path itself if no '/' is present. The returned pointer refers
 * into the original string (no allocation).
 *
 * Return: pointer to the basename within @path
 */
const char *path_basename(const char *path)
{
	const char *base = strrchr(path, '/');
	return base ? base + 1 : path;
}

/**
 * user_load_bitstream() - Load bitstream via sysfs/configfs without base_designs tracking
 * @bitstream: Full path to bitstream file
 * @overlay: Full path to overlay file (NULL if none)
 * @region: Region name for overlay loading (can be NULL if no overlay)
 * @flags: fpga-manager flags (bit 0 = partial, plus any secure-load bits)
 * @aes_key: raw AES user key value, or NULL; written to the fpga-manager only
 *           when the EnUsrKey bit (DFX_ENCRYPTION_USERKEY_EN) is set in @flags
 *
 * This function handles the actual FPGA programming using sysfs/configfs interfaces.
 * It does NOT create or update base_designs entries - that's the caller's responsibility.
 * This allows the same loading logic to be used for both user-managed and daemon-managed designs.
 *
 * Return: 0 on success, -1 on failure
 */
int user_load_bitstream(const char *bitstream, const char *overlay, const char *region,
						unsigned int flags, const char *aes_key, char *fpga_state,
						size_t fpga_state_sz)
{
	const char *bin, *ov;
	int rv = -1;

	if (!bitstream) {
		DFX_ERR("Bitstream path is NULL");
		return -1;
	}

	bin = path_basename(bitstream);

	if (dfx_set_fpga_flags(flags)) {
		DFX_ERR("Failed to set fpga flags for %s (flags=0x%x)", bitstream, flags);
		return -1;
	}

	/* Program the AES user key before the bitstream; it is written only
	 * when encrypted user-key loading is requested. */
	if ((flags & DFX_ENCRYPTION_USERKEY_EN) && aes_key && aes_key[0]) {
		if (dfx_set_fpga_key(aes_key)) {
			DFX_ERR("Failed to set AES user key for %s", bitstream);
			return -1;
		}
	}

	if (overlay && region) {
		ov = path_basename(overlay);
		dfx_set_firmware_search_path(overlay);
		rv = user_load_overlay(ov, region);
	} else {
		dfx_set_firmware_search_path(bitstream);
		rv = user_load_sysfs(bin);
	}

	dfx_set_firmware_search_path("");

	/* Capture fpga state at the programming site (regardless of rv). */
	if (fpga_state && fpga_state_sz) {
		if (dfx_get_fpga_state(fpga_state, fpga_state_sz) < 0)
			fpga_state[0] = '\0';
	}

	return rv;
}

/**
 * user_load_from_dir() - Find and load bitstream/overlay from directory
 * @search_path: Directory to search for bitstream and overlay files
 * @region: Region name for overlay loading
 * @is_partial: 0 for full bitstream, 1 for partial bitstream
 *
 * This wrapper function discovers bitstream and overlay files in the specified
 * directory and loads them using the user load mechanism.
 *
 * Return: 0 on success, -1 on failure
 */
int user_load_from_dir(const char *search_path, const char *region, int is_partial,
					   char *fpga_state, size_t fpga_state_sz)
{
	char *bitstream, *overlay;
	int ret;

	bitstream = find_bitstream_file(search_path);
	if (!bitstream) {
		DFX_ERR("Bitstream file not found in %s", search_path);
		return -1;
	}

	overlay = find_overlay_file(search_path);
	ret = user_load_bitstream(bitstream, overlay, region, is_partial, NULL, fpga_state,
							  fpga_state_sz);

	free(bitstream);
	free_overlay_file_path(overlay);

	return ret;
}

/**
 * user_load_init_accel() - Initialize accel structure for user load path
 * @pl_accel: Accelerator structure to initialize
 * @pkg: Package structure with name and path already set by caller
 * @slot_num: Slot number for rm_slot assignment
 * @accel_type: Accelerator type string (e.g. "XRT_FLAT", "PL_DFX")
 *
 * The user load path bypasses libdfx (init_accel + load_accel), so the accel
 * structure is never fully populated. This function fills in the
 * fields required by unload_accelerator (pkg, type) and listPackage (pkg->name)
 * to operate correctly. Fields like ip_dev, num_ip_devs, and fpga_cfg_id are
 * left zeroed since the user load removal path does not use them.
 */
void user_load_init_accel(acapd_accel_t *pl_accel, acapd_accel_pkg_hd_t *pkg, int slot_num,
						  const char *accel_type)
{
	strncpy(pl_accel->type, accel_type, sizeof(pl_accel->type) - 1);
	pl_accel->type[sizeof(pl_accel->type) - 1] = '\0';
	pl_accel->pkg = pkg;
	pl_accel->rm_slot = slot_num;
	strncpy(pl_accel->sys_info.tmp_dir, pkg->path, sizeof(pl_accel->sys_info.tmp_dir) - 1);
	pl_accel->sys_info.tmp_dir[sizeof(pl_accel->sys_info.tmp_dir) - 1] = '\0';
}

/**
 * fpga_readback() - capture PL configuration readback into a file.
 * @type:     readback type: 0 = configuration registers, 1 = configuration
 *            data frames.
 * @out_path: destination file for the captured readback data.
 *
 * Low-level wrapper over libdfx's dfx_fpga_readback(). Platform support
 * (e.g. ZynqMP-only) is enforced by libdfx itself, which rejects
 * unsupported requests, so no additional gating is done here.
 *
 * Return: 0 on success, negative libdfx DFX_* error code on failure.
 */
int fpga_readback(unsigned type, const char *out_path)
{
	int ret = dfx_fpga_readback(type, out_path);

	if (ret)
		DFX_ERR("PL readback (type=%u, out=%s): %s", type, out_path, readback_error_str(ret));

	return ret;
}

const char *readback_error_str(int code)
{
	switch (-code) {
	case DFX_INVALID_PARAM:
		return "invalid readback arguments (check name and -t 0 or -t 1)";
	case DFX_INVALID_PLATFORM_ERROR:
		return "PL readback is supported on ZynqMP only";
	case DFX_READBACK_ERROR:
		return "failed to write readback data to file";
	default:
		return "PL readback failed";
	}
}

/**
 * is_user_load_platform() - Detect if platform requires user load path
 *
 * @fpga_mgr: FPGA manager family detected once at startup (see
 *            detect_fpga_mgr_type() / platform.fpga_mgr).
 *
 * All load and unload commands must behave identically from the user's
 * perspective regardless of the underlying platform. On platforms that
 * support DMA-buf, dfx-mgr loads bitstreams through libdfx. On platforms
 * that do not, dfx-mgr transparently routes through the sysfs/configfs
 * "user" path instead, so the user-facing interface stays the same.
 *
 * Currently, the Zynq-7000 series is the only platform requiring the user
 * load path. This is a pure predicate over the already-detected family; it
 * does not read sysfs.
 *
 * Return: 1 if user load path is required, 0 otherwise
 */
int is_user_load_platform(enum fpga_mgr_type fpga_mgr)
{
	return fpga_mgr == FPGA_MGR_ZYNQ;
}
