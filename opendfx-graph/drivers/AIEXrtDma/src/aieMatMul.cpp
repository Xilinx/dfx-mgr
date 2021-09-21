#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "device.h"
#include "utils.h"
#include "aieMatMul.h"

#include <dfx-mgr/daemon_helper.h>

#ifdef __cplusplus

#include <iostream>
#include <adf.h>
#include <fstream>
#include <unistd.h>

#include "kernels/config.h"
#include "xgemm.h"

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
	#include "adf/adf_api/XRTConfig.h"
#endif

using namespace adf;


GMIO gmin[NUM_HW_ROWS] = {
	GMIO("gmioin0", 64, 1),
	GMIO("gmioin1", 64, 1),
	GMIO("gmioin2", 64, 1),
	GMIO("gmioin3", 64, 1),
	GMIO("gmioin4", 64, 1),
	GMIO("gmioin5", 64, 1),
	GMIO("gmioin6", 64, 1),
	GMIO("gmioin7", 64, 1),
};

GMIO gmout[NUM_HW_ROWS] = {
	GMIO("gmioout0", 64, 1),
	GMIO("gmioout1", 64, 1),
	GMIO("gmioout2", 64, 1),
	GMIO("gmioout3", 64, 1),
	GMIO("gmioout4", 64, 1),
	GMIO("gmioout5", 64, 1),
	GMIO("gmioout6", 64, 1),
	GMIO("gmioout7", 64, 1),
};

XGeMM my_graph;


AIEMatmul::AIEMatmul(const std::string &xclbinFilename) : xclbinFilename(xclbinFilename) {
}

extern "C" {
#endif

int aieMatMul_open(DeviceConfig_t *config){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) malloc(sizeof(aieMatMulConfig_t));
    sihaCfg->status = 0;
    
    adf::registerXRT(*(config->device_hdl), *(config->xrt_uid));
	std::cout << "[INFO] XCLBIN download complete" << std::endl;
	
	std::cout << "initialising graph ..." << std::endl;
	// Configure AIE 
	my_graph.init(); 

    config->privConfig = sihaCfg;
    
	printf("aieMatMul open \n");
	return 0;
}

int aieMatMul_close(DeviceConfig_t *config){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) config->privConfig;
    _unused(sihaCfg);
    
	my_graph.end();
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
	xrtDeviceClose(*(config->device_hdl));
#endif
	printf("aieMatMul close \n");
	return 0;
}

int matMulCompute(int *arrayA, int *arrayB, int *arrayC){
	int *arrayBT = (int*) GMIO::malloc(NUM_ELMNTS * sizeof(int));
	int *arrayAieA = (int*) GMIO::malloc(NUM_HW_ROWS * MAT_A_CHUNK * sizeof(int));
	int *arrayAieC = (int*) GMIO::malloc(NUM_HW_ROWS * MAT_A_CHUNK * sizeof(int));
	
	int *matA[NUM_ROWS];
	int *matB[NUM_ROWS];
	int *matC[NUM_ROWS];
	int *matBT[NUM_ROWS];
	int *matAieA[NUM_HW_ROWS];
	int *matAieC[NUM_HW_ROWS];
		
	for (int i = 0; i < NUM_ROWS; i++) {
		matA[i]  = (int *)(arrayA)  + i * NUM_COLS;
		matB[i]  = (int *)(arrayB)  + i * NUM_COLS;
		matC[i]  = (int *)(arrayC)  + i * NUM_COLS;
		matBT[i] = (int *)(arrayBT) + i * NUM_COLS;
	}
	
	for (int i = 0; i < NUM_HW_ROWS; i++) {
		matAieA[i] = (int *)(arrayAieA) + i * MAT_A_CHUNK;
		matAieC[i] = (int *)(arrayAieC) + i * MAT_A_CHUNK;
	}

	for (int i = 0; i < NUM_ROWS; i++) {
		for (int j = 0; j < NUM_COLS; j++) {
			matBT[j][i] = matB[i][j];
		}
	}

	for (int i = 0; i < NUM_HW_ROWS; i++) {
		for (int j = 0; j < MAT_A_CHUNK; j++) {
			matAieA[i][j] = matA[i * NUM_ROWS_PER_HW_ROW + j / NUM_COLS][j % NUM_COLS];
		}
	}

	for (int i = 0; i < NUM_HW_ROWS; i++) {
		gmin[i].gm2aie_nb(matAieA[i], MAT_A_CHUNK_SIZE);
		gmin[i].gm2aie_nb(arrayBT, NUM_ELMNTS * sizeof(int));
		// Enable AIE cores 
		if (i == 0)
			my_graph.run(1);
		gmout[i].aie2gm_nb(matAieC[i], MAT_A_CHUNK_SIZE);
	}

	for (int i = 0; i < NUM_HW_ROWS; i++) {
		gmout[i].wait();
	}
	
	// Z-ordering 
	for (int i = 0; i < NUM_COLS / (WIN_SIZE / NUM_ROWS_PER_TILE); i++) {
		for (int j = 0; j < NUM_HW_ROWS; j++) {
			for (int k = 0; k < NUM_HW_COLS; k++) {
				for (int m = 0; m < WIN_SIZE / NUM_ROWS_PER_TILE; m++) {
					for (int l = 0; l < NUM_ROWS_PER_TILE; l++) {
						matC[j * NUM_ROWS_PER_HW_ROW + k * NUM_ROWS_PER_TILE + l][i * (WIN_SIZE / NUM_ROWS_PER_TILE) + m]
							= arrayAieC[j * MAT_A_CHUNK + i * NUM_ROWS_PER_HW_ROW * (WIN_SIZE / NUM_ROWS_PER_TILE) + k * WIN_SIZE + m * NUM_ROWS_PER_TILE + l];
					}
				}
			}
		}
	}

	GMIO::free(arrayBT);
	GMIO::free(arrayAieA);
	GMIO::free(arrayAieC);
	// For sanity check compute the same on APU 
	return 1;
}


