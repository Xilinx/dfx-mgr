// main.cpp
#include <iostream>
#include "graph.hpp"
#include "graphManager.hpp"
//#include "accel.hpp"

int main(int argc, char **argv) {
	opendfx::GraphManager gManager;
	opendfx::Graph *graph[10];

	for(int i = 0; i < 10; i++){
		graph[i] = new opendfx::Graph{"G"};
		auto input00   = graph[i]->addAccel("INPUT");
		auto accel01   = graph[i]->addAccel("AES");
		auto output02  = graph[i]->addAccel("OUTPUT");
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
		gManager.stageGraphs();
		gManager.listGraphs();
	}
	//std::cout << "###################\n";
	//std::cout << graph.toJson();
	//gManager.listGraphs();
	//std::cout << "###################\n";
	//gManager.delGraph(graph1);
	//gManager.listGraphs();
	return 0;
}
