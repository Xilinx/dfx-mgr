/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// accel.cpp
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <fcntl.h>
//#include <sys/socket.h>
#include "accel.hpp"
#include "utils.hpp"
#include "nlohmann/json.hpp"
#include <dfx-mgr/sys/linux/graph/layer0/xrtbuffer.h>
#include <dfx-mgr/accel.h>

using json = nlohmann::json;
using opendfx::Accel;


Accel::Accel(const std::string &name, int parentGraphId, int type) : name(name), parentGraphId(parentGraphId), type(type) {
	id = utils::genID();
	linkRefCount = 0;
	semaphore = id ^ parentGraphId; 
}

Accel::Accel(const std::string &name, int parentGraphId, int type, int id) : name(name), id(id), parentGraphId(parentGraphId), type(type) {
	linkRefCount = 0;
	//id = stoi (strid, 0, 16);
	semaphore = id ^ parentGraphId; 
}

std::string Accel::info() const {
	return "Accel name: " + name + "_" + utils::int2str(id);
}

std::string Accel::getName() const {
	return name;
}

int Accel::addLinkRefCount(){
	this->linkRefCount ++;
	return 0;
}

int Accel::subsLinkRefCount(){
	this->linkRefCount --;
	return 0;
}

int Accel::getLinkRefCount(){
	return this->linkRefCount;
}

std::string Accel::toJson(bool withDetail){
	json document;
	document["id"]      = utils::int2str(id);
	if(withDetail){
		document["linkRefCount"] = linkRefCount;
		document["parentGraphId"]    = parentGraphId;
	}
	document["name"]    = name;
	document["type"]    = type;
	document["bSize"]    = bSize;
	std::stringstream jsonStream;
	jsonStream << document.dump(true);

	return jsonStream.str();
}

int Accel::setDeleteFlag(bool deleteFlag){
	this->deleteFlag = deleteFlag;
	return 0;
}

bool Accel::getDeleteFlag() const{
	return this->deleteFlag;
}

int Accel::allocateBuffer(int xrt_fd){
	int status;
	if (type == opendfx::acceltype::inputNode || type == opendfx::acceltype::outputNode){
		status = xrt_allocateBuffer(xrt_fd, bSize, &handle,
				&ptr, &phyAddr, &fd);
		if(status < 0){
			printf( "error @ config allocation\n");
			return status;
		}
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
	} 
	return 0;
}

int Accel::deallocateBuffer(int xrt_fd){
	int status;
	if (type == opendfx::acceltype::inputNode || type == opendfx::acceltype::outputNode){
		status = xrt_deallocateBuffer(xrt_fd, bSize, &handle, &ptr, &phyAddr);
		if(status < 0){
			printf( "error @ deallocateBuffer\n");
			return status;
		}
		char SemaphoreName[100];
		memset(SemaphoreName, '\0', 100);
		sprintf(SemaphoreName, "%x", semaphore);
		sem_close(semptr);
        sem_unlink(SemaphoreName);
        semptr = NULL;
	}
	return 0;
}

int Accel::allocateAccelResource(){
	//int status;
	if (type == opendfx::acceltype::accelNode){
		char *cname = new char[name.length() + 1];
		strcpy(cname, name.c_str());
		std::string metadata(getAccelMetadata(cname));
		std::cout << metadata << std::endl;
	}
	/*if (type == opendfx::acceltype::inputNode || type == opendfx::acceltype::outputNode){
	  status = xrt_allocateBuffer(xrt_fd, bSize, &handle,
	  &ptr, &phyAddr, &fd);
	  if(status < 0){
	  printf( "error @ config allocation\n");
	  return status;
	  }
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
	  } */
	return 0;
}

int Accel::deallocateAccelResource(){
	return 0;
}
