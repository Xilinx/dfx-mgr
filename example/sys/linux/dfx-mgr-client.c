/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022 - 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <dfx-mgr/dfxmgr_client.h>

static int resolve_cwd_path(char *out, size_t out_size)
{
	if (getcwd(out, out_size) == NULL) {
		perror("getcwd");
		return -1;
	}

	return 0;
}

/* ZynqMP secure bitstream flags. Values match the fpga-manager "flags"
 * bits libdfx programs; -f (Full/Partial) is handled separately. */
static const struct {
	const char *name;
	unsigned value;
} secure_flags[] = {
	/* clang-format off */
	{"AuthDDR", 0x40},
	{"AuthOCM", 0x80},
	{"EnUsrKey", 0x20},
	{"EnDevKey", 0x04},
	{"AuthEnUsrKeyDDR", 0x60},
	{"AuthEnUsrKeyOCM", 0xA0},
	{"AuthEnDevKeyDDR", 0x44},
	{"AuthEnDevKeyOCM", 0x84},
	/* clang-format on */
};

/* Map a -s secure flag name to its bit value. Returns 0 on success, -1 if
 * the name is not a recognized secure flag. */
static int lookup_secure_flag(const char *name, unsigned *value)
{
	for (size_t i = 0; i < sizeof(secure_flags) / sizeof(secure_flags[0]); i++) {
		if (!strcmp(name, secure_flags[i].name)) {
			*value = secure_flags[i].value;
			return 0;
		}
	}
	return -1;
}

/* Print the pkg-dirty warning when the reply sets DFX_RESP_PKG_DIRTY. */
static void print_pkg_dirty_warning(uint32_t flags)
{
	if (flags & DFX_RESP_PKG_DIRTY)
		printf("WARNING: Package IDs have changed since last -listPackage.\n");
}

static int send_and_recv_msg(socket_t *gs, struct message *send_msg, struct message *recv_msg)
{
	send_msg->size = strlen(send_msg->data);
	if (write(gs->sock_fd, send_msg, HEADERSIZE + send_msg->size) < 0) {
		perror("write");
		return -1;
	}
	if (read(gs->sock_fd, recv_msg, sizeof(struct message)) <= 0) {
		perror("No message or read error");
		return -1;
	}
	return 0;
}

static int validate_numeric_arg(const char *str, long min_val)
{
	char *endptr;
	long val = strtol(str, &endptr, 10);
	return (*endptr == '\0' && val >= min_val) ? 0 : -1;
}

static int format_load_request(int argc, char *argv[], char *data, size_t data_size)
{
	char *cma_path = NULL;

	if (argc >= 4 && !strcmp(argv[3], "-cma")) {
		if (argc < 5) {
			printf("Error: -cma option requires a path argument\n");
			return -1;
		}
		cma_path = argv[4];
	}
	if (cma_path)
		snprintf(data, data_size, "%s:%s", argv[2], cma_path);
	else
		snprintf(data, data_size, "%s", argv[2]);
	return 0;
}

static void print_fpga_state(const char *fpga_state)
{
	if (fpga_state != NULL)
		printf(" (FPGA state: %s)", fpga_state);
	printf("\n");
}

static int print_load_result(const char *label, const char *id_str, char *resp, uint32_t flags)
{
	long ret;

	print_pkg_dirty_warning(flags);

	/* Reply may carry the fpga_manager state as "<value>:<state>". */
	char *fpga_state = dfxmgr_split_fpga_state(resp);

	if (resp[0] != '-') {
		printf("%s%s%s: Loaded with slot_handle %s", label, label[0] ? " " : "", id_str, resp);
		print_fpga_state(fpga_state);
		return 0;
	}

	ret = strtol(&resp[1], NULL, 10);
	printf("Load Error: ");
	switch (ret) {
	case 2:
		printf("No package found for %s%s%s", label, label[0] ? " " : "", id_str);
		break;
	case 3:
		printf("No empty slot for %s%s%s", label, label[0] ? " " : "", id_str);
		break;
	case 4:
		printf("Secure load is supported on ZynqMP only");
		break;
	default:
		printf("Unable to load %s%s%s", label, label[0] ? " " : "", id_str);
		break;
	}
	print_fpga_state(fpga_state);
	return -(int)ret;
}

static int print_unload_result(const char *label, const char *id_str, const char *resp,
							   uint32_t flags)
{
	const char *sep = label[0] ? " " : "";

	print_pkg_dirty_warning(flags);

	if (resp[0] == '0') {
		printf("unload %s%s%s returns: %s (Ok)\n", label, sep, id_str, resp);
		return 0;
	}
	printf("unload %s%s%s returns: %s (Error)\n", label, sep, id_str, resp);
	return -1;
}

