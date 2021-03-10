/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include "graph.h"

typedef struct Fallback Fallback_t;
typedef struct InterRM InterRM_t;
typedef struct PLDevice PLDevice_t;
typedef struct Metadata Metadata_t;
typedef struct Json Json_t;

struct PLDevice{
        char		name[100];
        uint64_t	base;
        uint64_t	size;
	uint8_t		type;
};

//#define NONE		0x00
#define LIB		0x01
#define USER_DEFINED	0x02

struct Fallback{
	char		lib[100];
	int		behaviour;
};

struct InterRM{
	int		compatible;
	uint64_t	address;
};

struct Metadata{
	struct Fallback	fallback;
	struct InterRM	interRM;
	struct PLDevice	plDevices[10];
	char		lib[100];
	char		accel_type[100];
	uint8_t		plDevices_Count;
	uint64_t	Input_Data_Block_Size;
	uint64_t	Output_Data_Block_Size;
	uint8_t		Input_Channel_Count;
	uint8_t		Output_Channel_Count;
	uint64_t	Config_Buffer_Size;
	uint64_t	Input_Buffer_Size;
	uint64_t	Output_Buffer_Size;
	uint64_t	Throughput;
	uint8_t		DMA_type;
	uint8_t		Accel_Handshake_Type;	
};

struct Json{
	int size;
	char* data;
};

extern int json2meta(Json_t* json, Metadata_t *metadata);
extern int printMeta(Metadata_t *metadata);
extern int file2json(char* filename, Json_t *json);

typedef struct AbstractGraph AbstractGraph_t;
typedef struct Element Element_t;

extern int graphParser(char* jsonStr, AbstractGraph_t **graph);
extern int graphIDParser(char* jsonStr);
