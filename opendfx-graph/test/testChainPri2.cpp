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

	auto graph = new opendfx::Graph{"G", 2};
	auto input00   = graph->addInputNode("INPUT", IONODE_SIZE);
	auto accel01   = graph->addAccel("FIR");
	auto accel02   = graph->addAccel("AES192");
	auto accel03   = graph->addAccel("AES128");
	auto output02  = graph->addOutputNode("OUTPUT", IONODE_SIZE);

	auto buffer00  = graph->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);
	auto buffer01  = graph->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);
	auto buffer02  = graph->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);
	auto buffer03  = graph->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);

	auto link00    = graph->connectOutputBuffer(input00,  buffer00, 0x00, TRANSACTION_SIZE, 0, 0);
	auto link01    = graph->connectInputBuffer (accel01,  buffer00, 0x00, TRANSACTION_SIZE, 1, 0);
	auto link02    = graph->connectOutputBuffer(accel01,  buffer01, 0x00, TRANSACTION_SIZE, 1, 0);
	auto link03    = graph->connectInputBuffer (accel02,  buffer01, 0x00, TRANSACTION_SIZE, 2, 0);
	auto link04    = graph->connectOutputBuffer(accel02,  buffer02, 0x00, TRANSACTION_SIZE, 2, 0);
	auto link05    = graph->connectInputBuffer (accel03,  buffer02, 0x00, TRANSACTION_SIZE, 3, 0);
	auto link06    = graph->connectOutputBuffer(accel03,  buffer03, 0x00, TRANSACTION_SIZE, 3, 0);
	auto link07   = graph->connectInputBuffer  (output02, buffer03, 0x00, TRANSACTION_SIZE, 4, 0);
	std::cout << graph->toJson() << std::endl;;
	graph->submit();
	_unused(link00);
	_unused(link01);
	_unused(link02);
	_unused(link03);
	_unused(link04);
	_unused(link05);
	_unused(link06);
	_unused(link07);

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
