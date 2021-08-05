#include <stdio.h>
#include "graph_api.h"

int testJson(void){	GRAPH_HANDLE gHandle	= Graph_CreateWithPriority("test", 2);
	char * strid = Graph_fromJson(gHandle, "{\
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
	printf("%s\n", Graph_toJson(gHandle));
	printf("%s\n", strid);
	return 0;
}
int main(void){
	printf("Testing Graph JSON API ...\n");
	testJson();
	printf("... Done\n");
	
	return 0;
}
