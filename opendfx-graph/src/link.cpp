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

Link::Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir) : accel(accel), buffer(buffer), dir(dir) {
	deleteFlag = false;
	accel->addLinkRefCount();
	buffer->addLinkRefCount();
	//std::cout << accel.toJson(true); 
	utils::setID(id, strid);
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
	}
	document["accel"]    = accel->getId();
	document["buffer"]    = buffer->getId();
	std::stringstream jsonStream;
	jsonStream << document.dump(true);

	return jsonStream.str();
}
