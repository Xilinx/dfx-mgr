/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "metadata.h"
#include "graph.h"
#include "layer0/debug.h"
#define JSMN_PARENT_LINKS
#define JSMN_HEADER
#include "jsmn.h"
#include "abstractGraph.h"
//#include "uio.h"




static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int json2fallback(Json_t* json, Metadata_t *metadata){
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return 1;
	}
	for (int i = 1; i < r; i++) {
		if (jsoneq(json->data, &t[i], "Behaviour") == 0) {
			sscanf(json->data + t[i + 1].start, "0x%x", &(metadata->fallback.behaviour));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "fallback_Lib") == 0) {
			sprintf(metadata->fallback.lib, "%.*s", t[i + 1].end - t[i + 1].start,
				json->data + t[i + 1].start);
			i++;
		}
	}
	return 0;
}

int json2interrm(Json_t* json, Metadata_t *metadata){
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return 1;
	}
	for (int i = 1; i < r; i++) {
		if (jsoneq(json->data, &t[i], "Compatible") == 0) {
			INFO("%.*s\n", t[i + 1].end - t[i + 1].start,
				json->data + t[i + 1].start);
			INFO("################# %d\n", strcmp(json->data + t[i + 1].start, "True"));
			if (strcmp(json->data + t[i + 1].start, "True") > 0){
				INFO("################# %d\n", strcmp(json->data + t[i + 1].start, "True"));
				metadata->interRM.compatible = INTER_RM_COMPATIBLE;
			}
			else{
				metadata->interRM.compatible = INTER_RM_NOT_COMPATIBLE;
			}
			i++;
		}
		
		if (jsoneq(json->data, &t[i], "Address") == 0) {
			sscanf(json->data + t[i + 1].start, "0x%lx", &(metadata->interRM.address));
			i++;
		}
	}
	return 0;
}

int json2metadata(Json_t* json, Metadata_t *metadata){
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return 1;
	}
	for (int i = 1; i < r; i++) {
		if (jsoneq(json->data, &t[i], "fallback") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			Json_t* subJson = malloc(sizeof(Json_t));
			subJson->data = json->data + t[i + 1].start;
			subJson->size = t[i + 1].end - t[i + 1].start;
			json2fallback(subJson, metadata);
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Input_Data_Block_Size") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			sscanf(json->data + t[i + 1].start, "0x%lx", &(metadata->Input_Data_Block_Size));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Output_Data_Block_Size") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			sscanf(json->data + t[i + 1].start, "0x%lx", &(metadata->Output_Data_Block_Size));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Input_Channel_Count") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			sscanf(json->data + t[i + 1].start, "%hhd", &(metadata->Input_Channel_Count));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Output_Channel_Count") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			sscanf(json->data + t[i + 1].start, "%hhd", &(metadata->Output_Channel_Count));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Inter_RM") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			Json_t* subJson = malloc(sizeof(Json_t));
			subJson->data = json->data + t[i + 1].start;
			subJson->size = t[i + 1].end - t[i + 1].start;
			json2interrm(subJson, metadata);
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Config_Buffer_Size") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			sscanf(json->data + t[i + 1].start, "0x%lx", &(metadata->Config_Buffer_Size));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Input_Buffer_Size") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			sscanf(json->data + t[i + 1].start, "0x%lx", &(metadata->Input_Buffer_Size));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Output_Buffer_Size") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			sscanf(json->data + t[i + 1].start, "0x%lx", &(metadata->Output_Buffer_Size));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Throughput") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			sscanf(json->data + t[i + 1].start, "%ld", &(metadata->Throughput));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "DMA_type") == 0) {
			if(strcmp(json->data + t[i + 1].start, "HLS_MULTICHANNEL_DMA"))
				metadata->DMA_type = HLS_MULTICHANNEL_DMA;
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			i++;
		}
		else if (jsoneq(json->data, &t[i], "Accel_Handshake_Type") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			i++;
		}
	}
	return 0;
}

