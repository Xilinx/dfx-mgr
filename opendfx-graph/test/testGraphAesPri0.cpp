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
#include <unistd.h>

#define MAX_SLOTS 3 
#define IONODE_SIZE 32*1024*1024
#define BUFFER_SIZE 16*1024*1024
#define TRANSACTION_SIZE  32*1024*1024
#define DDR_BASED opendfx::buffertype::DDR_BASED
#define STREAM_BASED opendfx::buffertype::STREAM_BASED

int main(int argc, char **argv) {
	_unused(argc);
	_unused(argv);
	std::cout << "# main\n";

	auto graph = new opendfx::Graph{"G", 0};
	auto input00   = graph->addInputNode("INPUT", IONODE_SIZE);
	auto accel03   = graph->addAccel("AES128");
	auto output02  = graph->addOutputNode("OUTPUT", IONODE_SIZE);

	auto buffer00  = graph->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);
	auto buffer01  = graph->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);

	graph->connectOutputBuffer(input00,  buffer00, 0x00, TRANSACTION_SIZE, 0, 0);
	graph->connectInputBuffer (accel03,  buffer00, 0x00, TRANSACTION_SIZE, 1, 0);
	graph->connectOutputBuffer(accel03,  buffer01, 0x00, TRANSACTION_SIZE, 1, 0);
	graph->connectInputBuffer  (output02, buffer01, 0x00, TRANSACTION_SIZE, 2, 0);
	std::cout << graph->toJson() << std::endl;;
	graph->submit();

	std::cout << "checking schedules \n";
	while(!graph->isScheduled()){
		sleep(1);
		std::cout << "waiting ..." << std::endl;
	}

	uint32_t *A = (uint32_t*)input00->ptr;
	std::cout << A[0] << "Copying data ... \n";
	for(int i = 0; i < TRANSACTION_SIZE/8; i++){
		A[i] = i;
	}
	
	std::cout << "Semaphore post ... \n";
	//std::cout << input00->trywait() << std::endl;	
	input00->post();	
	//std::cout << input00->trywait() << std::endl;	
	output02->wait();	
	std::cout << "Client test done ... \n";
	utils::printBuffer((uint32_t*)input00->ptr, 0x100);
	utils::printBuffer((uint32_t*)output02->ptr, 0x100);
	return 0;
}
