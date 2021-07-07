// accel.cpp
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include "accel.hpp"
#include "utils.hpp"

using opendfx::Accel;

Accel::Accel(const std::string &name) : name(name) {
	utils::setID(id, strid);
	linkRefCount = 0;
}

std::string Accel::info() const {
	return "Accel name: " + name + "_" + strid;
}

std::string Accel::getName() const {
	return name;
}

int Accel::addLinkRefCount(){
	this->linkRefCount ++;
}

int Accel::subsLinkRefCount(){
	this->linkRefCount --;
}

int Accel::getLinkRefCount(){
	return this->linkRefCount;
}

std::string Accel::toJson(bool withDetail){
	std::stringstream jsonStream;
	jsonStream << "{\n";
	jsonStream << "\t\"id\"\t: "   << strid << ",\n";
	if(withDetail){
		jsonStream << "\t\"linkRefCount\"\t: "   << linkRefCount << ",\n";
	}
	jsonStream << "\t\"name\"\t: " << name;
	jsonStream << "}";
	return jsonStream.str();
}

int Accel::setDeleteFlag(bool deleteFlag){
	this->deleteFlag = deleteFlag;
	return 0;
}

bool Accel::getDeleteFlag() const{
	return this->deleteFlag;
}
