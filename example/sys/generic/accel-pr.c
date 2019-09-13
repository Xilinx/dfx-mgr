#include <acapd/accel.h>
#include <unistd.h>
#include <xil_printf.h>

int main(void)
{
#if 1
	acapd_accel_pkg_hd_t *pkg1;
	//acapd_accel_pkg_hd_t *pkg2;
	acapd_accel_t accel;
	//int ret;
#endif

	/* Configure packages */
	/* TODO: This step should be replaced by host tool. */
	xil_printf("allocate memory for package\r\n");
#if 1
	pkg1 = (acapd_accel_pkg_hd_t *)(0x10000000);
	//pkg1 = acapd_alloc_pkg(8 * 1024 * 1024);
	//pkg2 = acapd_alloc_pkg(8 * 1024 * 1024);
	//if (pkg1 == NULL) {
	//	xil_printf("Failed to allocate memory for packages.\n");
	//	return -1;
	//}
	//xil_printf("Configure package.\r\n");
	//ret = acapd_config_pkg(pkg1, ACAPD_ACCEL_PKG_TYPE_PDI, "pr1-rm1",
	//		       (8 * 1204 * 1024), (void *)0x10000000, 0);
	//if (ret < 0) {
	//	xil_printf("Failed to configure package.\r\n");
	//	return -1;
	//}

	/* Initialize accelerator with package */
	xil_printf("Initialize accelerator with packge.\r\n");
	init_accel(&accel, pkg1);

	/* Load accelerator */
	xil_printf("Load accelerator with packge.\r\n");
	load_accel(&accel, 0);

	sleep(2);
	remove_accel(&accel, 0);
#endif
	return 0;
}

