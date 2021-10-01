// main.cpp
#include <iostream>
#include <dlfcn.h>
#include <iomanip>
#include <thread>
#include <signal.h>
#include <atomic>
#include <unistd.h>
#include "device.h"
#include "graph.hpp"
#include "graphManager.hpp"
#include "utils.hpp"

#define MAX_SLOTS 3 
#define IONODE_SIZE 32*1024*1024
#define BUFFER_SIZE 16*1024*1024
#define TRANSACTION_SIZE  32*1024*1024
#define DDR_BASED opendfx::buffertype::DDR_BASED
#define STREAM_BASED opendfx::buffertype::STREAM_BASED

int main(int argc, char **argv) {
	_unused(argc);
	_unused(argv);
	std::cout << "# main\n";
	opendfx::Graph    *graph[10];
	opendfx::Accel  *input00[10];
	opendfx::Accel *output02[10];
	int priority;
	int N = 5;
	int M = 1;

	for(int j = 0; j < M; j++){
		for(int i = 0; i < N; i++){
			priority = i % 3;
			graph[i] = new opendfx::Graph{"G", priority, true};
			input00[i]   = graph[i]->addInputNode("INPUT", IONODE_SIZE);
			auto accel01   = graph[i]->addAccel("AES128");
			output02[i]  = graph[i]->addOutputNode("OUTPUT", IONODE_SIZE);

			auto buffer00  = graph[i]->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);
			auto buffer01  = graph[i]->addBuffer("BUFF", BUFFER_SIZE, DDR_BASED);

			auto link00    = graph[i]->connectOutputBuffer(input00[i],  buffer00, 0x00, TRANSACTION_SIZE, 0, 0);
			auto link01    = graph[i]->connectInputBuffer (accel01,  buffer00, 0x00, TRANSACTION_SIZE, 1, 0);
			auto link02    = graph[i]->connectOutputBuffer(accel01,  buffer01, 0x00, TRANSACTION_SIZE, 1, 0);
			auto link03    = graph[i]->connectInputBuffer (output02[i], buffer01, 0x00, TRANSACTION_SIZE, 2, 0);
			std::cout << graph[i]->toJson() << std::endl;;
			graph[i]->submit();
			_unused(link00);
			_unused(link01);
			_unused(link02);
			_unused(link03);
		}
		for(int i = 0; i < N; i++){
			std::cout << "checking schedules \n";
			//graph[i]->isScheduled();
			//delete graph[i];
			while(!graph[i]->isScheduled()){
				usleep(100000);
			};
			//for(int j=0; j < 1024; j++){
			//	input00[i]->ptr[j] = j;
			//}
			input00[i]->post();
			output02[i]->wait();

		}
	}

	std::cout << "Client test done ... \n";
	return 0;
}
