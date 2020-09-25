#ifndef __LIBFPGA_H
#define __LIBFPGA_H

#define XFPGA_NORMAL_EN				(0x00000000U)
#define XFPGA_EXTERNAL_CONFIG_EN		(0x00000001U)
#define XFPGA_ENCRYPTION_USERKEY_EN		(0x00000020U)

/* Error codes */
#define XFPGA_INVALID_PLATFORM_ERROR		(0x1U)
#define XFPGA_CREATE_PACKAGE_ERROR		(0x2U)
#define XFPGA_DUPLICATE_FIRMWARE_ERROR		(0x3U)
#define XFPGA_DUPLICATE_DTBO_ERROR		(0x4U)
#define XFPGA_READ_PACKAGE_ERROR		(0x5U)
#define XFPGA_AESKEY_READ_ERROR			(0x6U)
#define XFPGA_DMABUF_ALLOC_ERROR		(0x7U)
#define XFPGA_INVALID_PACKAGE_ID_ERROR		(0x8U)
#define XFPGA_GET_PACKAGE_ERROR			(0x9U)
#define XFPGA_FAIL_TO_OPEN_DEV_NODE		(0xAU)
#define XFPGA_IMAGE_CONFIG_ERROR		(0xBU)
#define XFPGA_DRIVER_CONFIG_ERROR		(0xCU)
#define XFPGA_NO_VALID_DRIVER_DTO_FILE		(0xDU)
#define XFPGA_DESTROY_PACKAGE_ERROR		(0xEU)

int fpga_cfg_init(const char *fpga_package_path,
                  const char *devpath, unsigned long flags);
int fpga_cfg_load(int package_id);
int fpga_cfg_drivers_load(int package_id);
int fpga_cfg_remove(int package_id);
int fpga_cfg_destroy(int package_id);

#endif
