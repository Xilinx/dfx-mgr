/***************************************************************
 * Copyright (c) 2020 Xilinx, Inc.  All rights reserved.
 * SPDX-License-Identifier: MIT
 ***************************************************************/

/*This example programs the FPGA to have The base FPGA Region(static)
 * with  One PR region (This PR region has two RM's).
 *
 * Configure Base Image
 * 	-->Also called the "static image"
 * 	   An FPGA image that is designed to do full reconfiguration of the FPGA
 * 	   A base image may set up a set of partial reconfiguration regions that
 *	   may later be reprogrammed. 
 *	-->DFX_EXTERNAL_CONFIG flag should be set if the FPGA has already been
 *	   configured prior to OS boot up.
 *
 * Configure the PR Images
 *	-->An FPGA set up with a base image that created a PR region.
 *	   The contents of each PR may have multiple Reconfigurable Modules
 *	   This RM's(PR0-RM0, PR0-RM1) are reprogrammed independently while
 *	   the rest of the system continues to function.
 */

#include <stdio.h>
#include "libdfx.h"

#define BASE "/lib/firmware/xilinx/firmware/"
#define PR0RP0  "/lib/firmware/xilinx/firmware/AES128/AES128_slot0/"
#define PR0RP1  "/lib/firmware/xilinx/firmware/AES192/AES192_slot0/"
#define PR0RP2  "/lib/firmware/xilinx/firmware/FFT4K/FFT4K_slot0/"

#define PR1RP0  "/lib/firmware/xilinx/firmware/AES128/AES128_slot1/"
#define PR1RP1  "/lib/firmware/xilinx/firmware/AES192/AES192_slot1/"
#define PR1RP2  "/lib/firmware/xilinx/firmware/FFT4K/FFT4K_slot1/"

#define PR2RP0  "/lib/firmware/xilinx/firmware/AES128/AES128_slot2/"
#define PR2RP1  "/lib/firmware/xilinx/firmware/AES192/AES192_slot2/"


