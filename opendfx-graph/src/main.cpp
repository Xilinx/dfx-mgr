// main.cpp
#include <iostream>
#include "graph.hpp"
#include "graphManager.hpp"
//#include "accel.hpp"

int main(int argc, char **argv) {
	opendfx::GraphManager gManager;
	opendfx::Graph graph0{"G0"};
	auto input00   = graph0.addAccel("INPUT");
	auto accel01   = graph0.addAccel("AES");
	auto output02  = graph0.addAccel("OUTPUT");
	auto buffer00  = graph0.addBuffer("BUFF");
	auto buffer01  = graph0.addBuffer("BUFF");
	auto link00    = graph0.addOutputBuffer(input00,  buffer00);
	auto link01    = graph0.addInputBuffer (accel01,  buffer00);
	auto link02    = graph0.addOutputBuffer(accel01,  buffer01);
	auto link03    = graph0.addInputBuffer (output02, buffer01);
	std::cout << graph0.toJson();

	opendfx::Graph graph1{"G1"};
	auto input10   = graph1.addAccel("INPUT");
	auto accel11   = graph1.addAccel("AES");
	auto output12  = graph1.addAccel("OUTPUT");
	auto buffer10  = graph1.addBuffer("BUFF");
	auto buffer11  = graph1.addBuffer("BUFF");
	auto link10    = graph1.addOutputBuffer(input10,  buffer10);
	auto link11    = graph1.addInputBuffer (accel11,  buffer10);
	auto link12    = graph1.addOutputBuffer(accel11,  buffer11);
	auto link13    = graph1.addInputBuffer (output12, buffer11);
	std::cout << graph1.toJson();

	gManager.addGraph(graph0);
	gManager.addGraph(graph1);
	opendfx::Graph graph = gManager.mergeGraphs();
	std::cout << "###################\n";
	std::cout << graph.toJson();
	gManager.listGraphs();
	//std::cout << "###################\n";
	//gManager.delGraph(graph1);
	//gManager.listGraphs();
	return 0;
}
