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
		inline BuffConfig_t* getConfig(){ return this->config;}
		inline int setBType(int bType){
			this->bType = bType;
			return 0;
		}
		int allocateBuffer(int xrt_fd);
		int deallocateBuffer(int xrt_fd);
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
};
} // #end of wrapper
#endif // BUFFER_HPP_
