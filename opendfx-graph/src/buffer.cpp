// buffer.cpp
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include "buffer.hpp"
#include "utils.hpp"

using opendfx::Buffer;

Buffer::Buffer(const std::string &name) : name(name) {
	utils::setID(id, strid);
	linkRefCount = 0;
}
std::string Buffer::info() const {
	return "Buffer name: " + name + "_" + strid;
}
std::string Buffer::getName() const {
	return name;
}

int Buffer::addLinkRefCount(){
	this->linkRefCount ++;
}

int Buffer::subsLinkRefCount(){
	this->linkRefCount --;
}

int Buffer::getLinkRefCount(){
	return this->linkRefCount;
}

std::string Buffer::toJson(bool withDetail){
	std::stringstream jsonStream;
	jsonStream << "{\n";
	jsonStream << "\t\"id\"\t: "   << strid << ",\n";
	if(withDetail){
		jsonStream << "\t\"linkRefCount\"\t: "   << linkRefCount << ",\n";
		jsonStream << "\t\"deleteFlag\"\t: "   << deleteFlag << ",\n";
	}
	jsonStream << "\t\"name\"\t: " << name;
	jsonStream << "}";
	return jsonStream.str();
}

int Buffer::setDeleteFlag(bool deleteFlag){
	this->deleteFlag = deleteFlag;
	return 0;
}

bool Buffer::getDeleteFlag() const{
	return this->deleteFlag;
}
