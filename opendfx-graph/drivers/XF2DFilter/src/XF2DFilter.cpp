#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


#include <adf/adf_api/XRTConfig.h>
#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <xaiengine.h>
#include <xrt/experimental/xrt_kernel.h>

#include "device.h"
#include "utils.h"
#include "XF2DFilter.h"

#include <dfx-mgr/daemon_helper.h>

#ifdef __cplusplus

#include <iostream>
#include <adf.h>
#include <fstream>
#include <unistd.h>

#include "graph.cpp"

int run_opencv_ref(cv::Mat& srcImageR, cv::Mat& dstRefImage, float coeff[9]) {
    cv::Mat tmpImage;
    cv::Mat kernel = cv::Mat(3, 3, CV_32F, coeff);
    cv::filter2D(srcImageR, dstRefImage, -1, kernel, cv::Point(-1, -1), 0, cv::BORDER_REPLICATE);
    return 0;
}

extern "C" {
#endif

int XF2DFilter_open(DeviceConfig_t *config){
    XF2DFilterConfig_t *sihaCfg = (XF2DFilterConfig_t*) malloc(sizeof(XF2DFilterConfig_t));
    sihaCfg->status = 0;
    xF::deviceInit(config->xclbin);

	std::cout << xF::gpDhdl << std::endl;
    //adf::registerXRT(*(config->device_hdl), *(config->xrt_uid));
	//std::cout << "[INFO] XCLBIN download complete" << std::endl;
    //xF::gpDhdl = *config->device_hdl;

	//std::cout << xF::gpDhdl << std::endl;
    //std::ifstream stream(config->xclbin);
    //stream.seekg(0, stream.end);
    //size_t size = stream.tellg();
    //stream.seekg(0, stream.beg);
	//xF::gHeader.resize(size);
    //stream.read(xF::gHeader.data(), size);

    //xF::gpTop = reinterpret_cast<const axlf*>(xF::gHeader.data());
	
	//std::cout << "initialising graph ..." << std::endl;
	// Configure AIE 
	//my_graph.init(); 

    filter_graph.init();
    filter_graph.update(filter_graph.kernelCoefficients, float2fixed_coeff<10, 16>(kData).data(), 16);
    config->privConfig = sihaCfg;
    
	printf("XF2DFilter open \n");
	return 0;
}

int XF2DFilter_close(DeviceConfig_t *config){
    XF2DFilterConfig_t *sihaCfg = (XF2DFilterConfig_t*) config->privConfig;
	xrtDeviceClose(xF::gpDhdl);
	xF::gpTop = NULL;
    _unused(sihaCfg);
    
	//my_graph.end();
	xrtDeviceClose(xF::gpDhdl);
	printf("XF2DFilter close \n");
	return 0;
}

int filterCompute(uint8_t *inputData, uint8_t *outputData, cv::Size *metadata, int inputSize, int outputSize, int metadataSize){
	std::cout << "filterCompute !!" << std::endl;

	
	//////////////////////////////////////////
	// Read image from file and resize
	//////////////////////////////////////////
	/*cv::Mat srcImageR;
	srcImageR = cv::imread("/media/test/2dFilter/4k.jpg", 0);

	int width = srcImageR.cols;
	int height = srcImageR.rows;
	
	if ((width != srcImageR.cols) || (height != srcImageR.rows))
		cv::resize(srcImageR, srcImageR, cv::Size(width, height));

	int iterations = 1;
	
	std::cout << "Image size" << std::endl;
	std::cout << srcImageR.rows << std::endl;
	std::cout << srcImageR.cols << std::endl;
	std::cout << srcImageR.elemSize() << std::endl;
	std::cout << "Image size (end)" << std::endl;
	int op_width = srcImageR.cols;
	int op_height = srcImageR.rows;


    // Load image

    std::vector<int16_t> srcData;
    srcData.assign(srcImageR.data, (srcImageR.data + srcImageR.total()));
    cv::Mat src(srcImageR.rows, srcImageR.cols, CV_16SC1, (void*)srcData.data());

    // Allocate output buffer
    std::vector<int16_t> dstData;
    dstData.assign(op_height * op_width, 0);

    cv::Mat dst(op_height, op_width, CV_16SC1, (void*)dstData.data());
    cv::Mat dstOutImage(op_height, op_width, srcImageR.type());

	std::cout << xF::gpDhdl << std::endl;*/
    xF::xfcvDataMovers<xF::TILER, int16_t, MAX_TILE_HEIGHT, MAX_TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> tiler(1, 1);
    xF::xfcvDataMovers<xF::STITCHER, int16_t, MAX_TILE_HEIGHT, MAX_TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> stitcher;

    std::cout << "Graph init. This does nothing because CDO in boot PDI "
                 "already configures AIE.\n" << *metadata << std::endl;

    START_TIMER
    tiler.compute_metadata(*metadata);
    STOP_TIMER("Meta data compute time")

    START_TIMER
    auto tiles_sz = tiler.host2aie_nb((short int *)inputData, *metadata, {"gmioIn[0]"});
    stitcher.aie2host_nb((short int *)outputData, *metadata, tiles_sz, {"gmioOut[0]"});

    std::cout << "Graph run(" << (tiles_sz[0] * tiles_sz[1]) << ")\n";

    filter_graph.run(tiles_sz[0] * tiles_sz[1]);

    filter_graph.wait();
    tiler.wait({"gmioIn[0]"});
    stitcher.wait({"gmioOut[0]"});

    STOP_TIMER("filter2D function")
    //tt += tdiff;
    //@}

    // Saturate the output values to [0,255]
    /*dst = cv::max(dst, 0);
    dst = cv::min(dst, 255);

    // Convert 16-bit output to 8-bit
    dst.convertTo(dstOutImage, srcImageR.type());

    // Analyze output {
    std::cout << "Analyzing diff\n";
    //cv::Mat diff;
    //cv::absdiff(dstRefImage, dstOutImage, diff);
    //cv::imwrite("ref.png", dstRefImage);
    cv::imwrite("aie.png", dstOutImage);
    //cv::imwrite("diff.png", diff);
	_unused(tiler);
	_unused(stitcher);
	_unused(iterations);
	*/

/*
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
*/
	return 1;
}


int XF2DFilter_MM2SStatus(DeviceConfig_t *config){
    XF2DFilterConfig_t *sihaCfg = (XF2DFilterConfig_t*) config->privConfig;
	_unused(sihaCfg);
	return 0;
}

int XF2DFilter_S2MMStatus(DeviceConfig_t *config){
    XF2DFilterConfig_t *sihaCfg = (XF2DFilterConfig_t*) config->privConfig;
	_unused(sihaCfg);
	return 0;
}

int XF2DFilter_MM2SData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid){
    XF2DFilterConfig_t *sihaCfg = (XF2DFilterConfig_t*) config->privConfig;
	acapd_debug("XF2DFilter MM2S\n");
	if (tid == 0){
		sihaCfg->inputBuffer = ((uint8_t*)buffer->ptr) + offset;
		sihaCfg->inputSize = size;
		sihaCfg->status += 1;
		std::cout << sihaCfg->status << std::endl;
	}
	else if(tid == 1){
		sihaCfg->metadata = (cv::Size*) buffer->ptr;		
		sihaCfg->metadataSize = sizeof(cv::Size);		
	}
	_unused(sihaCfg);
	_unused(firstLast);
	return 0;
}

