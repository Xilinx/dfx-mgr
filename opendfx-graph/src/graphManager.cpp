// graphManager.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include <thread>
#include <atomic>
#include "utils.hpp"
#include "graph.hpp"
#include "graphManager.hpp"
#include <signal.h>

using opendfx::GraphManager;

GraphManager::GraphManager(){
	int i;
	//srand(time(0)); // Seed initialisation based on time
	char * block;
	short size = 1;
	std::ifstream urandom("/dev/urandom", std::ios::in|std::ios::binary); // Seed initialisation based on /dev/urandom
	urandom.read((char*)&i, sizeof(i));
	srand(i ^ time(0));
	urandom.close();
	//std::cout << i << " : " << time(0) << std::endl;

	id = rand() % 0x100000000;
}

int GraphManager::addGraph(opendfx::Graph *graph){
	int priority = graph->getPriority() % 3;
	graphQueue_mutex.lock();
	//std::cout << "addGraph " << priority << "\n";
	graphsQueue[priority].push_back(graph);
	//std::cout << "addGraph done ..\n";
	graphQueue_mutex.unlock();
	return 0;
}

int GraphManager::delGraph(opendfx::Graph *graph){
	int priority = graph->getPriority() % 3;
	graphQueue_mutex.lock();
	//std::cout << "delGraph " << priority << "\n";
	graphsQueue[priority].erase(std::remove(graphsQueue[priority].begin(), graphsQueue[priority].end(), graph), graphsQueue[priority].end());
	//std::cout << "delGraph done ..\n";
	graphQueue_mutex.unlock();
	return 0;
}


int GraphManager::listGraphs()
{
	std::vector<opendfx::Graph *>*graphs;
	for (int i = 0; i < 3; i++){
		graphQueue_mutex.lock();
		graphs = &graphsQueue[3-i-1];
		for (std::vector<opendfx::Graph *>::iterator it = graphs->begin() ; it != graphs->end(); ++it)
		{
			opendfx::Graph*graph = *it;
			std::cout << "# " << graph->getInfo() << "\n";
		}
		graphQueue_mutex.unlock();
	}
	return 0;
}

opendfx::Graph* GraphManager::mergeGraphs(){
	opendfx::Graph *graph0 = new opendfx::Graph{"Merged"};
	stagedGraphs_mutex.lock();
	for (std::vector<opendfx::Graph *>::iterator it = stagedGraphs.begin() ; it != stagedGraphs.end(); ++it)
	{
		opendfx::Graph* graph = *it;
		graph0->copyGraph(graph);
	}
	stagedGraphs_mutex.unlock();
	std::cout << "\nNo of accels  = " << graph0->countAccel();
	std::cout << "\nNo of buffers = " << graph0->countBuffer(); 
	std::cout << "\nNo of links   = " << graph0->countLink(); 
	return graph0;
}

int GraphManager::stageGraphs(int slots){
	int remainingSlots = slots;
	int accelCounts = 0;
	std::vector<opendfx::Graph *>*graphs;
	while(1){
		//std::cout  << "stageGraph\n" ;
		//listGraphs();
		for (int i = 0; i < 3; i++){
			graphQueue_mutex.lock();
			graphs = &graphsQueue[3-i-1];
			if (remainingSlots > 0){
				//std::cout << graphs->size() << std::endl;
				for (std::vector<opendfx::Graph *>::iterator it = graphs->begin() ; it != graphs->end(); ++it)
				{
					opendfx::Graph* graph = *it;
					//std::cout << "##" << graph->countAccel() << std::endl;
					accelCounts = graph->countAccel();
					if (remainingSlots >= accelCounts && accelCounts > 0){
						remainingSlots = remainingSlots - accelCounts;
						stagedGraphs_mutex.lock();
						stagedGraphs.push_back(graph);
						stagedGraphs_mutex.unlock();
						graphs->erase(std::remove(graphs->begin(), graphs->end(), graph), graphs->end());
					}
					else{
						break;
					}
				}
			}
			//std::cout << "stageGraph done ...\n";
			graphQueue_mutex.unlock();
		}
	}
	return 0;
}

int GraphManager::scheduleGraph(){
	return 0;
}

/*void GraphManager::sigint_handler (int) {
  if (pthread_self() == main_thread) {
  std::cout << "\rQuitting.\n";
  quit = true;
  for (auto &t : all) pthread_kill(t.native_handle(), SIGINT);
  } else if (!quit) pthread_kill(main_thread, SIGINT);
  }

  template <decltype(signal)>
  void GraphManager::sighandler(int sig, sighandler_t handler) {
  signal(sig, handler);
  }

  template <decltype(sigaction)>
  void GraphManager::sighandler(int sig, sighandler_t handler) {
  struct sigaction sa = {};
  sa.sa_handler = handler;
  sa.sa_flags = SA_RESTART;
  sigaction(sig, &sa, NULL);
  }
  */

int GraphManager::startServices(int slots){
	//main_thread = pthread_self();
	stageGraphThread = new std::thread(&GraphManager::stageGraphs, this, slots);
	// sighandler<sigaction>(SIGINT, sigint_handler);
	return 0;
}

int GraphManager::stopServices(){
	stageGraphThread->join();
	return 0;
}