int json2device(Json_t* json, Metadata_t *metadata, int index){
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return 1;
	}
	for (int i = 1; i < r; i++) {
		if (jsoneq(json->data, &t[i], "dev_name") == 0) {
			sprintf(metadata->plDevices[index].name, "%.*s", t[i + 1].end - t[i + 1].start,
				json->data + t[i + 1].start);
			i++;
		}
		else if (jsoneq(json->data, &t[i], "reg_base") == 0) {
			sscanf(json->data + t[i + 1].start, "0x%lx", &(metadata->plDevices[index].base));
			i++;
		}
		else if (jsoneq(json->data, &t[i], "reg_size") == 0) {
			sscanf(json->data + t[i + 1].start, "%ld", &(metadata->plDevices[index].size));
			i++;
		}
	}
	return 0;
}

int json2devices(Json_t* json, Metadata_t *metadata){
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	if (r < 1 || t[0].type != JSMN_ARRAY) {
		printf("Object expected\n");
		return 1;
	}
	metadata->plDevices_Count = t[0].size;
	for (int i = 0, j=0; i < r; i++) {
		if (t[i].parent != 0) {
			continue; /* We expect groups to be an array of strings */
		}
		Json_t* subJson = malloc(sizeof(Json_t));
		subJson->data = json->data + t[i].start;
		subJson->size = t[i].end - t[i].start;
		json2device(subJson, metadata, j);
		j++;
	}
	return 0;
}

int json2meta(Json_t* json, Metadata_t *metadata){
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return 1;
	}
	/* Loop over all keys of the root object */
	for (int i = 1; i < r; i++) {
		if (jsoneq(json->data, &t[i], "accel_type") == 0) {
			sprintf(metadata->accel_type, "%.*s", t[i + 1].end - t[i + 1].start,
				json->data + t[i + 1].start);
			i++;
		}
		else if (jsoneq(json->data, &t[i], "accel_devices") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			Json_t* subJson = malloc(sizeof(Json_t));
			subJson->data = json->data + t[i + 1].start;
			subJson->size = t[i + 1].end - t[i + 1].start;
			json2devices(subJson, metadata);
			i++;
		}
		else if (jsoneq(json->data, &t[i], "accel_metadata") == 0) {
			//printf("%.*s\n", t[i + 1].end - t[i + 1].start,
			//	json->data + t[i + 1].start);
			Json_t* subJson = malloc(sizeof(Json_t));
			subJson->data = json->data + t[i + 1].start;
			subJson->size = t[i + 1].end - t[i + 1].start;
			json2metadata(subJson, metadata);
			i++;
		}
	}
	return 0;	
} 

int file2json(char* filename, Json_t *json){
	FILE *f = fopen(filename, "rb");
	fseek(f, 0, SEEK_END);
	json->size = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	json->data = malloc(json->size + 1);
	int len = fread(json->data, json->size, 1, f);
	_unused(len);
	fclose(f);
	json->data[json->size] = 0;
	INFO("%s\n", json->data);
	return 0;
}

