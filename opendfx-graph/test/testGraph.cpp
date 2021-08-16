// main.cpp
#include <iostream>
#include <dlfcn.h>
#include <iomanip>
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
	opendfx::GraphManager gManager;
	opendfx::Graph *graph[10];

	Device_t* device;
	DeviceConfig_t *config;
	void *fallbackDriver = dlopen("./drivers/fallback/src/libfallback_shared.so", RTLD_NOW);
	REGISTER registerDev = (REGISTER) dlsym(fallbackDriver, "registerDriver");
	UNREGISTER unregisterDev = (UNREGISTER) dlsym(fallbackDriver, "unregisterDriver");
	registerDev(&device, &config);
	device->open(config);
	device->close(config);
	unregisterDev(&device, &config);
	dlclose(fallbackDriver);
	int N = 2;

	for(int i = 0; i < N; i++){
		graph[i] = new opendfx::Graph{"G", i};
		auto input00   = graph[i]->addInputNode("INPUT", IONODE_SIZE);
		auto accel01   = graph[i]->addAccel("AES");
		auto output02  = graph[i]->addOutputNode("OUTPUT", IONODE_SIZE);

		auto buffer00  = graph[i]->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);
		auto buffer01  = graph[i]->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);

		auto link00    = graph[i]->connectOutputBuffer(input00,  buffer00, 0x00, TRANSACTION_SIZE, 0, 0);
		auto link01    = graph[i]->connectInputBuffer (accel01,  buffer00, 0x00, TRANSACTION_SIZE, 1, 0);
		auto link02    = graph[i]->connectOutputBuffer(accel01,  buffer01, 0x00, TRANSACTION_SIZE, 1, 0);
		auto link03    = graph[i]->connectInputBuffer (output02, buffer01, 0x00, TRANSACTION_SIZE, 2, 0);
		_unused(link00);
		_unused(link01);
		_unused(link02);
		_unused(link03);
		std::cout << std::setw(4) << graph[i]->toJson() << std::endl;
		gManager.addGraph(graph[i]);
	}
	gManager.listGraphs();
	for(int i = 0; i < N; i++){
		std::cout << "###################\n";
		gManager.stageGraphs();
		gManager.listGraphs();
	}

	return 0;
}