int main()
{
	//int  package_id_full;
	int  ret;
	int  package_id_pr0_rm0, package_id_pr0_rm1, package_id_pr0_rm2;
	int  package_id_pr1_rm0, package_id_pr1_rm1, package_id_pr1_rm2;
	int  package_id_pr2_rm0, package_id_pr2_rm1;

	/* package FULL Initilization */
	/*	package_id_full = dfx_cfg_init(BASE, 0, DFX_EXTERNAL_CONFIG_EN);
		if (package_id_full < 0)
		return -1;
		printf("dfx_cfg_init: FULL Package completed successfully\r\n");
		*/
	/* package PR0-RM0 Initilization */
	package_id_pr0_rm0 = dfx_cfg_init(PR0RP0, 0, 0);
	if (package_id_pr0_rm0 < 0)
		return -1;
	printf("dfx_cfg_init: PR0-RM0 Package completed successfully\r\n");

	/* package PR0-RM1 Initilization */
	package_id_pr0_rm1 = dfx_cfg_init(PR0RP1, 0, 0);
	if (package_id_pr0_rm1 < 0)
		return -1;
	printf("dfx_cfg_init: PR0-RM1 Package completed successfully\r\n");

	/* package PR0-RM1 Initilization */
	package_id_pr0_rm2 = dfx_cfg_init(PR0RP2, 0, 0);
	if (package_id_pr0_rm2 < 0)
		return -1;
	printf("dfx_cfg_init: PR0-RM2 Package completed successfully\r\n");
	
	/*#########################################################*/
	/* package PR1-RM0 Initilization */
	package_id_pr1_rm0 = dfx_cfg_init(PR1RP0, 0, 0);
	if (package_id_pr1_rm0 < 0)
		return -1;
	printf("dfx_cfg_init: PR1-RM0 Package completed successfully\r\n");

	/* package PR1-RM1 Initilization */
	package_id_pr1_rm1 = dfx_cfg_init(PR1RP1, 0, 0);
	if (package_id_pr1_rm1 < 0)
		return -1;
	printf("dfx_cfg_init: PR1-RM1 Package completed successfully\r\n");

	/* package PR1-RM2 Initilization */
	package_id_pr1_rm2 = dfx_cfg_init(PR1RP2, 0, 0);
	if (package_id_pr1_rm2 < 0)
		return -1;
	printf("dfx_cfg_init: PR1-RM2 Package completed successfully\r\n");
	
	/*#########################################################*/
	
	/* package PR2-RM0 Initilization */
	package_id_pr2_rm0 = dfx_cfg_init(PR2RP0, 0, 0);
	if (package_id_pr2_rm0 < 0)
		return -1;
	printf("dfx_cfg_init: PR2-RM0 Package completed successfully\r\n");

	/* package PR2-RM1 Initilization */
	package_id_pr2_rm1 = dfx_cfg_init(PR2RP1, 0, 0);
	if (package_id_pr2_rm1 < 0)
		return -1;
	printf("dfx_cfg_init: PR2-RM1 Package completed successfully\r\n");

	/*ret = dfx_cfg_load(package_id_full);
	if (ret) {
		dfx_cfg_destroy(package_id_full);
		return -1;
	}
	printf("dfx_cfg_load: FULL Package completed successfully\r\n");
	*/
	
	/*#########################################################*/
	/* Package PR0-RM0 load */
	ret = dfx_cfg_load(package_id_pr0_rm0);
	if (ret) {
		dfx_cfg_destroy(package_id_pr0_rm0);
		return -1;
	}
	printf("dfx_cfg_load: PR0-RM0 Package completed successfully\r\n");

	ret = dfx_cfg_load(package_id_pr1_rm0);
	if (ret) {
		dfx_cfg_destroy(package_id_pr1_rm0);
		return -1;
	}
	printf("dfx_cfg_load: PR1-RM0 Package completed successfully\r\n");

	ret = dfx_cfg_load(package_id_pr2_rm0);
	if (ret) {
		dfx_cfg_destroy(package_id_pr2_rm0);
		return -1;
	}
	printf("dfx_cfg_load: PR2-RM0 Package completed successfully\r\n");

	/*#########################################################*/
	/* Remove PR0-RM0 package */
	ret = dfx_cfg_remove(package_id_pr0_rm0);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: PR0-RM0 Package completed successfully\r\n"); 

	/* Remove PR0-RM0 package */
	ret = dfx_cfg_remove(package_id_pr1_rm0);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: PR1-RM0 Package completed successfully\r\n"); 

	/* Remove PR0-RM0 package */
	ret = dfx_cfg_remove(package_id_pr2_rm0);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: PR2-RM0 Package completed successfully\r\n"); 
	
	/*#########################################################*/
	/* Package PR0-RM0 load */
	ret = dfx_cfg_load(package_id_pr0_rm1);
	if (ret) {
		dfx_cfg_destroy(package_id_pr0_rm1);
		return -1;
	}
	printf("dfx_cfg_load: PR0-RM1 Package completed successfully\r\n");

	ret = dfx_cfg_load(package_id_pr1_rm1);
	if (ret) {
		dfx_cfg_destroy(package_id_pr1_rm1);
		return -1;
	}
	printf("dfx_cfg_load: PR1-RM1 Package completed successfully\r\n");

	ret = dfx_cfg_load(package_id_pr2_rm1);
	if (ret) {
		dfx_cfg_destroy(package_id_pr2_rm1);
		return -1;
	}
	printf("dfx_cfg_load: PR2-RM1 Package completed successfully\r\n");

	/*#########################################################*/
	/* Remove PR0-RM0 package */
	ret = dfx_cfg_remove(package_id_pr0_rm1);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: PR0-RM1 Package completed successfully\r\n"); 

	/* Remove PR0-RM0 package */
	ret = dfx_cfg_remove(package_id_pr1_rm1);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: PR1-RM1 Package completed successfully\r\n"); 

	/* Remove PR0-RM0 package */
	ret = dfx_cfg_remove(package_id_pr2_rm1);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: PR2-RM1 Package completed successfully\r\n"); 
	
	/*#########################################################*/
	/* Package PR0-RM0 load */
	ret = dfx_cfg_load(package_id_pr0_rm2);
	if (ret) {
		dfx_cfg_destroy(package_id_pr0_rm2);
		return -1;
	}
	printf("dfx_cfg_load: PR0-RM2 Package completed successfully\r\n");

	ret = dfx_cfg_load(package_id_pr1_rm2);
	if (ret) {
		dfx_cfg_destroy(package_id_pr1_rm2);
		return -1;
	}
	printf("dfx_cfg_load: PR1-RM2 Package completed successfully\r\n");


	/*#########################################################*/
	/* Remove PR0-RM0 package */
	ret = dfx_cfg_remove(package_id_pr0_rm2);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: PR0-RM2 Package completed successfully\r\n"); 

	/* Remove PR0-RM0 package */
	ret = dfx_cfg_remove(package_id_pr1_rm2);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: PR1-RM2 Package completed successfully\r\n"); 

	
	/*#########################################################*/

	/* Remove Full package */
	/*ret = dfx_cfg_remove(package_id_full);
	if (ret)
		return -1;
	printf("dfx_cfg_remove: FULL Package completed successfully\r\n");
	*/
	/* Destroy PR0_RM0 package */
	ret = dfx_cfg_destroy(package_id_pr0_rm0);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: PR0-RM0 Package completed successfully\r\n");

	/* Destroy PR0_RM1 package */
	ret = dfx_cfg_destroy(package_id_pr0_rm1);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: PR0-RM1 Package completed successfully\r\n");

	/* Destroy PR0_RM2 package */
	ret = dfx_cfg_destroy(package_id_pr0_rm2);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: PR0-RM2 Package completed successfully\r\n");

	/*#########################################################*/
	ret = dfx_cfg_destroy(package_id_pr1_rm0);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: PR1-RM0 Package completed successfully\r\n");

	/* Destroy PR0_RM1 package */
	ret = dfx_cfg_destroy(package_id_pr1_rm1);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: PR1-RM1 Package completed successfully\r\n");

	/* Destroy PR0_RM2 package */
	ret = dfx_cfg_destroy(package_id_pr1_rm2);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: PR1-RM2 Package completed successfully\r\n");

	/*#########################################################*/
	ret = dfx_cfg_destroy(package_id_pr2_rm0);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: PR2-RM0 Package completed successfully\r\n");

	/* Destroy PR0_RM1 package */
	ret = dfx_cfg_destroy(package_id_pr2_rm1);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: PR2-RM1 Package completed successfully\r\n");

	/* Destroy Full package */
	/*ret = dfx_cfg_destroy(package_id_full);
	if (ret)
		return -1;
	printf("dfx_cfg_destroy: FULL Package completed successfully\r\n");
	*/
	return 0;
}	