int printMeta(Metadata_t *metadata){
	INFOP("accel_type : %s\n", metadata->accel_type);
	INFOP("fallback :\n");
	INFOP("\tlib : %s\n", metadata->fallback.lib);
	INFOP("\tbehaviour : %d\n", metadata->fallback.behaviour);

	INFOP("interRM :\n");
	INFOP("\tcompatible : %d\n", metadata->interRM.compatible);
	INFOP("\taddress : %lx\n", metadata->interRM.address);

	for(int i = 0; i < metadata->plDevices_Count; i ++){
		INFOP("plDevice :\n");
		INFOP("\tname : %s\n", metadata->plDevices[i].name);
		INFOP("\tbase : %lx\n", metadata->plDevices[i].base);
		INFOP("\tsize : %lx\n", metadata->plDevices[i].size);
		INFOP("\ttype : %d\n", metadata->plDevices[i].type);
	}

	INFOP("Input_Data_Block_Size : %lx\n", metadata->Input_Data_Block_Size);
	INFOP("Output_Data_Block_Size: %lx\n", metadata->Output_Data_Block_Size);
	INFOP("Input_Channel_Count : %d\n", metadata->Input_Channel_Count);
	INFOP("Output_Channel_Count : %d\n", metadata->Output_Channel_Count);
	INFOP("Config_Buffer_Size : %lx\n", metadata->Config_Buffer_Size);
	INFOP("Input_Buffer_Size : %lx\n", metadata->Input_Buffer_Size);
	INFOP("Output_Buffer_Size : %lx\n", metadata->Output_Buffer_Size);
	INFOP("Throughput : %ld\n", metadata->Throughput);
	INFOP("DMA_type : %d\n", metadata->DMA_type);
	INFOP("Accel_Handshake_Type : %d\n", metadata->Accel_Handshake_Type);
	return 0;
}

int json2accelNode(Json_t* json, AbstractGraph_t *graph, int index){
	_unused(json);	
	_unused(graph);	
	_unused(index);	
	return 0;
}
int json2accelNodes(Json_t* json, AbstractGraph_t *graph){
	jsmn_parser p;
	jsmntok_t t[128];
	jsmn_init(&p);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	if (r < 1 || t[0].type != JSMN_ARRAY) {
		printf("Object expected\n");
		return 1;
	}
	printf("%d\n", t[0].size);
	for (int i = 0, j=0; i < r; i++) {
		if (t[i].parent != 0) {
			continue;
		}
		printf("%.*s\n", t[i].end - t[i].start,
			json->data + t[i].start);
		Json_t* subJson = malloc(sizeof(Json_t));
		subJson->data = json->data + t[i].start;
		subJson->size = t[i].end - t[i].start;
		json2accelNode(subJson, graph, j);
		j++;
	}
	return 0;
}

int json2buffNodes(Json_t* json, AbstractGraph_t *graph){
	jsmn_parser p;
	jsmntok_t t[128];
	jsmn_init(&p);
	_unused(graph);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	if (r < 1 || t[0].type != JSMN_ARRAY) {
		printf("Object expected\n");
		return 1;
	}
	printf("%d\n", t[0].size);
	for (int i = 0, j=0; i < r; i++) {
		if (t[i].parent != 0) {
			continue;
		}
		printf("%.*s\n", t[i].end - t[i].start,
			json->data + t[i].start);
		Json_t* subJson = malloc(sizeof(Json_t));
		subJson->data = json->data + t[i].start;
		subJson->size = t[i].end - t[i].start;
		//json2accelNode(subJson, graph, j);
		j++;
	}
	return 0;
}

int json2links(Json_t* json, AbstractGraph_t *graph){
	jsmn_parser p;
	jsmntok_t t[128];
	jsmn_init(&p);
	_unused(graph);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	if (r < 1 || t[0].type != JSMN_ARRAY) {
		printf("Object expected\n");
		return 1;
	}
	printf("%d\n", t[0].size);
	for (int i = 0, j=0; i < r; i++) {
		if (t[i].parent != 0) {
			continue;
		}
		printf("%.*s\n", t[i].end - t[i].start,
			json->data + t[i].start);
		Json_t* subJson = malloc(sizeof(Json_t));
		subJson->data = json->data + t[i].start;
		subJson->size = t[i].end - t[i].start;
		//json2accelNode(subJson, graph, j);
		j++;
	}
	return 0;
}

