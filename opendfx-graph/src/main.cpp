// main.cpp
#include <iostream>
#include "graph.hpp"
//#include "accel.hpp"

int main(int argc, char **argv) {
	opendfx::Graph graph{"G"};
	//std::cout << graph.getInfo() << '\n';
	auto accel0 = graph.addAccel("AES");
	auto accel1 = graph.addAccel("AES");
	auto accel2 = graph.addAccel("AES");
	auto buffer0 = graph.addBuffer("BUFF");
	auto buffer1 = graph.addBuffer("BUFF");
	auto buffer2 = graph.addBuffer("BUFF");
	//accel0->addLinkRefCount();
	//std::cout << accel0->toJson(true);
	//std::cout << graph.toJson(true);
	auto link0 = graph.addInputBuffer(accel0, buffer0);
	auto link1 = graph.addOutputBuffer(accel1, buffer0);
	std::cout << graph.toJson(true);
	graph.delBuffer(buffer0);
	std::cout << graph.toJson(true);
	//std::cout << graph.toJson(true);
	//std::cout << graph.toJson();
	return 0;
}
