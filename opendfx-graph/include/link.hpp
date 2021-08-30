/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// link.hpp
#ifndef LINK_HPP_
#define LINK_HPP_

#include <string>
#include "accel.hpp"
#include "buffer.hpp"

namespace opendfx {

	enum direction {toAccel=0, fromAccel=1};

	class Link {

	public:
		explicit Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, int parentGraphId);
		explicit Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, int parentGraphId, int id);
		int info();
		inline bool operator==(const Link& rhs) const {
		    return (this->id == rhs.id);
		}
		opendfx::Accel* getAccel() const;
		opendfx::Buffer* getBuffer() const;
		int setDeleteFlag(bool deleteFlag);
		bool getDeleteFlag() const;
		static inline bool staticGetDeleteFlag(Link *link) {
			return link->getDeleteFlag();
		}
		inline int getId() const { return this->id; }
		inline int getDir() const { return this->dir; }
		std::string toJson(bool withDetail = false);
		inline int getOffset(){return this->offset;}
		inline int setOffset(int offset){
			this->offset = offset;
			return 0;
		}
		inline int getTransactionSize(){
			return this->transactionSize;
		}
		inline int setTransactionSize(int transactionSize){
			this->transactionSize = transactionSize;
			return 0;
		}
		inline int getTransactionSizeScheduled(){
			return this->transactionSizeScheduled;
		}
		inline int setTransactionSizeScheduled(int transactionSizeScheduled){
			this->transactionSizeScheduled = transactionSizeScheduled;
			return 0;
		}
		inline int getTransactionIndex(){
			return this->transactionIndex;
		}
		inline int setTransactionIndex(int transactionIndex){
			this->transactionIndex = transactionIndex;
			return 0;
		}
		inline int getChannel(){return this->channel;}
		inline int setChannel(int channel){
			this->channel = channel;
			return 0;
		}
		inline int setLayerIndex(int layerIndex){
			this->layerIndex = layerIndex;
			return 0;
		}
		inline int getLayerIndex(){
			return this->layerIndex;
		}
		//inline int setAccounted(int accounted){
		//	this->accounted = accounted;
		//	return 0;
		//}
		//inline int getAccounted(){
		//	return this->accounted;
		//}
	private:
		opendfx::Accel *accel;
		opendfx::Buffer *buffer;
		int dir;
		int id;
		int parentGraphId;
		bool deleteFlag;
		int offset;
		int transactionSize;
		int transactionSizeScheduled;
		int transactionIndex;
		int channel;
		int layerIndex;
		//int accounted;
	};
} 
#endif // LINK_HPP_
