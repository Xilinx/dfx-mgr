// main.cpp
#include <iostream>
#include <dlfcn.h>
#include <iomanip>
#include "device.h"
#include "graph.hpp"
#include "graphManager.hpp"
#include "utils.hpp"

int main(int argc, char **argv) {
	_unused(argc);
	_unused(argv);
	opendfx::Graph *graph;

	graph = new opendfx::Graph{"G"};
	int id = graph->fromJson("{\
       \"accels\": [\
        {\
		\"bSize\": 33554432, \
         \"id\": \"3561\",\
         \"name\": \"INPUT\",\
         \"type\": 1\
        },\
        {\
		\"bSize\": 0, \
         \"id\": \"41b1\",\
         \"name\": \"AES\",\
         \"type\": 0\
        },\
        {\
		\"bSize\": 33554432, \
         \"id\": \"4ff2\",\
         \"name\": \"OUTPUT\",\
         \"type\": 2\
        }\
       ],\
       \"buffers\": [\
        {\
		 \"bSize\": 16777216, \
		 \"bType\": 0, \
         \"id\": \"2827\",\
         \"name\": \"BUFF\"\
        },\
        {\
		 \"bSize\": 16777216, \
		 \"bType\": 0, \
         \"id\": \"841a\",\
         \"name\": \"BUFF\"\
        }\
       ],\
       \"id\": \"755\",\
       \"links\": [\
        {\
         \"accel\": \"3561\",\
         \"buffer\": \"2827\",\
		 \"dir\": 1,\
         \"id\": \"5673\",\
		 \"channel\": 0,\
		 \"offset\": 0,\
		 \"transactionIndex\": 0,\
		 \"transactionSize\": 33554432\
        },\
        {\
         \"accel\": \"41b1\",\
         \"buffer\": \"2827\",\
		 \"dir\": 0,\
         \"id\": \"c97f\",\
		 \"channel\": 0,\
		 \"offset\": 0,\
		 \"transactionIndex\": 1,\
		 \"transactionSize\": 33554432\
        },\
        {\
         \"accel\": \"41b1\",\
         \"buffer\": \"841a\",\
		 \"dir\": 1,\
         \"id\": \"7203\",\
		 \"channel\": 0,\
		 \"offset\": 0,\
		 \"transactionIndex\": 1,\
		 \"transactionSize\": 33554432\
        },\
        {\
         \"accel\": \"4ff2\",\
         \"buffer\": \"841a\",\
		 \"dir\": 0,\
         \"id\": \"981d\",\
		 \"channel\": 0,\
		 \"offset\": 0,\
		 \"transactionIndex\": 2,\
		 \"transactionSize\": 33554432\
        }\
       ],\
       \"name\": \"G\"\
      }");
	std::cout << std::setw(4) << graph->toJson() << std::endl;
	std::cout << std::hex << id << std::endl;

	return 0;
}