int XF2DFilter_S2MMData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast){
    XF2DFilterConfig_t *sihaCfg = (XF2DFilterConfig_t*) config->privConfig;
	acapd_debug("XF2DFilter S2MM\n");
	sihaCfg->outputBuffer = ((uint8_t*)buffer->ptr) + offset;
	sihaCfg->outputSize = size;
	sihaCfg->status += 1;
	std::cout << sihaCfg->status << std::endl;
	
	_unused(sihaCfg);
	_unused(firstLast);
	return 0;
}

int XF2DFilter_S2MMDone(DeviceConfig_t *config){
    XF2DFilterConfig_t *sihaCfg = (XF2DFilterConfig_t*) config->privConfig;
	int status, res = 1;
	_unused(sihaCfg);
	_unused(status);
	_unused(res);
	if (sihaCfg->status == 2){
		filterCompute(sihaCfg->inputBuffer, sihaCfg->outputBuffer, sihaCfg->metadata, sihaCfg->inputSize, sihaCfg->outputSize, sihaCfg->metadataSize);
		
		sihaCfg->status = 0;
		res = 1;
	}
	else{
		res = 0;
	}
	return res;
}

int XF2DFilter_MM2SDone(DeviceConfig_t *config){
    XF2DFilterConfig_t *sihaCfg = (XF2DFilterConfig_t*) config->privConfig;
	int status, res = 1;
	_unused(sihaCfg);
	_unused(status);
	_unused(res);
	return res;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "XF2DFilter");
	tDevice->open       = XF2DFilter_open;
	tDevice->close      = XF2DFilter_close;
	tDevice->MM2SStatus = XF2DFilter_MM2SStatus;
	tDevice->S2MMStatus = XF2DFilter_S2MMStatus;
	tDevice->MM2SData   = XF2DFilter_MM2SData;
	tDevice->S2MMData   = XF2DFilter_S2MMData;
	tDevice->MM2SDone   = XF2DFilter_MM2SDone;
	tDevice->S2MMDone   = XF2DFilter_S2MMDone;
	*device = tDevice;
	*config = tConfig;
	printf("XF2DFilter driver registered \n");
	return 0;
}

int unregisterDriver(Device_t** device, DeviceConfig_t** config){
	free(*device);
	free(*config);
	printf("XF2DFilter driver unregistered \n");
	return 0;
}

#ifdef __cplusplus
}
#endif

