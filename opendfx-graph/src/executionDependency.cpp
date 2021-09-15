/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// executionDependency.cpp

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <executionDependency.hpp>
/*#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include <sys/socket.h>

//#include <string.h>
//#include <signal.h>
//#include <stdbool.h>
//#include <stdint.h>
//#include <sys/select.h>
//#include <sys/types.h>
//#include <sys/un.h>
//#include <fcntl.h>
//#include <errno.h>
#include <unistd.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
//#include <sys/stat.h>

#include "graph.hpp"
#include "accel.hpp"
#include "buffer.hpp"
#include "link.hpp"
#include "nlohmann/json.hpp"
#include "utils.hpp"
#include <dfx-mgr/dfxmgr_client.h>
*/

using opendfx::ExecutionDependency;

ExecutionDependency::ExecutionDependency(opendfx::Link *link) : link(link){};

int ExecutionDependency::addDependency(opendfx::Link *dependentLink){
	dependentLinks.push_back(dependentLink);	
	return 0;
}
			
std::string ExecutionDependency::getInfo(){
	std::stringstream stream;
	stream << "link: " << std::hex << link->getId() << std::endl;
	stream << "dependentLinks: ";
	for (std::vector<opendfx::Link   *>::iterator it = dependentLinks.begin()  ; it != dependentLinks.end()  ; ++it)
	{
		stream << std::hex << (*it)->getId() << ", ";
	}
	stream << std::endl;
	return stream.str();
}


using opendfx::Schedule;

Schedule::Schedule(opendfx::ExecutionDependency *eDependency, int index, int size, int offset, int last, int first):eDependency(eDependency), index(index), size(size), offset(offset), last(last), first(first) {
	deleteFlag = false;
};

int Schedule::getInfo(){
	std::cout << "@@@@@@@@@@@@@@@@@@@" << std::endl;
	std::cout << eDependency->getInfo() << " : " << index << " : " << eDependency->getLink()->getTransactionSizeScheduled() << " : " << size << " : " << offset << " : " << last << " : " << first << std::endl;
	return 0;
}

int Schedule::printCurrentStatus(){
	ExecutionDependency *eDependency = getEDependency();
	opendfx::Link* link = eDependency->getLink();
	opendfx::Accel* accel = link->getAccel();
	opendfx::Buffer* buffer = link->getBuffer();
	std::cout << getDeleteFlag() << " " << index << " : " << accel->getCurrentIndex();
	if (link->getDir() == opendfx::direction::fromAccel){
		std::cout << " [S2MM] ";
	}
	else{
		std::cout << " [MM2S] ";
	}
	std::cout << accel->getMM2SCurrentIndex() << " MM2S : " << accel->getMM2SStatus() << " : ";
	std::cout << accel->getS2MMCurrentIndex() << " S2MM : " << accel->getS2MMStatus() << " : ";
	std::cout << "Buffer : " << buffer->getStatus() << " Transaction"<< std::endl;
	return 0;
}
