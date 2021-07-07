#include <iostream>
#include <sstream>
#include <stdlib.h>
#include "utils.hpp"

int utils::setID(int & id, std::string & strid){
	id = rand() % 0x10000;
	std::stringstream stream;
	stream << std::hex << id;
	strid = stream.str();
	return 0;
}
