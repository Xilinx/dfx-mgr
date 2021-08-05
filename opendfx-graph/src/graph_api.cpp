#ifdef __cplusplus
#include <iostream>
#include "graph.hpp"
#endif

#include <stdlib.h>
#include <string.h>
#include "graph_api.h"

#ifdef __cplusplus
extern "C" {
#endif

GRAPH_HANDLE Graph_Create(const char* name){
	std::string sname(name);
    opendfx::Graph *graph = new opendfx::Graph(sname, 0);
	return (GRAPH_HANDLE) graph;	
}

GRAPH_HANDLE Graph_CreateWithPriority(const char* name, int priority){
	std::string sname(name);
    opendfx::Graph *graph = new opendfx::Graph(sname, priority);
	return (GRAPH_HANDLE) graph;	
}

int Graph_Distroy (GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	delete (graph);
	return 0;
}

char* Graph_getInfo (GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->getInfo();
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

ACCEL_HANDLE    Graph_addAccelByName    (GRAPH_HANDLE gHandle, const char* name){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	auto accel   = graph->addAccel(name);
	return (ACCEL_HANDLE) accel;
}

ACCEL_HANDLE    Graph_addInputNode      (GRAPH_HANDLE gHandle, const char* name){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	auto accel   = graph->addInputNode(name);
	return (ACCEL_HANDLE) accel;
}

ACCEL_HANDLE    Graph_addOutputNode     (GRAPH_HANDLE gHandle, const char* name){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	auto accel   = graph->addOutputNode(name);
	return (ACCEL_HANDLE) accel;
}

BUFFER_HANDLE   Graph_addBufferByName   (GRAPH_HANDLE gHandle, const char* name){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	auto buffer   = graph->addBuffer(name);
	return (BUFFER_HANDLE) buffer;
}

//BUFFER_HANDLE   Graph_addBufferByHandle (GRAPH_HANDLE gHandle, BUFFER_HANDLE bHandle){
//}


LINK_HANDLE     Graph_connectInputBuffer(GRAPH_HANDLE gHandle, ACCEL_HANDLE aHandle, BUFFER_HANDLE  bHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	opendfx::Accel *accel = (opendfx::Accel *) aHandle;
	opendfx::Buffer *buffer = (opendfx::Buffer *) bHandle;
	auto link   = graph->connectInputBuffer(accel, buffer);
	return (LINK_HANDLE) link;
}

LINK_HANDLE     Graph_connectOutputBuffer(GRAPH_HANDLE gHandle, ACCEL_HANDLE aHandle, BUFFER_HANDLE  bHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	opendfx::Accel *accel = (opendfx::Accel *) aHandle;
	opendfx::Buffer *buffer = (opendfx::Buffer *) bHandle;
	auto link   = graph->connectOutputBuffer(accel, buffer);
	return (LINK_HANDLE) link;
}

//LINK_HANDLE     Graph_addLinkByHandle   (GRAPH_HANDLE gHandle, LINK_HANDLE  lHandle){}


int Graph_delAccel          (GRAPH_HANDLE gHandle, ACCEL_HANDLE aHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	opendfx::Accel *accel = (opendfx::Accel *) aHandle;
	graph->delAccel(accel);
	return 0;
}

int Graph_delBuffer         (GRAPH_HANDLE gHandle, BUFFER_HANDLE bHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	opendfx::Buffer *buffer = (opendfx::Buffer *) bHandle;
	graph->delBuffer(buffer);
	return 0;
}

int Graph_delLink           (GRAPH_HANDLE gHandle, LINK_HANDLE  lHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	opendfx::Link *link = (opendfx::Link *) lHandle;
	graph->delLink(link);
	return 0;
}

int Graph_countAccel(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->countAccel();
}

int Graph_countBuffer(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->countBuffer();
}

int Graph_countLink(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->countLink();
}

int Graph_listAccels(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->listAccels();
}

int Graph_listBuffers(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->listBuffers();
}

int Graph_listLinks(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->listLinks();
}

char * Graph_jsonAccels(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->jsonAccels();
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

char * Graph_jsonBuffers(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->jsonBuffers();
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

char * Graph_jsonLinks(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->jsonLinks();
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

char * Graph_toJson(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->toJson();
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

char * Graph_fromJson(GRAPH_HANDLE gHandle, char * jsonstr){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->fromJson(jsonstr);
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;

}

char * Graph_jsonAccelsDbg(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->jsonAccels(true);
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

char * Graph_jsonBuffersDbg(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->jsonBuffers(true);
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

char * Graph_jsonLinksDbg(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->jsonLinks(true);
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

char * Graph_toJsonDbg(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	std::string str = graph->toJson(true);
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}


#ifdef __cplusplus
}
#endif

