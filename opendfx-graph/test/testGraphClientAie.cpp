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
	_unused(argc);
	_unused(argv);
	std::cout << "# main\n" << BUFFER_SIZE;
	int *arrayCS = (int*) malloc(BUFFER_SIZE);

	auto graph = new opendfx::Graph{"G", 0};
	auto input00   = graph->addInputNode("INPUT", IONODE_SIZE);
	auto input01   = graph->addInputNode("INPUT", IONODE_SIZE);
	auto accel02   = graph->addAccel("AIE_MAT_MUL");
	auto output03  = graph->addOutputNode("OUTPUT", IONODE_SIZE);

	auto buffer00  = graph->addBuffer("BUFF", IONODE_SIZE, DDR_BASED);
	auto buffer01  = graph->addBuffer("BUFF", IONODE_SIZE, DDR_BASED);
	auto buffer02  = graph->addBuffer("BUFF", IONODE_SIZE, DDR_BASED);

	graph->connectOutputBuffer(input00,  buffer00, 0x00, BUFFER_SIZE, 0, 0);
	graph->connectOutputBuffer(input01,  buffer01, 0x00, BUFFER_SIZE, 1, 0);
	graph->connectInputBuffer (accel02,  buffer00, 0x00, BUFFER_SIZE, 2, 0);
	graph->connectInputBuffer (accel02,  buffer01, 0x00, BUFFER_SIZE, 3, 1);
	graph->connectOutputBuffer(accel02,  buffer02, 0x00, BUFFER_SIZE, 3, 0);
	graph->connectInputBuffer (output03, buffer02, 0x00, BUFFER_SIZE, 4, 0);
	std::cout << graph->toJson() << std::endl;;
	graph->submit();
	std::cout << "checking schedules \n";
	while(!graph->isScheduled());

	int *arrayA = (int*)input00->ptr;
	int *arrayB = (int*)input01->ptr;
	int *arrayC = (int*)output03->ptr;

	int *matA[NUM_ROWS];
	int *matB[NUM_ROWS];
	int *matC[NUM_ROWS];
	int *matCS[NUM_ROWS];
	
	for (int i = 0; i < NUM_ROWS; i++) {
		matA[i]  = (int *)(arrayA)  + i * NUM_COLS;
		matB[i]  = (int *)(arrayB)  + i * NUM_COLS;
		matC[i]  = (int *)(arrayC)  + i * NUM_COLS;
		matCS[i]  = (int *)(arrayCS)  + i * NUM_COLS;
	}

	for (int i = 0; i < NUM_ROWS; i++) {
		for (int j = 0; j < NUM_COLS; j++) {
			matA[i][j] = (i * NUM_COLS + j) % 10;
			matB[i][j] = (i * NUM_COLS + j) % 10;
			matCS[i][j] = 0;
		}
	}
	std::cout << "Input Matrix Generated ... \n";

	input00->post();	
	input01->post();	
	output03->wait();	

	std::cout << "Computation Done ... \n";

	utils::printBuffer((int*)arrayA, 0x100);
	utils::printBuffer((int*)arrayB, 0x100);
	utils::printBuffer((int*)arrayC, 0x100);


	/* For sanity check compute the same on APU */
	std::cout << "[INFO] Running sanity check" << std::endl;
	for(int i = 0; i < NUM_ROWS; i++) {
		for (int j = 0; j < NUM_COLS; j++) {
			for (int k = 0; k < NUM_COLS; k++) {
				matCS[i][j] += matA[i][k] * matB[k][j];
			}
		}
	}

	int pass = 1;
	for (int i = 0; i < NUM_ROWS; i++) {
		for (int j = 0; j < NUM_COLS; j++) {
			if (matC[i][j] == matCS[i][j])
				continue;

			std::cout << "[ERROR] Data mismatch: "
				  << "AIE[" << i << "][" << j << "] = "
				  << matC[i][j] << " APU[" << i
				  << "][" << j << "] =" << matCS[i][j]
				  << std::endl;
			pass = -1;
		}
	}
	if(pass == 1){
		std::cout << "Check Passed" << std::endl;
	}
	return 0;
}
