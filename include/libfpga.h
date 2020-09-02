#ifndef LIBFPGA_H
#define LIBFPGA_H

#define XFPGA_FULLBIT_EN			(0x00000000U)
#define XFPGA_PARTIAL_EN			(0x00000001U)
#define XFPGA_AUTHENTICATION_DDR_EN		(0x00000002U)
#define XFPGA_AUTHENTICATION_OCM_EN		(0x00000004U)
#define XFPGA_ENCRYPTION_USERKEY_EN		(0x00000020U)
#define XFPGA_ENCRYPTION_DEVKEY_EN		(0x00000010U)

int fpga_cfg_init(const char *fpga_package_path,
                  const char *devpath, unsigned long flags);
int fpga_cfg_load(int package_id);
int fpga_cfg_drivers_load(int package_id);
int fpga_cfg_remove(int package_id);
int fpga_cfg_destroy(int package_id);

#endif
