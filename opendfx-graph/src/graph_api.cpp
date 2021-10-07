#ifdef __cplusplus
#include <iostream>
#include "graph.hpp"
#include "graphManager.hpp"
#endif

#include <stdlib.h>
#include <string.h>
#include "graph_api.h"
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>

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
	//graph->redeallocateIOBuffers();
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

ACCEL_HANDLE    Graph_addFallbackAccelByName    (GRAPH_HANDLE gHandle, const char* name){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	auto accel   = graph->addAccelWithFallback(name);
	return (ACCEL_HANDLE) accel;
}

ACCEL_HANDLE    Graph_addInputNode      (GRAPH_HANDLE gHandle, const char* name, int bSize){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	auto accel   = graph->addInputNode(name, bSize);
	return (ACCEL_HANDLE) accel;
}

ACCEL_HANDLE    Graph_addOutputNode     (GRAPH_HANDLE gHandle, const char* name, int bSize){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	auto accel   = graph->addOutputNode(name, bSize);
	return (ACCEL_HANDLE) accel;
}

BUFFER_HANDLE   Graph_addBufferByName   (GRAPH_HANDLE gHandle, const char* name, int bSize, int bType){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	auto buffer   = graph->addBuffer(name, bSize, bType);
	return (BUFFER_HANDLE) buffer;
}

//BUFFER_HANDLE   Graph_addBufferByHandle (GRAPH_HANDLE gHandle, BUFFER_HANDLE bHandle){
//}


LINK_HANDLE     Graph_connectInputBuffer(GRAPH_HANDLE gHandle, ACCEL_HANDLE aHandle, BUFFER_HANDLE  bHandle,
										int offset, int transactionSize, int transactionIndex, int channel){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	opendfx::Accel *accel = (opendfx::Accel *) aHandle;
	opendfx::Buffer *buffer = (opendfx::Buffer *) bHandle;
	auto link   = graph->connectInputBuffer(accel, buffer, offset, transactionSize, transactionIndex, channel);
	return (LINK_HANDLE) link;
}

LINK_HANDLE     Graph_connectOutputBuffer(GRAPH_HANDLE gHandle, ACCEL_HANDLE aHandle, BUFFER_HANDLE  bHandle,
										int offset, int transactionSize, int transactionIndex, int channel){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	opendfx::Accel *accel = (opendfx::Accel *) aHandle;
	opendfx::Buffer *buffer = (opendfx::Buffer *) bHandle;
	auto link   = graph->connectOutputBuffer(accel, buffer, offset, transactionSize, transactionIndex, channel);
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

int Graph_fromJson(GRAPH_HANDLE gHandle, char * jsonstr){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return  graph->fromJson(jsonstr);
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


int redeallocateIOBuffers(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->redeallocateIOBuffers();
}

int	Graph_submit(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->submit();
}

int	Graph_isScheduled(GRAPH_HANDLE gHandle){
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graph->isScheduled();
}

int Data2IO(ACCEL_HANDLE aHandle, uint8_t *ptr, int size){
	opendfx::Accel *accel = (opendfx::Accel *) aHandle;
	memcpy(accel->ptr, ptr, size);
	accel->post();
	return 0;
}

int IO2Data(ACCEL_HANDLE aHandle, uint8_t *ptr, int size){
	opendfx::Accel *accel = (opendfx::Accel *) aHandle;
	accel->wait();
	memcpy(ptr, accel->ptr, size);
	return 0;
}

GRAPH_MANAGER_HANDLE GraphManager_Create(int slots){
	opendfx::GraphManager *graphManager = new opendfx::GraphManager(slots);
	return (GRAPH_MANAGER_HANDLE) graphManager;	
}

GRAPH_MANAGER_HANDLE GraphManager_Distroy(GRAPH_MANAGER_HANDLE gmHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	delete (graphManager);
	return 0;
}

//char *			GraphManager_getInfo(GRAPH_MANAGER_HANDLE gmHandle){
//	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
//	std::string str = graphManager->getInfo();
//	char *cstr = new char[str.length() + 1];
//	strcpy(cstr, str.c_str());
//	return cstr;
//}

int				GraphManager_addGraph(GRAPH_MANAGER_HANDLE gmHandle, GRAPH_HANDLE gHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graphManager->addGraph(graph);
}

int				GraphManager_delGraph(GRAPH_MANAGER_HANDLE gmHandle, GRAPH_HANDLE gHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	opendfx::Graph *graph = (opendfx::Graph *) gHandle;
	return graphManager->delGraph(graph);
}

int				GraphManager_listGraphs(GRAPH_MANAGER_HANDLE gmHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	return graphManager->listGraphs();
}

int				GraphManager_mergeGraphs(GRAPH_MANAGER_HANDLE gmHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	return graphManager->mergeGraphs();
}

int				GraphManager_stageGraphs(GRAPH_MANAGER_HANDLE gmHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	return graphManager->mergeGraphs();
}

int				GraphManager_scheduleGraph(GRAPH_MANAGER_HANDLE gmHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	return graphManager->scheduleGraph();
}

int				GraphManager_startServices(GRAPH_MANAGER_HANDLE gmHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	return graphManager->startServices();
}

int				GraphManager_stopServices(GRAPH_MANAGER_HANDLE gmHandle){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	return graphManager->stopServices();
}

GRAPH_HANDLE 	GraphManager_getStagedGraphByID(GRAPH_MANAGER_HANDLE gmHandle, int id){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	opendfx::Graph *graph = graphManager->getStagedGraphByID(id);
	return (GRAPH_HANDLE) graph;	
}

int 			GraphManager_ifGraphStaged(GRAPH_MANAGER_HANDLE gmHandle, int id, int **fd, int **ids, int *size){
	opendfx::GraphManager *graphManager = (opendfx::GraphManager *) gmHandle;
	return (int) graphManager->ifGraphStaged(id, fd, ids, size);
}


#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int notify(char * path)
{
	int length, i = 0;
	int fd;
	int wd;
	char buffer[EVENT_BUF_LEN];

	/*creating the INOTIFY instance*/
	fd = inotify_init();

	/*checking for error*/
	if ( fd < 0 ) {
		perror( "inotify_init" );
	}

	/*adding the “/tmp” directory into watch list. Here, the suggestion is to validate the existence of the directory before adding into monitoring list.*/
	wd = inotify_add_watch( fd, path, IN_CREATE | IN_MODIFY );

	/*read to determine the event change happens on “/tmp” directory. Actually this read blocks until the change event occurs*/ 

	length = read( fd, buffer, EVENT_BUF_LEN ); 

	/*checking for error*/
	if ( length < 0 ) {
		perror( "read" );
	}  
	/*actually read return the list of change events happens. Here, read the change event one by one and process it accordingly.*/
	while ( i < length ) { 
		struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ]; 
	    if ( event->len ) {
			if ( event->mask & IN_CREATE ) {
				if ( event->mask & IN_ISDIR ) {
					printf( "New directory %s created.\n", event->name );
				}
				else {
					printf( "New file %s created.\n", event->name );
				}
			}
			else if ( event->mask & IN_MODIFY ) {
				if ( event->mask & IN_ISDIR ) {
					printf( "Directory %s deleted.\n", event->name );
				}
				else {
					printf( "File %s deleted.\n", event->name );
				}
			}
		}
		i += EVENT_SIZE + event->len;
	}
	/*removing the “/tmp” directory from the watch list.*/
	inotify_rm_watch( fd, wd );

	/*closing the INOTIFY instance*/
	close( fd );
	return 0;
}

#ifdef __cplusplus
}
#endif

