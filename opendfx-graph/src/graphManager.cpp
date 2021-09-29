/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
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
#include <dfx-mgr/daemon_helper.h>
#include <unistd.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using opendfx::GraphManager;

GraphManager::GraphManager(int slots) : slots(slots) {
	int i;
	//srand(time(0)); // Seed initialisation based on time
	//char * block;
	//short size = 1;
	std::ifstream urandom("/dev/urandom", std::ios::in|std::ios::binary); // Seed initialisation based on /dev/urandom
	urandom.read((char*)&i, sizeof(i));
	srand(i ^ time(0));
	urandom.close();
	//std::cout << i << " : " << time(0) << std::endl;
	//dfx_init();
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

opendfx::Graph* GraphManager::getStagedGraphByID(int id)
{
	opendfx::Graph* rgraph = NULL;
	graphQueue_mutex.lock();
	for (std::vector<opendfx::Graph *>::iterator it = stagedGraphs.begin() ; it != stagedGraphs.end(); ++it)
	{
	opendfx::Graph* graph = *it;
		if (graph->getId() == id){
			rgraph = *it;
			break;
		}
	}
	graphQueue_mutex.unlock();
	return rgraph;
}

bool GraphManager::ifGraphStaged(int id, int **fd, int **ids, int *size){
	//bool GraphManager::ifGraphStaged(int id){
	opendfx::Graph* graph = getStagedGraphByID(id);
	std::cout << "%%%%%%%%%%%%" << std::endl;
	if (graph != NULL && graph->getStatus() == opendfx::graphStatus::GraphStaged){
		std::cout << "%%%%%%%%%%%%" << std::endl;
		graph->getIODescriptors(fd, ids, size);
		return true;
	}
	else{
		return false;
	}
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

int GraphManager::executeStagedGraphs(){
	for (std::vector<opendfx::Graph *>::iterator it = stagedGraphs.begin() ; it != stagedGraphs.end(); ++it)
	{
		opendfx::Graph* tGraph = *it;

		if (tGraph->execute() == 0){
			tGraph->setStatus(opendfx::graphStatus::GraphExecuted);
		}
		//if (tGraph->executeBypass() == 0){
		//	tGraph->setStatus(opendfx::graphStatus::GraphExecuted);
		//}
		tGraph->removeCompletedSchedule();
	}
	return 0;
}
int GraphManager::unstageGraphs(){
	for (std::vector<opendfx::Graph *>::iterator it = stagedGraphs.begin() ; it != stagedGraphs.end(); ++it)
	{
		opendfx::Graph* tGraph = *it;
		if(tGraph->getStatus() == opendfx::graphStatus::GraphExecuted){
			tGraph->setStatus(opendfx::graphStatus::GraphUnstaged);
		std::cout << "### @@@ !!!" << std::endl;
			tGraph->deallocateAccelResources();
		std::cout << "### @@@ !!!" << std::endl;
			tGraph->deallocateBuffers();
			tGraph->deallocateIOBuffers();
			this->stagedGraphs.erase(it);
			delete tGraph;
			break;
		}
	}

	return 0;
}

int GraphManager::mergeGraphs(){
	opendfx::Graph mergedGraph{"Merged"};
	for (std::vector<opendfx::Graph *>::iterator it = stagedGraphs.begin() ; it != stagedGraphs.end(); ++it)
	{
		opendfx::Graph* graph = *it;
		if (graph->getStatus() == opendfx::graphStatus::GraphIdle){
			graph->allocateIOBuffers();
			graph->allocateBuffers();
			graph->allocateAccelResources();
			graph->createExecutionDependencyList();
			graph->getExecutionDependencyList();
			graph->createScheduleList();
			graph->setStatus(opendfx::graphStatus::GraphStaged);
		}
		mergedGraph.copyGraph(graph);
		std::cout << "scheduled ..." << std::endl;
		std::cout << "No of accels  = " << mergedGraph.countAccel() << std::endl;
		std::cout << "No of buffers = " << mergedGraph.countBuffer() << std::endl; 
		std::cout << "No of links   = " << mergedGraph.countLink() << std::endl; 
	
	}
	std::string dataToWrite = mergedGraph.toJson();
	//std::stringstream jsonStream;
	//jsonStream << document.dump(true);	
	std::ofstream filePtr;
	filePtr.open("/run/mergedGraph.json", std::ios::trunc);
	//std::string dataToWrite = jsonStream.str();
	//std::cout << dataToWrite << std::endl; 
	filePtr << dataToWrite.c_str();

	filePtr.close();
	logQueues();
	return 0;
}

int GraphManager::stageGraphs(){
	int remainingSlots = slots;
	int accelCounts = 0;
	std::vector<opendfx::Graph *>*graphs;
	std::cout << "service started ..." << std::endl;
	int graphsInQueue = 0;
	while(1){
		graphsInQueue = 0;
		for (int i = 0; i < 3; i++){
			graphQueue_mutex.lock();
			graphs = &graphsQueue[3-i-1];
			if (remainingSlots > 0){
				for (std::vector<opendfx::Graph *>::iterator it = graphs->begin() ; it != graphs->end(); ++it)
				{
					graphsInQueue ++;
					opendfx::Graph* graph = *it;
					accelCounts = graph->countAccel();
					if (remainingSlots > 0 && remainingSlots >= accelCounts){ // && accelCounts > 0){
						std::cout << "##" <<  std::endl;
						remainingSlots = remainingSlots - accelCounts;
						std::cout << "staged : " << utils::int2str(graph->getId()) << std::endl;
						stagedGraphs.push_back(graph);
						std::cout << "###" << std::endl;
						graphs->erase(std::remove(graphs->begin(), graphs->end(), graph), graphs->end());
						if(graphs->size() == 0){
							break;
						}
					}
						else{
							break;
						}
					}
				}
				graphQueue_mutex.unlock();
			}
			graphsInQueue += stagedGraphs.size();
			mergeGraphs();
			executeStagedGraphs();
			graphQueue_mutex.lock();
			for (std::vector<opendfx::Graph *>::iterator it = stagedGraphs.begin() ; it != stagedGraphs.end(); ++it)
			{
				opendfx::Graph* tGraph = *it;
				if(tGraph->getStatus() == opendfx::graphStatus::GraphExecuted){
					tGraph->setStatus(opendfx::graphStatus::GraphUnstaged);
					tGraph->deallocateAccelResources();
					tGraph->deallocateBuffers();
					tGraph->deallocateIOBuffers();
					this->stagedGraphs.erase(it);
					accelCounts = tGraph->countAccel();
					remainingSlots = remainingSlots + accelCounts;
					unstagedGraphs.push_back(tGraph);
					//delete tGraph;
					break;
				}
			}

			graphQueue_mutex.unlock();
			//std::cout << graphsInQueue << std::endl;
			if (graphsInQueue == 0){
				sleep(1);
			}
		}
		std::cout << "service done" << std::endl;
		return 0;
	}

	int GraphManager::scheduleGraph(){
		while(1){
		}
		return 0;
	}


	int GraphManager::logQueues () {
		json document;
		json mergedQueueDoc;
		for (std::vector<opendfx::Graph *>::iterator it = stagedGraphs.begin() ; it != stagedGraphs.end(); ++it){
			opendfx::Graph* graph = *it;
			json gDoc;
			gDoc["id"]      = utils::int2str(graph->getId());
			gDoc["name"]    = graph->getName();
			gDoc["status"]	= graph->getStatus();
			mergedQueueDoc.push_back(gDoc);
		}
		document["scheduled"] = mergedQueueDoc;
		for (int i = 0; i < 3; i++){
			json QueueDoc;
			auto graphs = &graphsQueue[3-i-1];
			for (std::vector<opendfx::Graph *>::iterator it = graphs->begin() ; it != graphs->end(); ++it)
			{
				opendfx::Graph* graph = *it;
				json gDoc;
				gDoc["id"]      = utils::int2str(graph->getId());
				gDoc["name"]    = graph->getName();
				gDoc["status"]	= graph->getStatus();
				QueueDoc.push_back(gDoc);
			}
			switch(i){
				case 0: document["queue0"] = QueueDoc;
						break;
				case 1: document["queue1"] = QueueDoc;
						break;
				case 2: document["queue2"] = QueueDoc;
						break;
			}
		}
		std::stringstream jsonStream;
		jsonStream << document.dump(true);
		std::ofstream filePtr;
		filePtr.open("/run/queues.json", std::ios::trunc);

		std::string dataToWrite = jsonStream.str();
		//std::cout << dataToWrite << std::endl; 
		filePtr << dataToWrite.c_str();

		filePtr.close();
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

	int GraphManager::startServices(){
		stageGraphThread = new std::thread(&GraphManager::stageGraphs, this);
		return 0;
	}

	int GraphManager::stopServices(){
		stageGraphThread->join();
		return 0;
	}
