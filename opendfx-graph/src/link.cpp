/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// link.cpp
#include <iostream>
#include <sstream>
#include "link.hpp"
#include "accel.hpp"
#include "buffer.hpp"
#include "utils.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using opendfx::Link;

Link::Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, int parentGraphId) : accel(accel), buffer(buffer), dir(dir), parentGraphId(parentGraphId) {
	deleteFlag = false;
	accel->addLinkRefCount();
	buffer->addLinkRefCount();
	//std::cout << accel.toJson(true); 
	id = utils::genID();
}

Link::Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, int parentGraphId, int id) : accel(accel), buffer(buffer), dir(dir), id(id), parentGraphId(parentGraphId) {
	deleteFlag = false;
	accel->addLinkRefCount();
	buffer->addLinkRefCount();
}

opendfx::Accel* Link::getAccel() const{
	return this->accel;
}

opendfx::Buffer* Link::getBuffer() const{
	return this->buffer;
}

int Link::setDeleteFlag(bool deleteFlag){
	this->deleteFlag = deleteFlag;
	return 0;
}

bool Link::getDeleteFlag() const{
	return this->deleteFlag;
}

int Link::info() {
	std::string resp;
	if(dir == opendfx::direction::toAccel){
		std::cout << utils::int2str(id) << "Link: buffer:" + buffer->getName() + " -> accel:" + accel->getName() << std::endl;
	}
	else{
		std::cout << utils::int2str(id) << "Link: accel:" + accel->getName() + " -> buffer:" + buffer->getName() << std::endl;
	}

	return 0;
}

std::string Link::toJson(bool withDetail){
	json document;
	document["id"]      = utils::int2str(id);
	if(withDetail){
		document["deleteFlag"] = deleteFlag;
		document["parentGraphId"]    = parentGraphId;
	}
	document["accel"]    = utils::int2str(accel->getId());
	document["buffer"]   = utils::int2str(buffer->getId());
	document["dir"]      = dir;
	document["offset"]   = offset;
	document["transactionSize"]   = transactionSize;
	document["transactionIndex"]   = transactionIndex;
	document["channel"]   = channel;
	std::stringstream jsonStream;
	jsonStream << document.dump(true);

	return jsonStream.str();
}
