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

Link::Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, std::string parentGraphId) : accel(accel), buffer(buffer), dir(dir), parentGraphId(parentGraphId) {
	deleteFlag = false;
	accel->addLinkRefCount();
	buffer->addLinkRefCount();
	//std::cout << accel.toJson(true); 
	utils::setID(id, strid);
}

Link::Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, std::string parentGraphId, const std::string &strid) : accel(accel), buffer(buffer), dir(dir), parentGraphId(parentGraphId), strid(strid) {
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
		std::cout << strid << "Link: buffer:" + buffer->getName() + " -> accel:" + accel->getName() << std::endl;
	}
	else{
		std::cout << strid << "Link: accel:" + accel->getName() + " -> buffer:" + buffer->getName() << std::endl;
	}

	return 0;
}

std::string Link::toJson(bool withDetail){
	json document;
	document["id"]      = strid;
	if(withDetail){
		document["deleteFlag"] = deleteFlag;
		document["parentGraphId"]    = parentGraphId;
	}
	document["accel"]    = accel->getId();
	document["buffer"]   = buffer->getId();
	document["dir"]      = dir;
	document["offset"]   = offset;
	document["transactionSize"]   = transactionSize;
	document["transactionIndex"]   = transactionIndex;
	document["channel"]   = channel;
	std::stringstream jsonStream;
	jsonStream << document.dump(true);

	return jsonStream.str();
}