int aieMatMul_MM2SStatus(DeviceConfig_t *config){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) config->privConfig;
	_unused(sihaCfg);
	return 0;
}

int aieMatMul_S2MMStatus(DeviceConfig_t *config){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) config->privConfig;
	_unused(sihaCfg);
	return 0;
}

int aieMatMul_MM2SData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) config->privConfig;
	acapd_debug("aieMatMul MM2S\n");
	if (tid == 0){
		sihaCfg->arrayA = (int*)(((uint8_t*)buffer->ptr) + offset);
		sihaCfg->status += 1;
	}
	else if(tid == 1){
		sihaCfg->arrayB = (int*)(((uint8_t*)buffer->ptr) + offset);
		sihaCfg->status += 1;
	}
	std::cout << sihaCfg->status << std::endl;
	_unused(sihaCfg);
	_unused(firstLast);
	return 0;
}

int aieMatMul_S2MMData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) config->privConfig;
	acapd_debug("aieMatMul S2MM\n");
	sihaCfg->arrayC = (int*)(((uint8_t*)buffer->ptr) + offset);
	sihaCfg->status += 1;
	std::cout << sihaCfg->status << std::endl;
	
	_unused(sihaCfg);
	_unused(firstLast);
	return 0;
}

int aieMatMul_S2MMDone(DeviceConfig_t *config){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) config->privConfig;
	int status, res = 1;
	_unused(sihaCfg);
	_unused(status);
	_unused(res);
	if (sihaCfg->status == 3){
		matMulCompute(sihaCfg->arrayA, sihaCfg->arrayB, sihaCfg->arrayC);
		sihaCfg->status = 0;
		res = 1;
	}
	else{
		res = 0;
	}
	return res;
}

int aieMatMul_MM2SDone(DeviceConfig_t *config){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) config->privConfig;
	int status, res = 1;
	_unused(sihaCfg);
	_unused(status);
	_unused(res);
	return res;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "aieMatMul");
	tDevice->open       = aieMatMul_open;
	tDevice->close      = aieMatMul_close;
	tDevice->MM2SStatus = aieMatMul_MM2SStatus;
	tDevice->S2MMStatus = aieMatMul_S2MMStatus;
	tDevice->MM2SData   = aieMatMul_MM2SData;
	tDevice->S2MMData   = aieMatMul_S2MMData;
	tDevice->MM2SDone   = aieMatMul_MM2SDone;
	tDevice->S2MMDone   = aieMatMul_S2MMDone;
	*device = tDevice;
	*config = tConfig;
	printf("aieMatMul driver registered \n");
	return 0;
}

int unregisterDriver(Device_t** device, DeviceConfig_t** config){
	free(*device);
	free(*config);
	printf("aieMatMul driver unregistered \n");
	return 0;
}

#ifdef __cplusplus
}
#endif

