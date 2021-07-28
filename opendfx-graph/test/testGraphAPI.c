#include <stdio.h>
#include "graph_api.h"

int testGraph(void){
	GRAPH_HANDLE graph = Graph_Create("test");	
	return 0;
}
int main(void){
	printf("Testing Graph API ...\n");
	testGraph();
	printf("... Done\n");
	
	return 0;
}
