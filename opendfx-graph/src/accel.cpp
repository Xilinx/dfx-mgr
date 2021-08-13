// accel.cpp
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include "accel.hpp"
#include "utils.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using opendfx::Accel;


Accel::Accel(const std::string &name, std::string &parentGraphId, int type) : name(name), parentGraphId(parentGraphId), type(type) {
	utils::setID(id, strid);
	linkRefCount = 0;
}

Accel::Accel(const std::string &name, std::string &parentGraphId, int type, const std::string &strid) : name(name), parentGraphId(parentGraphId), type(type), strid(strid) {
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
	return 0;
}

int Accel::subsLinkRefCount(){
	this->linkRefCount --;
	return 0;
}

int Accel::getLinkRefCount(){
	return this->linkRefCount;
}

std::string Accel::toJson(bool withDetail){
	json document;
	document["id"]      = strid;
	if(withDetail){
		document["linkRefCount"] = linkRefCount;
		document["parentGraphId"]    = parentGraphId;
	}
	document["name"]    = name;
	document["type"]    = type;
	std::stringstream jsonStream;
	jsonStream << document.dump(true);

	return jsonStream.str();
}

int Accel::setDeleteFlag(bool deleteFlag){
	this->deleteFlag = deleteFlag;
	return 0;
}

bool Accel::getDeleteFlag() const{
	return this->deleteFlag;
}
