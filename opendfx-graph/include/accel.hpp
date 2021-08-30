/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// accel.hpp
#ifndef ACCEL_HPP_
#define ACCEL_HPP_

#include <string>
#include <semaphore.h>
#include "device.h"

namespace opendfx {

	enum acceltype {accelNode=0, inputNode=1, outputNode=2, hwAccelNode=3, softAccelNode=4, aiAccelNode=5};

	class Accel {
		public:
			explicit Accel(const std::string &name, int parentGraphId, int type = opendfx::acceltype::accelNode);
			explicit Accel(const std::string &name, int parentGraphId, int type, const int id);
			std::string info() const;
			std::string getName() const;
			int addLinkRefCount();
			int subsLinkRefCount();
			int getLinkRefCount();
			inline int getParentGraphId(){
				return this->parentGraphId;
			}
			inline int setBSize(int bSize){
				this->bSize = bSize;
				return 0;
			}
			inline bool operator==(const Accel& rhs) const {
				return (this->id == rhs.id);
			}
			int setDeleteFlag(bool deleteFlag);
			bool getDeleteFlag() const;
			static inline bool staticGetDeleteFlag(Accel *accel) {
				return accel->getDeleteFlag();
			}
			inline int getId() const { return this->id; }
			std::string toJson(bool withDetail = false);
			inline int getType(){
				return type;
			};
			inline int setType(int type){
				this->type = type;
				return 0;
			};
			int allocateBuffer(int xrt_fd);
			int deallocateBuffer(int xrt_fd);
			int allocateAccelResource();
			int deallocateAccelResource();
			inline Device_t* getDevice() const { return this->device; }
			inline DeviceConfig_t* getConfig() const { return this->config; }
			inline int getS2MMStatus() const { return this->S2MMStatus; }
			inline int setS2MMStatus(int S2MMStatus){
				this->S2MMStatus = S2MMStatus;
				return 0;
			};
			inline int getMM2SStatus() const { return this->MM2SStatus; }
			inline int setMM2SStatus(int MM2SStatus){
				this->MM2SStatus = MM2SStatus;
				return 0;
			};
		private:
			std::string name;
			int id;
			int gid;
			int linkRefCount;
			int parentGraphId;
			int type;
			bool deleteFlag;
			int bSize;
			std::string dmaLib;
			std::string fallbackLib;
			int InterRMCompatible;
			Device_t* device;
    		DeviceConfig_t *config;
    		REGISTER registerDev;
   			UNREGISTER unregisterDev;
			void *dmDriver;

			int fd;     // File descriptor
		    int handle; // Buffer XRT Handeler
		    uint8_t* ptr;   // Buffer Ptr
		    unsigned long phyAddr; // Buffer Physical Address
		    int semaphore;
		    sem_t* semptr;
			bool staged;
		
			int slot;
			int S2MMStatus;
			int MM2SStatus;
	};

	//inline bool Accel::operator==(const Accel* lhs, const Accel* rhs) {
	//    return (*lhs == *rhs);
	//    //return (lhs->id == rhs->id && this->strid == rhs->strid);
	//}
} // #end of wrapper

#endif // ACCEL_HPP_
