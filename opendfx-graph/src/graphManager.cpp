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

int GraphManager::addGraph(opendfx::Graph &graph){
	graphs.push_back(&graph);
	return 0;
}

int GraphManager::delGraph(opendfx::Graph &graph){
	graphs.erase(std::remove(graphs.begin(), graphs.end(), &graph), graphs.end());
	return 0;
}


int GraphManager::listGraphs()
{
	for (std::vector<opendfx::Graph *>::iterator it = graphs.begin() ; it != graphs.end(); ++it)
	{
		opendfx::Graph* graph = *it;
		std::cout << ' ' << graph->info() << '\n';
	}
	std::cout << '\n';
	return 0;
}

opendfx::Graph GraphManager::mergeGraphs(){
	opendfx::Graph graph0{"Merged"};
	for (std::vector<opendfx::Graph *>::iterator it = graphs.begin() ; it != graphs.end(); ++it)
	{
		opendfx::Graph* graph = *it;
		graph0 = graph0 + *graph;
	}
	std::cout << "\nNo of accels  = " << graph0.countAccel();
	std::cout << "\nNo of buffers = " << graph0.countBuffer(); 
	std::cout << "\nNo of links   = " << graph0.countLink(); 
	return graph0;
}
