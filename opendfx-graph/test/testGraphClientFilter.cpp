// main.cpp
#include <iostream>
#include <dlfcn.h>
#include <iomanip>
#include <thread>
#include <signal.h>
#include <atomic>
#include "device.h"
#include "graph.hpp"
#include "graphManager.hpp"
#include "utils.hpp"
//#include <opencv2/core/types_c.h>
//#include <opencv2/core/core_c.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#define MAX_SLOTS 3 
#define IONODE_SIZE 32*1024*1024
#define TRANSACTION_SIZE  32*1024*1024
#define DDR_BASED opendfx::buffertype::DDR_BASED
#define STREAM_BASED opendfx::buffertype::STREAM_BASED

#define MAT_SIZE		1200
#define NUM_ROWS		MAT_SIZE
#define NUM_COLS		MAT_SIZE
#define BUFFER_SIZE		MAT_SIZE * MAT_SIZE * sizeof(int)
int main(int argc, char **argv) {
	int inDataSize=0;
	int outDataSize=0;
	cv::Mat srcImageR;
    srcImageR = cv::imread(argv[1], 0);

    int width = srcImageR.cols;
	if (argc >= 3) width = atoi(argv[2]);
	int height = srcImageR.rows;
	if (argc >= 4) height = atoi(argv[3]);

	if ((width != srcImageR.cols) || (height != srcImageR.rows))
		cv::resize(srcImageR, srcImageR, cv::Size(width, height));

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
	
	inDataSize = srcImageR.rows * srcImageR.cols * sizeof(int16_t);
	outDataSize = srcImageR.rows * srcImageR.cols * sizeof(int16_t);
 	auto metadata = srcImageR.size();
	int metadataSize = sizeof(srcImageR.size());

	auto graph = new opendfx::Graph{"G", 0};
	auto input00   = graph->addInputNode("INPUT", IONODE_SIZE);
	auto input01   = graph->addInputNode("INPUT", 1024);
	auto accel02   = graph->addAccel("XF2DFilter");
	auto output03  = graph->addOutputNode("OUTPUT", IONODE_SIZE);

	auto buffer00  = graph->addBuffer("BUFF", IONODE_SIZE, DDR_BASED);
	auto buffer01  = graph->addBuffer("BUFF", IONODE_SIZE, DDR_BASED);
	auto buffer02  = graph->addBuffer("BUFF", 1024, DDR_BASED);

	graph->connectOutputBuffer(input01,  buffer02, 0x00, metadataSize, 0, 0);
	graph->connectInputBuffer (accel02,  buffer02, 0x00, metadataSize, 1, 1);
	graph->connectOutputBuffer(input00,  buffer00, 0x00, inDataSize, 2, 0);
	graph->connectInputBuffer (accel02,  buffer00, 0x00, inDataSize, 3, 0);
	graph->connectOutputBuffer(accel02,  buffer01, 0x00, outDataSize, 3, 0);
	graph->connectInputBuffer (output03, buffer01, 0x00, outDataSize, 4, 0);
	std::cout << graph->toJson() << std::endl;;
	graph->submit();
	std::cout << "checking schedules \n";
	while(!graph->isScheduled());

	memcpy(input01->ptr, &metadata, metadataSize);
	memcpy(input00->ptr, srcData.data(), inDataSize);
	input00->post();	
	input01->post();	
	output03->wait();	
	memcpy(dstData.data(), output03->ptr, outDataSize);

	// Saturate the output values to [0,255]
	dst = cv::max(dst, 0);
	dst = cv::min(dst, 255);

	// Convert 16-bit output to 8-bit
	dst.convertTo(dstOutImage, srcImageR.type());

	// Analyze output {
	std::cout << "Analyzing diff\n";
	cv::imwrite("aie.png", dstOutImage);


	std::cout << "Computation Done ... \n";

	//utils::printBuffer((int*)arrayA, 0x100);
	//utils::printBuffer((int*)arrayB, 0x100);
	//utils::printBuffer((int*)arrayC, 0x100);

	return 0;
}