int main(int argc, char *argv[])
{
	socket_t gs;
	struct message send_message, recv_message;
	int ret, opt;
	int user_load_flag = 0;
	unsigned int sflag = 0; /* secure fpga-manager flag bits from -s */
	int user_unload_flag = 0;
	char *binfile = NULL, *overlay = NULL, *region = NULL;
	char *aeskey = NULL; /* raw AES user key value from -k */
	int readback_mode = 0;
	const char *rb_type = NULL; /* raw -t value, NULL until -t is given */
	const char *readback_file = "readback";

	memset(&send_message, '\0', sizeof(struct message));
	memset(&recv_message, '\0', sizeof(struct message));
	if (argc < 2) {
		printf("Expects an argument. Use -h to see options\n");
		return -1;
	}
	if (initSocket(&gs) < 0)
		return -1;

	if (!strcmp(argv[1], "-load")) {
		if (argc < 3) {
			printf("-load expects an ID. Try again.\n");
			return -1;
		}
		if (validate_numeric_arg(argv[2], 1) < 0) {
			printf("Error: -load expects a numeric ID. Use -loadByName for name-based loading.\n");
			return -1;
		}
		if (format_load_request(argc, argv, send_message.data, sizeof(send_message.data)) < 0)
			return -1;
		send_message.id = LOAD_ACCEL_BY_ID;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		ret = print_load_result("ID", argv[2], recv_message.data, recv_message.flags);
		if (ret)
			return ret;

	} else if (!strcmp(argv[1], "-remove")) {
		printf("WARNING: '-remove' is deprecated. Use '-unload' instead.\n");
		return -1;

	} else if (!strcmp(argv[1], "-unload")) {
		if (argc < 3) {
			printf("-unload expects an ID (from -listPackage, 0 = base design). Try again.\n");
			return -1;
		}
		if (validate_numeric_arg(argv[2], 0) < 0) {
			printf("Error: -unload expects a numeric ID (0 = base design). Use -unloadByName.\n");
			return -1;
		}
		snprintf(send_message.data, sizeof(send_message.data), "%s", argv[2]);
		send_message.id = UNLOAD_ACCEL_BY_ID;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		return print_unload_result("ID", argv[2], recv_message.data, recv_message.flags);

	} else if (!strcmp(argv[1], "-loadByName")) {
		if (argc < 3) {
			printf("-loadByName expects an accelerator name. Try again.\n");
			return -1;
		}
		if (format_load_request(argc, argv, send_message.data, sizeof(send_message.data)) < 0)
			return -1;
		send_message.id = LOAD_ACCEL_BY_NAME;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		ret = print_load_result("", argv[2], recv_message.data, recv_message.flags);
		if (ret)
			return ret;

	} else if (!strcmp(argv[1], "-unloadByName")) {
		if (argc < 3) {
			printf("-unloadByName expects an accelerator name. Try again.\n");
			return -1;
		}
		snprintf(send_message.data, sizeof(send_message.data), "%s", argv[2]);
		send_message.id = UNLOAD_ACCEL_BY_NAME;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		return print_unload_result("", argv[2], recv_message.data, recv_message.flags);

	} else if (!strcmp(argv[1], "-unloadByHandle")) {
		if (argc < 3) {
			printf("-unloadByHandle expects a slot handle number. Try again.\n");
			return -1;
		}
		if (validate_numeric_arg(argv[2], -1) < 0) {
			printf("Error: -unloadByHandle expects a numeric slot handle.\n");
			return -1;
		}
		snprintf(send_message.data, sizeof(send_message.data), "%s", argv[2]);
		send_message.id = UNLOAD_ACCEL_BY_HANDLE;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		return print_unload_result("handle", argv[2], recv_message.data, recv_message.flags);

	} else if (!strcmp(argv[1], "-listPackage")) {
		int list_flag = 0;

		/* Parse optional flags from remaining arguments */
		for (int i = 2; i < argc; i++) {
			if (!strcmp(argv[i], "-all")) {
				list_flag |= LIST_PKG_SHOW_ALL;
			} else if (!strcmp(argv[i], "-filter")) {
				list_flag |= LIST_PKG_FILTER;
			}
		}

		send_message.id = LIST_PACKAGE;
		send_message.flags = list_flag;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		printf("%s", recv_message.data);

	} else if (!strcmp(argv[1], "-listUIO")) {
		/*
		 * Need to convert to getopt_long. If argc=2, use slot 0.
		 * No UIO name means "list all", else get the first match
		 */
		char *uio = (argc < 4) ? "" : argv[3];

		send_message._u.slot = (argc == 3 || argc == 4) ? 0xff & strtol(argv[2], NULL, 10) : 0;
		sprintf(send_message.data, "%s", uio);
		send_message.id = LIST_ACCEL_UIO;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		printf("%s\n", recv_message.data);
	} else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		printf("Usage dfx-mgr-client COMMAND\n");
		printf("Commands\n");
		printf("-listPackage [-all] [-filter]\n");
		printf("\t\t\t List locally downloaded accelerator packages\n");
		printf("\t\t\t -all: shows all columns (default shows simplified view)\n");
		printf("\t\t\t -filter: filters by board name (shows only matching designs)\n");
		printf("\nID-based commands (use ID from -listPackage):\n");
		printf("-load <ID> [-cma <device>]\t Load accelerator by ID\n");
		printf("-unload <ID>\t\t\t Unload accelerator by ID (0 = base design)\n");
		printf("\nName-based commands:\n");
		printf("-loadByName <name> [-cma <device>]\t Load accelerator by name\n");
		printf("-unloadByName <name>\t\t\t Unload accelerator by name\n");
		printf("\nOther commands:\n");
		printf("-listUIO [<slot#> [UIOname]]\t list accelerator UIOs\n");
		printf("\nCMA Path Priority:\n");
		printf("\t1. Command-line -cma option (highest priority)\n");
		printf("\t2. Global 'cma_path' in daemon.conf\n");
		printf("\t3. Default system paths\n");
		printf("\nUsage for lightweight usecase\n");
		printf("-b <bitstream> -f <type>\t Load the bitstream alone\n");
		printf("-b <bitstream> -f <type> -o <dtbo> -n <region>\t Load the bitstream with dtbo\n");
		printf("-R -n <region>\t\t Remove overlay from livetree\n");
		printf("-unload <ID>\t\t Unload bitstream and unload associated overlay\n");
		printf("Options:\n\t -b <bitstream>\t Absolute path of bitstream file\n");
		printf("\t -o <dtbo>\t Absolute path of device tree overlay file\n");
		printf("\t -f <type>\t Bitstream type: <Full | Partial>\n");
		printf(
			"\t -n <region>\t Full or Partial reconfiguration region of FPGA in device tree (max 8 "
			"chars)\n");
		printf("\t -R\t\t Remove overlay from live tree without unloading bitstream\n");

		/* ZynqMP-only features: append new ZynqMP-specific commands below,
		 * keeping the "-<opt>\t <description>" layout used above. */
		printf("\nZynqMP-only features:\n");
		printf("  Secure bitstream load:\n");
		printf("\t -b <bitstream> -f <type> -s <flag>\t Load a secure bitstream\n");
		printf("\t -s <flag>\t Secure flag: <AuthDDR | AuthOCM | EnUsrKey | EnDevKey |\n");
		printf("\t\t\t AuthEnUsrKeyDDR | AuthEnUsrKeyOCM | AuthEnDevKeyDDR | AuthEnDevKeyOCM>\n");
		printf("\t -k <key>\t AES user key value for an encrypted bitstream;\n");
		printf("\t\t\t requires -s EnUsrKey (or a combo containing it)\n");
		printf("  PL configuration readback:\n");
		printf("\t -r [name]\t Read back PL configuration; \".bin\" is appended\n");
		printf("\t\t\t to <name> (default readback.bin), saved relative to the\n");
		printf("\t\t\t current directory unless <name> is an absolute path\n");
		printf("\t -t <0|1>\t Readback type: 0 = config registers, 1 = config data frames\n");
	} else {
		int unknown_arg = 1;
		while ((opt = getopt(argc, argv, "b:o:f:n:s:k:rt:R?:")) != -1) {
			unknown_arg = 0;
			switch (opt) {
			case 'b':
				binfile = optarg;
				break;
			case 'o':
				overlay = optarg;
				break;
			case 'f':
				if (!strcmp(optarg, "Partial")) {
					user_load_flag |= USER_LOAD_PARTIAL;
				} else if (strcmp(optarg, "Full")) {
					printf("Unknown value for -f: expect 'Full' or 'Partial'\n");
					return -1;
				}
				break;
			case 'n':
				if (strlen(optarg) > MAX_REGION_NAME_LEN) {
					printf(
						"Error: Region name must be %d characters or less (provided: %s, length: "
						"%d)\n",
						MAX_REGION_NAME_LEN, optarg, (int)strlen(optarg));
					return -1;
				}
				region = optarg;
				break;
			case 's': {
				unsigned int val;

				if (lookup_secure_flag(optarg, &val)) {
					printf("Unknown value for -s: %s\n", optarg);
					return -1;
				}
				sflag |= val;
				break;
			}
			case 'k':
				aeskey = optarg;
				break;
			case 'r':
				readback_mode = 1;
				if (optind < argc && argv[optind][0] != '-')
					readback_file = argv[optind++];
				break;
			case 't':
				rb_type = optarg;
				break;
			case 'R':
				user_unload_flag = 1;
				break;
			default:
				unknown_arg = 1;
				break;
			}
		}

		if (unknown_arg) {
			printf("Option not recognized, Try again.\n");
			return -1;
		}

		if (rb_type != NULL && !readback_mode)
			printf("Warning: -t has no effect without -r; ignoring\n");

		/* PL configuration readback */
		if (readback_mode) {
			if (binfile != NULL || user_unload_flag) {
				printf("Wrong usage: -r cannot be combined with -b or -R\n");
				return -1;
			}
			char abspath[PATH_MAX];
			char cwd[PATH_MAX];
			int n;
			/* Resolve against the client's CWD so the file lands where
			 * the command was run (like fpgautil), not in the daemon's CWD.
			 * An absolute name is used as-is. */
			if (readback_file[0] == '/') {
				n = snprintf(abspath, sizeof(abspath), "%s", readback_file);
			} else {
				if (resolve_cwd_path(cwd, sizeof(cwd)) < 0)
					return -1;
				n = snprintf(abspath, sizeof(abspath), "%s/%s", cwd, readback_file);
			}
			if (n < 0 || n >= (int)sizeof(abspath)) {
				printf("Error: readback path too long\n");
				return -1;
			}
			int readback_type = 0; /* default: config registers */
			if (rb_type != NULL) {
				if (strcmp(rb_type, "0") && strcmp(rb_type, "1")) {
					printf("Wrong usage: -t must be 0 or 1\n");
					return -1;
				}
				readback_type = atoi(rb_type); /* now known to be 0 or 1 */
			}
			send_message.id = USER_READBACK;
			send_message.flags = (readback_type == 1) ? USER_READBACK_DATAFRAME : 0;
			snprintf(send_message.data, sizeof(send_message.data), "%s", abspath);
			if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
				return -1;
			if (recv_message.data[0] != '-') {
				printf("Readback contents are stored in the file %s\n", recv_message.data);
				return 0;
			}
			printf("Readback failed: %s\n", &recv_message.data[1]);
			return -1;
		}

		if (user_unload_flag) {
			if ((binfile != NULL) || (overlay != NULL)) {
				printf("Wrong usage: Cannot load and unload together\n");
				return -1;
			}

			send_message.id = USER_UNLOAD;
			sprintf(send_message.data, "%s", (region == NULL) ? "full" : region);
			if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
				return -1;
			if (recv_message.data[0] == '0') {
				printf("Removed device tree overlay: %s\n", send_message.data);
			} else {
				printf("Failed to remove overlay: %s\n", send_message.data);
				return -1;
			}

		} else {
			if (binfile == NULL) {
				printf("Not provided the bitstream path\n");
				return -1;
			}

			send_message.id = USER_LOAD;
			sprintf(send_message.data, "%s", binfile);

			if (overlay != NULL) {
				if ((user_load_flag & USER_LOAD_PARTIAL) && (region == NULL)) {
					printf("FPGA region for partial loading has not provided\n");
					return -1;
				}
				user_load_flag |= USER_LOAD_HAS_OVERLAY;
				sprintf(send_message.data, "%s : %s : %s", binfile, overlay,
						(region == NULL) ? "full" : region);
			}
			send_message.flags = user_load_flag;
			send_message._u.fpga_flags = sflag;

			/* Append the AES user key as the trailing " : " token, the same
			 * way binfile/overlay/region are passed. It is only applied with
			 * the EnUsrKey secure bit. */
			if (aeskey != NULL && aeskey[0] != '\0') {
				unsigned int enusrkey = 0;

				lookup_secure_flag("EnUsrKey", &enusrkey);
				if (sflag & enusrkey) {
					size_t len = strlen(send_message.data);

					snprintf(send_message.data + len, sizeof(send_message.data) - len, " : %s",
							 aeskey);
				} else {
					printf("Warning: -k has no effect without -s EnUsrKey; ignoring\n");
				}
			}
			if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
				return -1;
			return print_load_result("", binfile, recv_message.data, recv_message.flags);
		}
	}
	return 0;
}
