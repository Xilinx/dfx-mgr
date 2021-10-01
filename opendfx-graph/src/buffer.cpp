/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// buffer.cpp
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <fcntl.h>
#include "buffer.hpp"
//#include "device.h"
#include "utils.hpp"
#include "nlohmann/json.hpp"
#include <dfx-mgr/sys/linux/graph/layer0/xrtbuffer.h>

using json = nlohmann::json;
using opendfx::Buffer;

Buffer::Buffer(const std::string &name, int parentGraphId) : name(name), parentGraphId(parentGraphId)  {
	id = utils::genID();
	linkRefCount = 0;
	semaphore = id ^ parentGraphId; 
	config = (BuffConfig_t*) malloc(sizeof(BuffConfig_t));
	status = opendfx::bufferStatus::BuffIsEmpty;
	interRMReqIn = 0;
	interRMReqOut = 0;
}

Buffer::Buffer(const std::string &name, int parentGraphId, int id) : name(name), id(id), parentGraphId(parentGraphId) {
	linkRefCount = 0;
	semaphore = id ^ parentGraphId; 
	config = (BuffConfig_t*) malloc(sizeof(BuffConfig_t));
	status = opendfx::bufferStatus::BuffIsEmpty;
	interRMReqIn = 0;
	interRMReqOut = 0;
}

std::string Buffer::info() const {
	return "Buffer name: " + name + "_" + utils::int2str(id);
}

std::string Buffer::getName() const {
	return name;
}

int Buffer::addLinkRefCount(){
	this->linkRefCount ++;
	return 0;
}

int Buffer::subsLinkRefCount(){
	this->linkRefCount --;
	return 0;
}

int Buffer::getLinkRefCount(){
	return this->linkRefCount;
}

std::string Buffer::toJson(bool withDetail){
	json document;
	document["id"]      = utils::int2str(id);
	if(withDetail){
		document["linkRefCount"] = linkRefCount;
		document["deleteFlag"] = deleteFlag;
		document["parentGraphId"]    = parentGraphId;
	}
	document["name"]    = name;
	document["bSize"]    = bSize;
	document["bType"]    = bType;
	std::stringstream jsonStream;
	jsonStream << document.dump(true);

	return jsonStream.str();
}

int Buffer::setDeleteFlag(bool deleteFlag){
	this->deleteFlag = deleteFlag;
	return 0;
}

bool Buffer::getDeleteFlag() const{
	return this->deleteFlag;
}

int Buffer::allocateBuffer(int xrt_fd){
	int status;
	std::cout << bType << std::endl;
	if (interRMReqIn == 1 && interRMReqOut == 1 && bType == buffertype::STREAM_BASED){
		interRMEnabled = 1;
		config->interRMSinks[0] = 0x20180000000;
		config->interRMSinks[1] = 0x201C0000000;
		config->interRMSinks[2] = 0x20200000000;
		config->sinkSlot = sinkSlot;
		config->sourceSlot = sourceSlot;
		config->interRMEnabled = 1;
		std::cout << "$$$$$" << config->sinkSlot << "@" << config->sourceSlot << std::endl; 
	}
	else{
		interRMEnabled = 0;

		status = xrt_allocateBuffer(xrt_fd, bSize, &handle,
				&ptr, &phyAddr, &fd);
		std::cout << "allocate : " << phyAddr << std::endl; 
		std::cout << "allocateBuffer of size : " << bSize << std::endl;
		if(status < 0){
			printf( "error @ config allocation\n");
			return status;
		}
		config->ptr = ptr;   // Buffer Ptr
    	config->phyAddr = phyAddr; // Buffer Physical Address
		config->interRMEnabled = 0;
	}
	std::cout << "allocating inter RM " << interRMEnabled << interRMReqIn << interRMReqOut << std::endl;
	char SemaphoreName[100];
	memset(SemaphoreName, '\0', 100);
	sprintf(SemaphoreName, "%x", semaphore);
	semptr = sem_open(SemaphoreName, 
			O_CREAT,       
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,   
			0);
	if (semptr == ((void*) -1)){ 
		std::cout << "sem_open" << std::endl;
	}
    config->semptr = semptr;
	return 0;
}

int Buffer::deallocateBuffer(int xrt_fd){
	std::cout << "deallocateBuffer : " << phyAddr << std::endl;
	std::cout << "deallocateBuffer of size : " << bSize << std::endl;
	int status;
	if (interRMEnabled == 0){
		if (ptr){
			if (bType == opendfx::buffertype::DDR_BASED){
				status = xrt_deallocateBuffer(xrt_fd, bSize, &handle, &ptr, &phyAddr);
				if(status < 0){
					printf( "error @ deallocateBuffer\n");
					return status;
				}
			}
		}
	}
	char SemaphoreName[100];
	memset(SemaphoreName, '\0', 100);
	sprintf(SemaphoreName, "%x", semaphore);
	sem_close(semptr);
	sem_unlink(SemaphoreName);
	semptr = NULL;
	ptr = NULL;
	return 0;
}
