/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// accel.cpp
#include <iostream>
#include <dlfcn.h>
#include <iomanip>
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
#include <dfx-mgr/daemon_helper.h>

using json = nlohmann::json;
using opendfx::Accel;


Accel::Accel(const std::string &name, int parentGraphId, int type) : name(name), parentGraphId(parentGraphId), type(type) {
	id = utils::genID();
	linkRefCount = 0;
	semaphore = id ^ parentGraphId; 
	S2MMStatus = 0;
	MM2SStatus = 0;
	S2MMCurrentIndex = 0;
	MM2SCurrentIndex = 0;
	CurrentIndex = 0;
}

Accel::Accel(const std::string &name, int parentGraphId, int type, int id) : name(name), id(id), parentGraphId(parentGraphId), type(type) {
	linkRefCount = 0;
	//id = stoi (strid, 0, 16);
	semaphore = id ^ parentGraphId; 
	S2MMStatus = 0;
	MM2SStatus = 0;
	S2MMCurrentIndex = 0;
	MM2SCurrentIndex = 0;
	CurrentIndex = 0;
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
		dmaLib = "/media/test/dfx-mgr/build/opendfx-graph/drivers/softDma/src/libsoftDma_shared.so";	
        dmDriver = dlopen(dmaLib.c_str(), RTLD_NOW);
        registerDev = (REGISTER) dlsym(dmDriver, "registerDriver");
        unregisterDev = (UNREGISTER) dlsym(dmDriver, "unregisterDriver");
        registerDev(&device, &config);
        config->semptr  = semptr;
        config->ptr     = ptr;
        config->phyAddr = phyAddr;
        device->open(config);
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
		//std::cout << metadata << std::endl;
		json document = json::parse(metadata);
		json accelMetadataObj = document["accel_metadata"];
		std::string dmaType;
		accelMetadataObj.at("DMA_type").get_to(dmaType);
		if (dmaType == "HLS_MULTICHANNEL_DMA"){
			dmaLib = "/media/test/dfx-mgr/build/opendfx-graph/drivers/sihaDma/src/libsihaDma_shared.so";
		}else if(dmaType == "SOFT_DMA"){
			dmaLib = "/media/test/dfx-mgr/build/opendfx-graph/drivers/fallback/src/libfallback_shared.so";
		}
		json fallbackObj = accelMetadataObj["fallback"];
		std::string behaviour;
		std::string fallbackLib;
		fallbackObj.at("Behaviour").get_to(behaviour);
		fallbackObj.at("fallback_Lib").get_to(fallbackLib);
		json interrmObj = accelMetadataObj["Inter_RM"];
		std::string compatible;
		interrmObj.at("Compatible").get_to(compatible);
		if (compatible == "True"){
			InterRMCompatible = 1;
		}
		else if (compatible == "False"){
			InterRMCompatible = 0;
		}
		else{
			InterRMCompatible = 0;
		}
		std::cout << "load accelerator" << std::endl;
		slot = load_accelerator(cname);
		std::cout << "load accelerator" << std::endl;
		if(slot >= 0){
			std::cout << "loading hardware accel" << std::endl;
			dmDriver = dlopen(dmaLib.c_str(), RTLD_NOW);
			registerDev = (REGISTER) dlsym(dmDriver, "registerDriver");
			unregisterDev = (UNREGISTER) dlsym(dmDriver, "unregisterDriver");
			registerDev(&device, &config);
			config->slot = slot;
			device->open(config);
		}
		else if(fallbackLib != "None"){
			std::cout << "loading soft accel" << std::endl;
			dmDriver = dlopen(fallbackLib.c_str(), RTLD_NOW);
			registerDev = (REGISTER) dlsym(dmDriver, "registerDriver");
			unregisterDev = (UNREGISTER) dlsym(dmDriver, "unregisterDriver");
			registerDev(&device, &config);
			device->open(config);
		}
		else {
			return -1;
		}
	}
	return 0;
}

int Accel::deallocateAccelResource(){
	if (type == opendfx::acceltype::accelNode){
		device->close(config);
		unregisterDev(&device, &config);
		dlclose(dmDriver);
		remove_accelerator(slot);			
	}
	return 0;
}
