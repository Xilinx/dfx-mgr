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
	std::string strid = graph->fromJson("{\
       \"accels\": [\
        {\
         \"id\": \"3561\",\
         \"name\": \"INPUT\",\
         \"type\": 1\
        },\
        {\
         \"id\": \"41b1\",\
         \"name\": \"AES\",\
         \"type\": 0\
        },\
        {\
         \"id\": \"4ff2\",\
         \"name\": \"OUTPUT\",\
         \"type\": 2\
        }\
       ],\
       \"buffers\": [\
        {\
         \"id\": \"2827\",\
         \"name\": \"BUFF\"\
        },\
        {\
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
         \"id\": \"5673\"\
        },\
        {\
         \"accel\": \"41b1\",\
         \"buffer\": \"2827\",\
		 \"dir\": 0,\
         \"id\": \"c97f\"\
        },\
        {\
         \"accel\": \"41b1\",\
         \"buffer\": \"841a\",\
		 \"dir\": 1,\
         \"id\": \"7203\"\
        },\
        {\
         \"accel\": \"4ff2\",\
         \"buffer\": \"841a\",\
		 \"dir\": 0,\
         \"id\": \"981d\"\
        }\
       ],\
       \"name\": \"G\"\
      }");
	std::cout << std::setw(4) << graph->toJson() << std::endl;
	std::cout << strid << std::endl;

	return 0;
}
