/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// buffer.hpp
#ifndef BUFFER_HPP_
#define BUFFER_HPP_

#include <string>
#include <semaphore.h>
#include "device.h"

namespace opendfx {

	enum buffertype {DDR_BASED=0, STREAM_BASED=1};
	enum bufferStatus {BuffIsEmpty=0, BuffIsFull=1, BuffIsBusy=2, BuffIsStream=3};

class Buffer {

	public:
		explicit Buffer(const std::string &name, int parentGraphId);
		explicit Buffer(const std::string &name, int parentGraphId, int id);
		std::string info() const;
		std::string getName() const;
		int addLinkRefCount();
		int subsLinkRefCount();
		int getLinkRefCount();
		inline bool operator==(const Buffer& rhs) const {
			return (this->id == rhs.id);
		}
		int setDeleteFlag(bool deleteFlag);
		bool getDeleteFlag() const;
		static inline bool staticGetDeleteFlag(Buffer *buffer) {
			return buffer->getDeleteFlag();
		}
		inline int getId() const { return this->id; }
		std::string toJson(bool withDetail = false);
		inline int setBSize(int bSize){
			this->bSize = bSize;
			return 0;
		}
		inline int getBSize(){ return this->bSize;}
		inline int getStatus(){ return this->status;}
		inline int setStatus(int status){ 
			this->status = status; 
			return 0;
		}
		inline BuffConfig_t* getConfig(){ return this->config;}
		inline int setBType(int bType){
			this->bType = bType;
			return 0;
		}
		int allocateBuffer(int xrt_fd);
		int deallocateBuffer(int xrt_fd);
		inline int setInterRMReqIn(int interRMReqIn){ 
			this->interRMReqIn += interRMReqIn; 
			return 0;
		}
		inline int setInterRMReqOut(int interRMReqOut){ 
			this->interRMReqOut += interRMReqOut; 
			return 0;
		}
		inline int getInterRMEnabled(){ return this->interRMEnabled;}
		inline int setSinkSlot(int sinkSlot){ 
			this->sinkSlot += sinkSlot; 
			return 0;
		}
		inline int getSinkSlot(){ return this->sinkSlot;}
		inline int setSourceSlot(int sourceSlot){ 
			this->sourceSlot += sourceSlot; 
			return 0;
		}
	private:
		std::string name;
		int id;
		int parentGraphId;
		int linkRefCount;
		bool deleteFlag;
		int bSize;
		int bType;

		int fd;     // File descriptor
		int handle; // Buffer XRT Handeler
		uint8_t* ptr;   // Buffer Ptr
		unsigned long phyAddr; // Buffer Physical Address
		int semaphore;
		sem_t* semptr;
		BuffConfig_t *config;
		int interRMReqIn;
		int interRMReqOut;
		int interRMEnabled;
		int sinkSlot;
		int sourceSlot;
		//bool empty;
		//bool full;
		int status;
};
} // #end of wrapper
#endif // BUFFER_HPP_
