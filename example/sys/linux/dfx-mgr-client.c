/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022 - 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/daemon_helper.h>
#include <dfx-mgr/dfxmgr_client.h>

/**
 * strip_warning_prefix() - Strip and print any pkg-dirty warning prefix from daemon response
 * @data: Response data buffer (modified in-place if warning found)
 *
 * If the response starts with "WARNING:", prints the warning and returns
 * a pointer past the warning to the actual result value.
 *
 * Return: pointer to the actual result within data
 */
static char *strip_warning_prefix(char *data)
{
	if (strncmp(data, "WARNING:", 8) != 0)
		return data;

	char *result_start = strrchr(data, ' ');
	if (result_start) {
		*result_start = '\0';
		printf("%s\n", data);
		return result_start + 1;
	}
	return data;
}

static int send_and_recv_msg(socket_t *gs, struct message *send_msg,
		     struct message *recv_msg)
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

static int format_load_request(int argc, char *argv[], char *data,
				size_t data_size)
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

static int print_load_result(const char *label, const char *id_str,
			     const char *resp)
{
	long ret;

	if (resp[0] != '-') {
		printf("%s%s%s: Loaded with slot_handle %s\n",
		       label, label[0] ? " " : "", id_str, resp);
		return 0;
	}

	ret = strtol(&resp[1], NULL, 10);
	printf("Load Error: ");
	switch (ret) {
	case 2:
		printf("No package found for %s%s%s\n", label, label[0] ? " " : "", id_str);
		break;
	case 3:
		printf("No empty slot for %s%s%s\n", label, label[0] ? " " : "", id_str);
		break;
	default:
		printf("Unable to load %s%s%s\n", label, label[0] ? " " : "", id_str);
		break;
	}
	return -(int)ret;
}

