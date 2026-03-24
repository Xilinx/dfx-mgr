/*
 * Copyright (C) 2022 - 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dfx-mgr/accel.h>
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
        DFX_ERR("Failed to determine the fpga state -"
        " could not read state file");
        return -1;
    }

    DFX_DBG("FPGA state read as: `%s`", read_buf);

    if (strcmp(read_buf, state_operating) == 0 ||
        strcmp(read_buf, state_unknown) == 0) {
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
        DFX_ERR("Failed to check the overlay was applied -"
                " could not read path file");
        return -1;
    }

    DFX_DBG("Overlay path read as: `%s`", read_buf);

    if (strcmp(read_buf, requested_path) != 0) {
        DFX_ERR("Overlay path does not match written path:\n"
                "\tRequested: `%s`\n"
                "\tCurrent:   `%s`",
                requested_path, read_buf);
        return -1;
    }

    /* Check overlay status */
    if (dfx_get_overlay_status(overlay_dir, read_buf, sizeof(read_buf)) < 0) {
        DFX_ERR("Failed to check the overlay was applied -"
                " could not read status file");
        return -1;
    }

    DFX_DBG("Overlay status read as: `%s`", read_buf);

    if (strcmp(read_buf, state_applied) != 0) {
        DFX_ERR("Overlay status is `%s`, expected `%s`",
                read_buf, state_applied);
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
		DFX_ERR("Failed to load firmware - failed to request bitstream load");
		return -1;
	}
	if (fpga_state()) {
		DFX_ERR("Failed to load firmware - write succeeded, but fpga reports"
				" bad state (or state could not be determined)");
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
static int user_load_overlay(const char *ov, const char *region) {
	char ov_dir[512];
	struct stat sb;

	snprintf(ov_dir, sizeof(ov_dir), "%s/%s", DTBO_ROOT_DIR, region);
	if (((stat(ov_dir, &sb) == 0) && S_ISDIR(sb.st_mode))) {
		DFX_ERR("Overlay already exists in the live tree");
		return -1;
	}

	// here, mode (0755) is ignored by the kernel so can be anything.
	if (mkdir(ov_dir, 0755)) {
		DFX_ERR("Failed to create overlay dir");
		return -1;
	}

	if (dfx_set_overlay_path(ov_dir, ov)) {
		DFX_ERR("Failed to set overlay's source path");
		remove_overlay_dir(ov_dir);
		return -1;
	}

	if (check_overlay_was_applied(ov_dir, ov)) {
		DFX_ERR("Overlay failed to apply - state or path was wrong");
		remove_overlay_dir(ov_dir);
		return -1;
	}

	if (fpga_state()) {
		DFX_ERR("Bitstream loading failed during overlay application");
		remove_overlay_dir(ov_dir);
		return -1;
	}

	return 0;
}

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
 * @is_partial: 0 for full bitstream, 1 for partial bitstream
 *
 * This function handles the actual FPGA programming using sysfs/configfs interfaces.
 * It does NOT create or update base_designs entries - that's the caller's responsibility.
 * This allows the same loading logic to be used for both user-managed and daemon-managed designs.
 *
 * Return: 0 on success, -1 on failure
 */
int user_load_bitstream(const char *bitstream, const char *overlay,
			const char *region, int is_partial)
{
	const char *bin, *ov;
	int rv = -1;

	if (!bitstream) {
		DFX_ERR("Bitstream path is NULL");
		return -1;
	}

	bin = path_basename(bitstream);

	if (dfx_set_fpga_flags(is_partial)) {
		DFX_ERR("Failed to set flags");
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
	return rv;
}
