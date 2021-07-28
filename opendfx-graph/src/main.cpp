// main.cpp
#include <iostream>
#include <dlfcn.h>
#include "device.h"
#include "graph.hpp"
#include "graphManager.hpp"

int main(int argc, char **argv) {
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

	for(int i = 0; i < 10; i++){
		graph[i] = new opendfx::Graph{"G", i};
		auto input00   = graph[i]->addInputNode("INPUT");
		auto accel01   = graph[i]->addAccel("AES");
		auto output02  = graph[i]->addOutputNode("OUTPUT");
		auto buffer00  = graph[i]->addBuffer("BUFF");
		auto buffer01  = graph[i]->addBuffer("BUFF");
		auto link00    = graph[i]->addOutputBuffer(input00,  buffer00);
		auto link01    = graph[i]->addInputBuffer (accel01,  buffer00);
		auto link02    = graph[i]->addOutputBuffer(accel01,  buffer01);
		auto link03    = graph[i]->addInputBuffer (output02, buffer01);
		gManager.addGraph(graph[i]);
	}
	gManager.listGraphs();
	for(int i = 0; i < 10; i++){
		std::cout << "###################\n";
		gManager.stageGraphs();
		gManager.listGraphs();
	}

	return 0;
}
