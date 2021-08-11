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

#define MAX_SLOTS 3 
int main(int argc, char **argv) {
	opendfx::GraphManager gManager;
	std::cout << "# main\n";
	opendfx::Graph *graph[10];
	int N = 10;

	for(int i = 0; i < N; i++){
		graph[i] = new opendfx::Graph{"G", i};
		auto input00   = graph[i]->addInputNode("INPUT");
		auto accel01   = graph[i]->addAccel("AES");
		auto output02  = graph[i]->addOutputNode("OUTPUT");

		auto buffer00  = graph[i]->addBuffer("BUFF");
		auto buffer01  = graph[i]->addBuffer("BUFF");

		auto link00    = graph[i]->connectOutputBuffer(input00,  buffer00);
		auto link01    = graph[i]->connectInputBuffer (accel01,  buffer00);
		auto link02    = graph[i]->connectOutputBuffer(accel01,  buffer01);
		auto link03    = graph[i]->connectInputBuffer (output02, buffer01);
		gManager.addGraph(graph[i]);
		//std::cout << graph[i]->countAccel() << std::endl;
	}
	int ret = gManager.startServices(MAX_SLOTS);	
	//gManager.listGraphs();
	//for(int i = 0; i < N; i++){
	//	std::cout << "##################\n" << std::endl;
	//	gManager.stageGraphs(MAX_SLOTS);
	//	gManager.listGraphs();
	//}

	gManager.stopServices();
	std::cout << "main\n";
	return 0;
}
