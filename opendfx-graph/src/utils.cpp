#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <cstdlib>
#include <iomanip>
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

int utils::printBuffer(int *data, int size){
	for(int i = 0; i < size; i++){
		if (i%8 == 0){
			std::cout << std::endl;
		}
		std::cout << " " << std::setfill('0') << std::setw(16) << std::hex << data[i];
	}
	std::cout << std::endl;
	return 0;
}

int utils::printBuffer(uint32_t *data, int size){
	for(int i = 0; i < size; i++){
		if (i%8 == 0){
			std::cout << std::endl;
		}
		std::cout << " " << std::setfill('0') << std::setw(8) << std::hex << data[i];
	}
	std::cout << std::endl;
	return 0;
}
