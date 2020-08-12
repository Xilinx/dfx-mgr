#ifndef LIBFPGA_H
#define LIBFPGA_H

int fpga_cfg_init(const char *fpga_package_path,
		  const char *devpath, int flags);
int fpga_cfg_load(int package_id);
int fpga_cfg_remove(int package_id);
int fpga_cfg_destroy(int package_id);

#endif
