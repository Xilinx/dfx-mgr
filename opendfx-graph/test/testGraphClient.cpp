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
#define BUFFER_SIZE 16*1024*1024
#define TRANSACTION_SIZE  32*1024*1024
#define DDR_BASED 0

int main(int argc, char **argv) {
	_unused(argc);
	_unused(argv);
	std::cout << "# main\n";
	opendfx::Graph *graph[10];
	int N = 10;

	for(int i = 0; i < N; i++){
		graph[i] = new opendfx::Graph{"G", i};
		auto input00   = graph[i]->addInputNode("INPUT", IONODE_SIZE);
		auto accel01   = graph[i]->addAccel("AES128");
		auto output02  = graph[i]->addOutputNode("OUTPUT", IONODE_SIZE);

		auto buffer00  = graph[i]->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);
		auto buffer01  = graph[i]->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);

		auto link00    = graph[i]->connectOutputBuffer(input00,  buffer00, 0x00, TRANSACTION_SIZE, 0, 0);
		auto link01    = graph[i]->connectInputBuffer (accel01,  buffer00, 0x00, TRANSACTION_SIZE, 1, 0);
		auto link02    = graph[i]->connectOutputBuffer(accel01,  buffer01, 0x00, TRANSACTION_SIZE, 1, 0);
		auto link03    = graph[i]->connectInputBuffer (output02, buffer01, 0x00, TRANSACTION_SIZE, 2, 0);
		graph[i]->submit();
		_unused(link00);
		_unused(link01);
		_unused(link02);
		_unused(link03);
	}
	for(int i = 0; i < N; i++){
		std::cout << "checking schedules \n";
		graph[i]->isScheduled();
	
	}
	
	std::cout << "Client test done ... \n";
	return 0;
}
