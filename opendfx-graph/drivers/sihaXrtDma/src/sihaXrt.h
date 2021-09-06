#include "experimental/xrt_kernel.h"

typedef struct sihaXrtDmaConfig {
	xrtDeviceHandle device;
	xuid_t uuid;
	char s2mm_kname[100]; // = "dms2mm0:dms2mm0_1";
	char mm2s_kname[100]; // = "dmmm2s0:dmmm2s0_1";
	xrtKernelHandle s2mm_kernel;
	xrtKernelHandle mm2s_kernel;
	xrtRunHandle s2mm_Run;
	xrtRunHandle mm2s_Run;
	
} sihaXrtDmaConfig_t;

