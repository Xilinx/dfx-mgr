// graph_api.h
#ifndef GRAPH_API_H_
#define GRAPH_API_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void *GRAPH_HANDLE;
typedef void *ACCEL_HANDLE;
typedef void *BUFFER_HANDLE;
typedef void *LINK_HANDLE;

GRAPH_HANDLE Graph_Create			(const char* name);
GRAPH_HANDLE Graph_CreateWithPriority(const char* name, int priority);
int          Graph_Distroy			(GRAPH_HANDLE gHandle);

char*			Graph_getInfo			(GRAPH_HANDLE gHandle);
ACCEL_HANDLE	Graph_addAccelByName	(GRAPH_HANDLE gHandle, const char*	name);
// ACCEL_HANDLE	Graph_addAccelByHandle	(GRAPH_HANDLE gHandle, ACCEL_HANDLE	aHandle);
ACCEL_HANDLE	Graph_addInputNode		(GRAPH_HANDLE gHandle, const char*	name, int bSize);
ACCEL_HANDLE	Graph_addOutputNode		(GRAPH_HANDLE gHandle, const char*	name, int bSize);

BUFFER_HANDLE	Graph_addBufferByName	(GRAPH_HANDLE gHandle, const char*	name, int bSize, int bType);
//BUFFER_HANDLE	Graph_addBufferByHandle	(GRAPH_HANDLE gHandle, BUFFER_HANDLE bHandle);
//LINK_HANDLE		Graph_addInputBuffer	(GRAPH_HANDLE gHandle, ACCEL_HANDLE	aHandle, BUFFER_HANDLE	bHandle);
//LINK_HANDLE		Graph_addOutputBuffer	(GRAPH_HANDLE gHandle, ACCEL_HANDLE	aHandle, BUFFER_HANDLE	bHandle);
LINK_HANDLE     Graph_connectInputBuffer(GRAPH_HANDLE gHandle, ACCEL_HANDLE aHandle, BUFFER_HANDLE  bHandle,
										int offset, int transactionSize, int transactionIndex, int channel);
LINK_HANDLE     Graph_connectOutputBuffer(GRAPH_HANDLE gHandle, ACCEL_HANDLE aHandle, BUFFER_HANDLE  bHandle,
										int offset, int transactionSize, int transactionIndex, int channel);
LINK_HANDLE		Graph_addLinkByHandle	(GRAPH_HANDLE gHandle, LINK_HANDLE	lHandle);

int				Graph_delAccel			(GRAPH_HANDLE gHandle, ACCEL_HANDLE aHandle);
int				Graph_delBuffer			(GRAPH_HANDLE gHandle, BUFFER_HANDLE bHandle);
int				Graph_delLink			(GRAPH_HANDLE gHandle, LINK_HANDLE	lHandle);
int				Graph_countAccel		(GRAPH_HANDLE gHandle);
int				Graph_countBuffer		(GRAPH_HANDLE gHandle);
int				Graph_countLink			(GRAPH_HANDLE gHandle);
int				Graph_listAccels		(GRAPH_HANDLE gHandle);
int				Graph_listBuffers		(GRAPH_HANDLE gHandle);
int				Graph_listLinks			(GRAPH_HANDLE gHandle);


char *			Graph_jsonAccels		(GRAPH_HANDLE gHandle);
char *        	Graph_jsonBuffers		(GRAPH_HANDLE gHandle);
char *        	Graph_jsonLinks			(GRAPH_HANDLE gHandle);
char *        	Graph_toJson			(GRAPH_HANDLE gHandle);
int				Graph_fromJson			(GRAPH_HANDLE gHandle, char * jsonstr);

char *			Graph_jsonAccelsDbg		(GRAPH_HANDLE gHandle);
char *        	Graph_jsonBuffersDbg	(GRAPH_HANDLE gHandle);
char *        	Graph_jsonLinksDbg		(GRAPH_HANDLE gHandle);
char *        	Graph_toJsonDbg			(GRAPH_HANDLE gHandle);


typedef void *GRAPH_MANAGER_HANDLE;

GRAPH_MANAGER_HANDLE GraphManager_Create(int slots);
GRAPH_MANAGER_HANDLE GraphManager_Distroy(GRAPH_MANAGER_HANDLE gmHandle);
//char *			GraphManager_getInfo(GRAPH_MANAGER_HANDLE gmHandle);
int				GraphManager_addGraph(GRAPH_MANAGER_HANDLE gmHandle, GRAPH_HANDLE gHandle);
int				GraphManager_delGraph(GRAPH_MANAGER_HANDLE gmHandle, GRAPH_HANDLE gHandle);
int				GraphManager_listGraphs(GRAPH_MANAGER_HANDLE gmHandle);
int				GraphManager_mergeGraphs(GRAPH_MANAGER_HANDLE gmHandle);
int				GraphManager_stageGraphs(GRAPH_MANAGER_HANDLE gmHandle);
int				GraphManager_scheduleGraph(GRAPH_MANAGER_HANDLE gmHandle);
int				GraphManager_startServices(GRAPH_MANAGER_HANDLE gmHandle);
int				GraphManager_stopServices(GRAPH_MANAGER_HANDLE gmHandle);
GRAPH_HANDLE 	GraphManager_getStagedGraphByID(GRAPH_MANAGER_HANDLE gmHandle, int id);
int 			GraphManager_ifGraphStaged(GRAPH_MANAGER_HANDLE gmHandle, int id);


#ifdef __cplusplus
}
#endif


#endif // GRAPH_API_H_
