#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <cstdlib>
#include "utils.hpp"

int utils::setID(int & id, std::string & strid){
	id = rand() % 0xFFFFFFFFFFFFFFFF;
	std::stringstream stream;
	stream << std::hex << id;
	strid = stream.str();
	return 0;
}

int utils::genID(){
	return rand() % 0xFFFFFFFFFFFFFFFF;
}

std::string utils::int2str(int id){
	std::stringstream stream;
	stream << std::hex << id;
	return stream.str();
}

int utils::str2int(std::string strid){
	return stoi (strid, 0, 16);
}