int graphParser(char* jsonStr, AbstractGraph_t **graph){
	AbstractGraph_t *tgraph = *graph;
	Json_t* json = malloc(sizeof(Json_t));
	jsmn_parser p;
	jsmntok_t t[1024]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	json->size = strlen(jsonStr);
	json->data = malloc(json->size + 1);
	strcpy(json->data, jsonStr);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		INFO("Failed to parse JSON: %d\n", r);
		return 1;
	}
	
	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		INFO("Object expected\n");
		return 1;
	}
	for (int i = 1; i < r; i++) {
		if (t[i].parent != 0) {
                        continue; 
                }

		if (jsoneq(json->data, &t[i], "id") == 0) {
			sscanf(json->data + t[i + 1].start, "%d", &(tgraph->id));
			//INFO("id: %d\n", tgraph->id);
			i++;
		}
		else if (jsoneq(json->data, &t[i], "type") == 0) {
			sscanf(json->data + t[i + 1].start, "%hhd", &(tgraph->type));
			//INFO("type: %d\n", tgraph->type);
			i++;
		}
		else if (jsoneq(json->data, &t[i], "accelNode") == 0) {
			i ++;
			int arrayElementCount = t[i].size;
			int array = i;
			int objectElementCount = 0;
			int object = 0;
			i ++;
			for (int j = 0; j < arrayElementCount; j++) {
				if (t[i + j].parent == array) {
					objectElementCount = t[i + j].size;
					object = i + j;
					i ++;
					AbstractAccelNode_t *accelNode = malloc(sizeof(AbstractAccelNode_t));
					Element_t* element = (Element_t *) malloc(sizeof(Element_t));
        				element->node =  accelNode; 
        				tgraph->accelNodeID ++;
       					element->head = NULL;
				        element->tail = NULL;
       					addElement(&(tgraph->accelNodeHead), element);
					for (int k = 0; k < objectElementCount; k++) {
						if (t[i + j + k].parent == object) {
							if (jsoneq(json->data, &t[i + j + k], "id") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(accelNode->id));
							}
							else if (jsoneq(json->data, &t[i + j + k], "type") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%hhd", &(accelNode->type));
								if(accelNode->type == HW_NODE){
									tgraph->accelCount ++;
								}
							}
							else if (jsoneq(json->data, &t[i + j + k], "name") == 0) {
								sprintf(accelNode->name, "%.*s", 
									t[i + j + k + 1].end - t[i + j + k + 1].start,
									json->data + t[i + j + k + 1].start);
							}
							else if (jsoneq(json->data, &t[i + j + k], "size") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(accelNode->size));
							}
							else if (jsoneq(json->data, &t[i + j + k], "semaphore") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(accelNode->semaphore));
							}
						}
						else{
							i ++; k --;
							continue;
						}
					}
				}
				else{
					i ++; j --;
					continue;
				}
			}
			i++;
		}
		else if (jsoneq(json->data, &t[i], "buffNode") == 0) {
			i ++;
			int arrayElementCount = t[i].size;
			int array = i;
			int objectElementCount = 0;
			int object = 0;
			i ++;
			for (int j = 0; j < arrayElementCount; j++) {
				if (t[i + j].parent == array) {
					objectElementCount = t[i + j].size;
					object = i + j;
					i ++;
					AbstractBuffNode_t *buffNode = malloc(sizeof(AbstractBuffNode_t));
					Element_t* element = (Element_t *) malloc(sizeof(Element_t));
        				element->node =  buffNode; 
        				tgraph->buffNodeID ++;
       					element->head = NULL;
				        element->tail = NULL;
       					addElement(&(tgraph->buffNodeHead), element);
					for (int k = 0; k < objectElementCount; k++) {
						if (t[i + j + k].parent == object) {
							if (jsoneq(json->data, &t[i + j + k], "id") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(buffNode->id));
							}
							else if (jsoneq(json->data, &t[i + j + k], "type") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%hhd", &(buffNode->type));
							}
							else if (jsoneq(json->data, &t[i + j + k], "name") == 0) {
								sprintf(buffNode->name, "%.*s", 
									t[i + j + k + 1].end - t[i + j + k + 1].start,
									json->data + t[i + j + k + 1].start);
							}
							else if (jsoneq(json->data, &t[i + j + k], "size") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(buffNode->size));
							}
						}
						else{
							i ++; k --;
							continue;
						}
					}
				}
				else{
					i ++; j --;
					continue;
				}
			}
			i++;
		}
		else if (jsoneq(json->data, &t[i], "links") == 0) {
			i ++;
			int arrayElementCount = t[i].size;
			int array = i;
			int objectElementCount = 0;
			int object = 0;
			i ++;
			//printf("#%d %d\n", arrayElementCount, array);
			for (int j = 0; j < arrayElementCount; j++) {
				if (t[i + j].parent == array) {
					objectElementCount = t[i + j].size;
					object = i + j;
					//printf("%d %d\n", objectElementCount, object);
					i ++;
					AbstractLink_t *link = malloc(sizeof(AbstractLink_t));
					Element_t* element = (Element_t *) malloc(sizeof(Element_t));
        				element->node =  link; 
        				tgraph->linkID ++;
       					element->head = NULL;
				        element->tail = NULL;
       					addElement(&(tgraph->linkHead), element);
					for (int k = 0; k < objectElementCount; k++) {
						if (t[i + j + k].parent == object) {
							if (jsoneq(json->data, &t[i + j + k],
									"id") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(link->id));
							}
							else if (jsoneq(json->data, &t[i + j + k],
									"accelNode") == 0) {
								uint32_t nodeid;
								Element_t *accelElement = tgraph->accelNodeHead;
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(nodeid)); //link->accelNode));
								//printf("nodeid : %d\n", nodeid);
								while(accelElement != NULL){
									if (((AbstractAccelNode_t*)accelElement->node)->id == nodeid){
										link->accelNode = (AbstractAccelNode_t*)accelElement;
										break;
									}
									accelElement = accelElement->tail;
								}
							}
							else if (jsoneq(json->data, &t[i + j + k],
									"buffNode") == 0) {
								uint32_t nodeid;
								Element_t *buffElement = tgraph->buffNodeHead;
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(nodeid)); //link->accelNode));
								//printf("nodeid : %d\n", nodeid);
								while(buffElement != NULL){
									if (((AbstractBuffNode_t*)buffElement->node)->id == nodeid){
										link->buffNode = (AbstractBuffNode_t*)buffElement;
										break;
									}
									buffElement = buffElement->tail;
								}
							}
							else if (jsoneq(json->data, &t[i + j + k],
									"type") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%hhd", &(link->type));
							}
							else if (jsoneq(json->data, &t[i + j + k],
									"transactionIndex") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%hhd", &(link->transactionIndex));
							}
							else if (jsoneq(json->data, &t[i + j + k],
									"transactionSize") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(link->transactionSize));
							}
							else if (jsoneq(json->data, &t[i + j + k],
									"offset") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%d", &(link->offset));
							}
							else if (jsoneq(json->data, &t[i + j + k],
									"channel") == 0) {
								sscanf(json->data + t[i + j + k + 1].start, 
									"%hhd", &(link->channel));
							}
						}
						else{
							i ++; k --;
							continue;
						}
					}
				}
				else{
					i ++; j --;
					continue;
				}
			}
			i++;
		}
	}
	return 0;	
}



int graphIDParser(char* jsonStr){
	int id;
	Json_t* json = malloc(sizeof(Json_t));
	jsmn_parser p;
	jsmntok_t t[1024]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	json->size = strlen(jsonStr);
	json->data = malloc(json->size + 1);
	strcpy(json->data, jsonStr);
	int r = jsmn_parse(&p, json->data, json->size, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		INFO("Failed to parse JSON: %d\n", r);
		return 1;
	}
	
	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		INFO("Object expected\n");
		return 1;
	}
	for (int i = 1; i < r; i++) {
		if (t[i].parent != 0) {
                        continue; 
                }

		if (jsoneq(json->data, &t[i], "id") == 0) {
			sscanf(json->data + t[i + 1].start, "%d", &(id));
			break;
		}
	}
	return id;
}