static int print_unload_result(const char *label, const char *id_str,
			       const char *resp)
{
	const char *sep = label[0] ? " " : "";
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
	int user_unload_flag = 0;
	char *binfile = NULL, *overlay = NULL, *region = NULL;
	char *resp;

	memset (&send_message, '\0', sizeof(struct message));
	memset (&recv_message, '\0', sizeof(struct message));
	if (argc < 2) {
		printf("Expects an argument. Use -h to see options\n");
		return -1;
	}
	if (initSocket(&gs) < 0)
		return -1;

	if (!strcmp(argv[1],"-load")) {
		if (argc < 3) {
			printf("-load expects an ID. Try again.\n");
			return -1;
		}
		if (validate_numeric_arg(argv[2], 1) < 0) {
			printf("Error: -load expects a numeric ID. Use -loadByName for name-based loading.\n");
			return -1;
		}
		if (format_load_request(argc, argv, send_message.data,
					 sizeof(send_message.data)) < 0)
			return -1;
		send_message.id = LOAD_ACCEL_BY_ID;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		resp = strip_warning_prefix(recv_message.data);
		ret = print_load_result("ID", argv[2], resp);
		if (ret)
			return ret;

	} else if(!strcmp(argv[1],"-remove")) {
		printf("WARNING: '-remove' is deprecated. Use '-unload' instead.\n");
		return -1;

	} else if(!strcmp(argv[1],"-unload")) {
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
		resp = strip_warning_prefix(recv_message.data);
		return print_unload_result("ID", argv[2], resp);

	} else if(!strcmp(argv[1],"-loadByName")) {
		if (argc < 3) {
			printf("-loadByName expects an accelerator name. Try again.\n");
			return -1;
		}
		if (format_load_request(argc, argv, send_message.data,
					 sizeof(send_message.data)) < 0)
			return -1;
		send_message.id = LOAD_ACCEL_BY_NAME;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		ret = print_load_result("", argv[2], recv_message.data);
		if (ret)
			return ret;

	} else if(!strcmp(argv[1],"-unloadByName")) {
		if (argc < 3) {
			printf("-unloadByName expects an accelerator name. Try again.\n");
			return -1;
		}
		snprintf(send_message.data, sizeof(send_message.data), "%s", argv[2]);
		send_message.id = UNLOAD_ACCEL_BY_NAME;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		return print_unload_result("", argv[2], recv_message.data);

	} else if(!strcmp(argv[1],"-unloadByHandle")) {
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
		return print_unload_result("handle", argv[2], recv_message.data);

	} else if(!strcmp(argv[1],"-listPackage")) {
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
		printf("%s",recv_message.data);

	} else if(!strcmp(argv[1],"-listUIO")) {
		/*
		 * Need to convert to getopt_long. If argc=2, use slot 0.
		 * No UIO name means "list all", else get the first match
		 */
		char *uio = (argc < 4) ? "" : argv[3];

		send_message._u.slot = (argc == 3 || argc == 4)
			? 0xff & strtol(argv[2], NULL, 10)
			: 0;
		sprintf(send_message.data, "%s", uio);
		send_message.id = LIST_ACCEL_UIO;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		printf("%s\n", recv_message.data);
	} else if (!strcmp(argv[1], "-listIRbuf")) {
		send_message.id = SIHA_IR_LIST;
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		printf("%s\n", recv_message.data);
	} else if (!strcmp(argv[1], "-setIRbuf")) {
		if (argc < 3) {
			printf("-setIRbuf expects slot/accelerator list\n");
			return -1;
		}
		send_message.id = SIHA_IR_SET;
		snprintf(send_message.data, sizeof(send_message.data), "%s", argv[2]);
		if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
			return -1;
		printf("%s\n", recv_message.data);
	} else if(!strcmp(argv[1],"-allocBuffer")) {
	} else if(!strcmp(argv[1],"-freeBuffer")) {
	} else if(!strcmp(argv[1],"-getFDs")) {
	} else if(!strcmp(argv[1],"-getRMInfo")) {
	} else if(!strcmp(argv[1],"-getShellFD")) {
	} else if(!strcmp(argv[1],"-getClockFD")) {
	} else if(!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
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
		printf("-listIRbuf [slot]\t\t list inter-RM buffer info\n");
		printf("-setIRbuf a,b\t\t set RM stream from slot a to b\n");
		printf("-allocBuffer <size> \t\t Allocate buffer of size and return its DMA fd and pa\n");
		printf("-freeBuffer <pa> \t\t free buffer with physical address pa in decimal\n");
		printf("-getFDs <slot#> \t\t Send ip device FD's over socket\n");
		printf("-getRMInfo \n");
		printf("-getShellFD \n");
		printf("-getClockFD \n");
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
		printf("\t -n <region>\t Full or Partial reconfiguration region of FPGA in device tree (max 8 chars)\n");
		printf("\t -R\t\t Remove overlay from live tree without unloading bitstream\n");
	} else {
		int unknown_arg = 1;
		while ((opt = getopt(argc, argv, "b:o:f:n:R?:")) != -1) {
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
						printf("Error: Region name must be %d characters or less (provided: %s, length: %d)\n",
							MAX_REGION_NAME_LEN, optarg, (int)strlen(optarg));
						return -1;
					}
					region = optarg;
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

		if (user_unload_flag) {
			if ((binfile != NULL) || (overlay != NULL)) {
				printf("Wrong usage: Cannot load and unload together\n");
				return -1;
			}

			send_message.id = USER_UNLOAD;
			sprintf(send_message.data, "%s", (region == NULL) ? "full" : region);
			if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
				return -1;
			if (recv_message.data[0] == '0'){
				printf("Removed device tree overlay: %s\n", send_message.data);
			} else {
				printf("Failed to remove overlay: %s\n", send_message.data);
				return -1;
			}

		} else {
			if (binfile == NULL) {
				printf ("Not provided the bitstream path\n");
				return -1;
			}

			send_message.id = USER_LOAD;
			sprintf(send_message.data, "%s", binfile);

			if (overlay != NULL) {
				if ((user_load_flag & USER_LOAD_PARTIAL) && (region == NULL)) {
					printf ("FPGA region for partial loading has not provided\n");
					return -1;
				}
				user_load_flag |= USER_LOAD_HAS_OVERLAY;
				sprintf(send_message.data, "%s : %s : %s", binfile, overlay, (region == NULL) ? "full" : region);
			}
			send_message.flags = user_load_flag;
			if (send_and_recv_msg(&gs, &send_message, &recv_message) < 0)
				return -1;
			if (recv_message.data[0] == '-'){
				printf("Load Error: %s\n", recv_message.data);
				return -1;
			} else {
				printf("Loaded with slot_handle %s\n", recv_message.data);
			}
		}
	}	
	return 0;
}
