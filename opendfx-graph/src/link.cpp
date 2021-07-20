// link.cpp
#include <iostream>
#include <sstream>
#include "link.hpp"
#include "accel.hpp"
#include "buffer.hpp"
#include "utils.hpp"

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
	std::stringstream jsonStream;
	jsonStream << "{\n";
	jsonStream << "\t\"id\"\t: "    << strid          << ",\n";
	if(withDetail){
		jsonStream << "\t\"deleteFlag\"\t: "    << deleteFlag          << ",\n";
	}
	jsonStream << "\t\"accel\"\t: " << accel->getId()  << ",\n";
	jsonStream << "\t\"buffer\"\t: "<< buffer->getId(); 
	jsonStream << "}";
	return jsonStream.str();
}
