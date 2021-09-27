#include <stdio.h>
#include "graph_api.h"
#include "utils.h"
#include <stdint.h>

#define MAX_SLOTS 3 
#define IONODE_SIZE 32*1024*1024
#define BUFFER_SIZE 16*1024*1024
#define TRANSACTION_SIZE  32*1024*1024
#define DDR_BASED 0

int testGraph(void){
	GRAPH_HANDLE gHandle	= Graph_CreateWithPriority("test", 2);

	ACCEL_HANDLE inputHandle_00	= Graph_addInputNode	(gHandle, "Input", IONODE_SIZE);
	ACCEL_HANDLE aHandle_01		= Graph_addAccelByName	(gHandle, "AES128");
	ACCEL_HANDLE outputHandle_02= Graph_addOutputNode	(gHandle, "Output", IONODE_SIZE);

	BUFFER_HANDLE bHandle_00	= Graph_addBufferByName(gHandle, "Buff0", BUFFER_SIZE, DDR_BASED);
	BUFFER_HANDLE bHandle_01	= Graph_addBufferByName(gHandle, "Buff1", BUFFER_SIZE, DDR_BASED);

	LINK_HANDLE lHandle_00		= Graph_connectOutputBuffer(gHandle,  inputHandle_00,  bHandle_00, 0x00, TRANSACTION_SIZE, 0, 0);
	LINK_HANDLE lHandle_01		= Graph_connectInputBuffer (gHandle,      aHandle_01,  bHandle_00, 0x00, TRANSACTION_SIZE, 1, 0);
	LINK_HANDLE lHandle_02		= Graph_connectOutputBuffer(gHandle,      aHandle_01,  bHandle_01, 0x00, TRANSACTION_SIZE, 1, 0);
	LINK_HANDLE lHandle_03		= Graph_connectInputBuffer (gHandle, outputHandle_02,  bHandle_01, 0x00, TRANSACTION_SIZE, 2, 0);

	Graph_delAccel (gHandle, outputHandle_02);
	Graph_delBuffer(gHandle, bHandle_01);
	Graph_delLink  (gHandle, lHandle_03);
	_unused(lHandle_00);
	_unused(lHandle_01);
	_unused(lHandle_02);

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
