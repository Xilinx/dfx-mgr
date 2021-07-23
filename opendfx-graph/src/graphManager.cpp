// graphManager.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include "graph.hpp"
#include "graphManager.hpp"

using opendfx::GraphManager;

GraphManager::GraphManager(){
	int i;
	//srand(time(0)); // Seed initialisation based on time
	char * block;
	short size = 1;
	std::ifstream urandom("/dev/urandom", std::ios::in|std::ios::binary); // Seed initialisation based on /dev/urandom
	urandom >> i;
	srand(i ^ time(0));
	urandom.close();

	id = rand() % 0x10000;
}

int GraphManager::addGraph(opendfx::Graph *graph){
	int priority = graph->getPriority();
	switch (priority){
		case 0: graphsQueue0.push_back(graph);
				//std::cout << "graphsQueue0\n";
				break;
		case 1: graphsQueue1.push_back(graph);
				//std::cout << "graphsQueue1\n";
				break;
		case 2: graphsQueue2.push_back(graph);
				//std::cout << "graphsQueue2\n";
				break;
		default:graphsQueue0.push_back(graph);
				//std::cout << "graphsQueue0\n";
				break;
	}
	return 0;
}

int GraphManager::delGraph(opendfx::Graph *graph){
	int priority = graph->getPriority();
	switch (priority){
		case 0: graphsQueue0.erase(std::remove(graphsQueue0.begin(), graphsQueue0.end(), graph), graphsQueue0.end());
				break;
		case 1: graphsQueue1.erase(std::remove(graphsQueue1.begin(), graphsQueue1.end(), graph), graphsQueue1.end());
				break;
		case 2: graphsQueue2.erase(std::remove(graphsQueue2.begin(), graphsQueue2.end(), graph), graphsQueue2.end());
				break;
		default: graphsQueue0.erase(std::remove(graphsQueue0.begin(), graphsQueue0.end(), graph), graphsQueue0.end());
				 break;
	}
	return 0;
}


int GraphManager::listGraphs()
{
	std::vector<opendfx::Graph *>*graphs;
	for (int i = 0; i < 3; i++){
		switch(i){
			case 0: graphs = &graphsQueue2;
					break;  
			case 1: graphs = &graphsQueue1;
					break;  
			case 2: graphs = &graphsQueue0;
					break;  
		}
		for (std::vector<opendfx::Graph *>::iterator it = graphs->begin() ; it != graphs->end(); ++it)
		{
			opendfx::Graph*graph = *it;
			std::cout << "# " << graph->getInfo() << "\n";
		}
	}
	return 0;
}

opendfx::Graph GraphManager::mergeGraphs(){
	opendfx::Graph graph0{"Merged"};
	for (std::vector<opendfx::Graph *>::iterator it = graphsQueue0.begin() ; it != graphsQueue0.end(); ++it)
	{
		opendfx::Graph* graph = *it;
		graph0 = graph0 + *graph;
	}
	std::cout << "\nNo of accels  = " << graph0.countAccel();
	std::cout << "\nNo of buffers = " << graph0.countBuffer(); 
	std::cout << "\nNo of links   = " << graph0.countLink(); 
	return graph0;
}

#define MAX_SLOT 3

int GraphManager::stageGraphs(){
	int remainingSlots = MAX_SLOT;
	int accelCounts = 0;
	std::vector<opendfx::Graph *>*graphs;
	//std::cout << remainingSlots << " : " << accelCounts << "\n";

	for (int i = 0; i < 3; i++){
		switch(i){
			case 0: graphs = &graphsQueue2;
					break;  
			case 1: graphs = &graphsQueue1;
					break;  
			case 2: graphs = &graphsQueue0;
					break;  
		}
		for (std::vector<opendfx::Graph *>::iterator it = graphs->begin() ; it != graphs->end(); ++it)
		{
			opendfx::Graph* graph = *it;
			accelCounts = graph->countAccel();
			//std::cout << "##" << graph->getPriority() << "\n";
			if (remainingSlots >= accelCounts && accelCounts > 0){
				remainingSlots = remainingSlots - accelCounts;
				stagedGraphs.push_back(graph);
				graphs->erase(std::remove(graphs->begin(), graphs->end(), graph), graphs->end());
				//std::cout << graph->getInfo() << "\n";
			}
			else{
				break;
			}
		}
	}
	return 0;
}

int GraphManager::scheduleGraph(){
	return 0;
}
