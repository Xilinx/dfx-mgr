#include <stdio.h>
#include "graph_api.h"

int testGraph(void){
	GRAPH_HANDLE gHandle	= Graph_CreateWithPriority("test", 2);

	ACCEL_HANDLE inputHandle_00	= Graph_addInputNode	(gHandle, "Input");
	ACCEL_HANDLE aHandle_01		= Graph_addAccelByName	(gHandle, "AES128");
	ACCEL_HANDLE outputHandle_02= Graph_addOutputNode	(gHandle, "Output");

	BUFFER_HANDLE bHandle_00	= Graph_addBufferByName(gHandle, "Buff0");
	BUFFER_HANDLE bHandle_01	= Graph_addBufferByName(gHandle, "Buff1");

	LINK_HANDLE lHandle_00		= Graph_connectOutputBuffer(gHandle, inputHandle_00,  bHandle_00);
	LINK_HANDLE lHandle_01		= Graph_connectInputBuffer (gHandle, aHandle_01,  bHandle_00);
	LINK_HANDLE lHandle_02		= Graph_connectOutputBuffer(gHandle, aHandle_01,  bHandle_01);
	LINK_HANDLE lHandle_03		= Graph_connectInputBuffer (gHandle, outputHandle_02, bHandle_01);

	Graph_delAccel (gHandle, outputHandle_02);
	Graph_delBuffer(gHandle, bHandle_01);
	Graph_delLink  (gHandle, lHandle_03);


	char* infostr =  Graph_getInfo(gHandle);
	printf("%s\n", infostr);
	char * json = Graph_toJson(gHandle);
	printf("%s\n", json);
	Graph_Distroy(gHandle);	
	return 0;
}
int main(void){
	printf("Testing Graph API ...\n");
	testGraph();
	printf("... Done\n");
	
	return 0;
}
