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
	std::cout << "# main\n";

	auto graph = new opendfx::Graph{"G", 0};
	auto input00   = graph->addInputNode("INPUT", IONODE_SIZE);
	auto output01  = graph->addOutputNode("OUTPUT", IONODE_SIZE);

	auto buffer00  = graph->addBuffer("BUFF", IONODE_SIZE, DDR_BASED);

	graph->connectOutputBuffer(input00,  buffer00, 0x00, BUFFER_SIZE, 0, 0);
	graph->connectInputBuffer (output01, buffer00, 0x00, BUFFER_SIZE, 0, 0);
	std::cout << graph->toJson() << std::endl;;
	graph->submit();
	std::cout << "checking schedules \n";
	while(!graph->isScheduled());

	input00->post();	
	output01->wait();	

	std::cout << "Computation Done ... \n";

	return 0;
}
