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

simulation::platform<NUM_HW_ROWS, NUM_HW_ROWS> platform(&gmin[0], &gmin[1],
							&gmin[2], &gmin[3],
							&gmin[4], &gmin[5],
							&gmin[6], &gmin[7],
							&gmout[0], &gmout[1],
							&gmout[2], &gmout[3],
							&gmout[4], &gmout[5],
							&gmout[6], &gmout[7]);

connect<> in0(platform.src[0], my_graph.matrix_ab[0]);
connect<> in1(platform.src[1], my_graph.matrix_ab[1]);
connect<> in2(platform.src[2], my_graph.matrix_ab[2]);
connect<> in3(platform.src[3], my_graph.matrix_ab[3]);
connect<> in4(platform.src[4], my_graph.matrix_ab[4]);
connect<> in5(platform.src[5], my_graph.matrix_ab[5]);
connect<> in6(platform.src[6], my_graph.matrix_ab[6]);
connect<> in7(platform.src[7], my_graph.matrix_ab[7]);

connect<> out0(my_graph.result[0], platform.sink[0]);
connect<> out1(my_graph.result[1], platform.sink[1]);
connect<> out2(my_graph.result[2], platform.sink[2]);
connect<> out3(my_graph.result[3], platform.sink[3]);
connect<> out4(my_graph.result[4], platform.sink[4]);
connect<> out5(my_graph.result[5], platform.sink[5]);
connect<> out6(my_graph.result[6], platform.sink[6]);
connect<> out7(my_graph.result[7], platform.sink[7]);

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
static std::vector<char>
load_xclbin(xrtDeviceHandle device, const std::string& fnm)
{
	if (fnm.empty())
		throw std::runtime_error("No XCLBIN specified");

	/* Load bit stream */
	std::ifstream stream(fnm);
	stream.seekg(0,stream.end);
	size_t size = stream.tellg();
	stream.seekg(0,stream.beg);

	std::vector<char> header(size);
	stream.read(header.data(),size);

	auto top = reinterpret_cast<const axlf*>(header.data());
	if (xrtDeviceLoadXclbin(device, top))
		throw std::runtime_error("Bitstream download failed");

	return header;
}
#endif


AIEMatmul::AIEMatmul(const std::string &xclbinFilename) : xclbinFilename(xclbinFilename) {
}

extern "C" {
#endif

int aieMatMul_open(DeviceConfig_t *config){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) malloc(sizeof(aieMatMulConfig_t));
    AIEMatmul *handle = new AIEMatmul("/usr/bin/aie-matrix-multiplication.xclbin");
    sihaCfg->matmulHandle = (AIEMATMUL_HANDLE) handle;
    sihaCfg->status = 0;
    
    handle->dhdl = xrtDeviceOpen(0);
	if (!handle->dhdl) {
		std::cout << "[ERROR] Device open error" << std::endl;
		return EXIT_FAILURE;
	}

	auto xclbin = load_xclbin(handle->dhdl, handle->xclbinFilename);
	if(!xclbin.size()) {
		std::cout << "[ERROR] XCLBIN load error" << std::endl;
		return EXIT_FAILURE;
	};
	
	auto top = reinterpret_cast<const axlf*>(xclbin.data());
	adf::registerXRT(handle->dhdl, top->m_header.uuid);
	std::cout << "[INFO] XCLBIN download complete" << std::endl;
	
	std::cout << "initialising graph ..." << std::endl;
	/* Configure AIE */
	my_graph.init(); 

    config->privConfig = sihaCfg;
    
	printf("aieMatMul open \n");
	return 0;
}

int aieMatMul_close(DeviceConfig_t *config){
    aieMatMulConfig_t *sihaCfg = (aieMatMulConfig_t*) config->privConfig;
    _unused(sihaCfg);
    AIEMatmul *handle = (AIEMatmul*)sihaCfg->matmulHandle;
    
	my_graph.end();
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
	xrtDeviceClose(handle->dhdl);
	_unused(handle);
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
		/* Enable AIE cores */
		if (i == 0)
			my_graph.run(1);
		gmout[i].aie2gm_nb(matAieC[i], MAT_A_CHUNK_SIZE);
	}

	for (int i = 0; i < NUM_HW_ROWS; i++) {
		gmout[i].wait();
	}
	
	/* Z-ordering */
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
	/* For sanity check compute the same on APU */
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

