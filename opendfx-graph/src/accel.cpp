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
//#include <dfx-mgr/model.h>
#include <sys/mman.h>

using json = nlohmann::json;
using opendfx::Accel;

typedef struct xrt_device_info {
	char xclbin[100];
	uint8_t xrt_device_id;
	xrtDeviceHandle device_hdl;
	xuid_t xrt_uid;
} xrt_device_info_t;

Accel::Accel(const std::string &name, int parentGraphId, int type) : name(name), parentGraphId(parentGraphId), type(type) {
	id = utils::genID();
	linkRefCount = 0;
	semaphore = id ^ parentGraphId; 
	S2MMStatus = 0;
	MM2SStatus = 0;
	S2MMCurrentIndex = 0;
	MM2SCurrentIndex = 0;
	CurrentIndex = 0;
	slot = -1;
	ptr = NULL;
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
	slot = -1;
	ptr = NULL;
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
		document["deleteFlag"]    = deleteFlag;
	}
	document["name"]    = name;
	document["type"]    = type;
	document["bSize"]    = bSize;
	document["slot"]    = slot;
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
		//std::cout << xrt_fd << fd << " " << bSize << " " << status << std::endl;
		if(status < 0){
			printf( "error @ config allocation\n");
			return status;
		}
		std::cout << "allocateIOBuffer of size : " << bSize << std::endl;
		char SemaphoreName[100];
		memset(SemaphoreName, '\0', 100);
		sprintf(SemaphoreName, "%x", semaphore);
		//std::cout << "Semaphore : " << std::hex << semaphore << std::endl;
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

int Accel::reAllocateBuffer(){
	//std::cout << " reAllocateBuffer" << std::endl;
	char SemaphoreName[100];
	if (type == opendfx::acceltype::inputNode || type == opendfx::acceltype::outputNode){
		mapBuffer(fd, bSize, &ptr);
		//std::cout << fd << " " << bSize << " " << status << std::endl;
		memset(SemaphoreName, '\0', 100);
		sprintf(SemaphoreName, "%x", semaphore);
		//std::cout << "Semaphore : " << std::hex << semaphore << std::endl;
		semptr = sem_open(SemaphoreName, 
				O_CREAT,       
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,   
				0);
		if (semptr == ((void*) -1)){ 
			std::cout << "sem_open" << std::endl;
		}
		//this->post();
	} 
	return 0;
}

int Accel::deallocateBuffer(int xrt_fd){
	int status;
	if (type == opendfx::acceltype::inputNode || type == opendfx::acceltype::outputNode){
		std::cout << "deallocateIOBuffer of size : " << bSize << std::endl;
		if (ptr){
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
			ptr = NULL;
		}
	}
	return 0;
}

int Accel::reDeallocateBuffer(){
	if (type == opendfx::acceltype::inputNode || type == opendfx::acceltype::outputNode){
		if (ptr){
			std::cout << "reDeallocateIOBuffer of size : " << bSize << std::endl;
			unmapBuffer(fd, bSize, &ptr);
			//munmap(&ptr, bSize);
			//status = xrt_deallocateBuffer(xrt_fd, bSize, &handle, &ptr, &phyAddr);
			//if(status < 0){
			//	printf( "error @ deallocateBuffer\n");
			//	return status;
			//}
			char SemaphoreName[100];
			memset(SemaphoreName, '\0', 100);
			sprintf(SemaphoreName, "%x", semaphore);
			sem_close(semptr);
			sem_unlink(SemaphoreName);
			semptr = NULL;
			ptr = NULL;
		}
	}
	return 0;
}

int Accel::allocateAccelResource(){
	//int status;
	if (type == opendfx::acceltype::accelNode){
		char *cname = new char[name.length() + 1];
		strcpy(cname, name.c_str());
		std::string metadata;
		for(int i=0; i < 4; i++){
			void * metadataPtr = getAccelMetadata(cname, i);
			if(metadataPtr != NULL){
				metadata = (char *)metadataPtr;
				break;
			}
		}
		std::cout << metadata << std::endl;
		std::cout << "#################################" << std::endl;

		json document = json::parse(metadata);
		json accelMetadataObj = document["accel_metadata"];
		std::string dmaType;
		accelMetadataObj.at("DMA_type").get_to(dmaType);
		std::cout << "dmaType : " << dmaType << std::endl;
		std::cout << "loading AIE accel" << std::endl;
		accelMetadataObj.at("dma_driver_lib").get_to(dmaLib);
		//
		//}
		//	if (dmaType == "HLS_MULTICHANNEL_DMA"){
		//		dmaLib = "/media/test/dfx-mgr/build/opendfx-graph/drivers/sihaDma/src/libsihaDma_shared.so";
		//	}else if(dmaType == "SOFT_DMA"){
		//		dmaLib = "/media/test/dfx-mgr/build/opendfx-graph/drivers/fallback/src/libfallback_shared.so";
		//	}
		json fallbackObj = accelMetadataObj["fallback"];
		std::string behaviour;
		std::string fallbackLib;
		fallbackObj.at("Behaviour").get_to(behaviour);
		fallbackObj.at("fallback_Lib").get_to(fallbackLib);
		json interrmObj = accelMetadataObj["Inter_RM"];
		std::string compatible;
		interrmObj.at("Compatible").get_to(compatible);
		if (compatible == "True"){
			interRMCompatible = 1;
		}
		else {
			interRMCompatible = 0;
		}
		//std::cout << "load accelerator" << std::endl;
		slot = load_accelerator(cname);
		//std::cout << "load accelerator" << std::endl;
		if(slot >= 0){
			std::cout << "loading driver from path : " << dmaLib << std::endl;
			dmDriver = dlopen(dmaLib.c_str(), RTLD_NOW);
			registerDev = (REGISTER) dlsym(dmDriver, "registerDriver");
			unregisterDev = (UNREGISTER) dlsym(dmDriver, "unregisterDriver");
			registerDev(&device, &config);
			xrt_device_info_t *aie = (xrt_device_info_t *)getXRTinfo(slot);
			config->slot = slot;
			config->xclbin = aie->xclbin;
			config->xrt_device_id = &aie->xrt_device_id;
			config->device_hdl = &aie->device_hdl;
			config->xrt_uid = &aie->xrt_uid;
			device->open(config);
		}
		else if(fallbackLib != "None"){
			std::cout << "loading soft accel" << std::endl;
			dmDriver = dlopen(fallbackLib.c_str(), RTLD_NOW);
			registerDev = (REGISTER) dlsym(dmDriver, "registerDriver");
			unregisterDev = (UNREGISTER) dlsym(dmDriver, "unregisterDriver");
			registerDev(&device, &config);
			device->open(config);
			interRMCompatible = 0;
		}
		else {
			return -1;
		}
}
return 0;
}

int Accel::deallocateAccelResource(){
	if (type == opendfx::acceltype::accelNode){
		if (slot >=0){
			device->close(config);
			unregisterDev(&device, &config);
			dlclose(dmDriver);
			remove_accelerator(slot);			
		}
	}
	return 0;
}

int Accel::post(){
	return sem_post(semptr);
}

int Accel::wait(){
	return sem_wait(semptr);
}

int Accel::trywait(){
	return sem_trywait(semptr);
}
