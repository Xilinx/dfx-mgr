/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// executionDependency.hpp
#ifndef EXECUTION_DEPENDENCY_HPP_
#define EXECUTION_DEPENDENCY_HPP_

#include <link.hpp>
#include <string>
/*#include <string>
#include <vector>
#include <mutex>
#include <accel.hpp>
#include <buffer.hpp>
#include <link.hpp>
#include <sys/socket.h>
#include <dfx-mgr/dfxmgr_client.h>*/


namespace opendfx {

	class ExecutionDependency {

		public:
			explicit ExecutionDependency(opendfx::Link *link); //, int layerIndex);
			int addDependency(opendfx::Link *dependentLink);
			std::string getInfo();
			inline opendfx::Link* getLink(){return this->link;} 
			inline bool isDependent(){
				std::cout <<  this->dependentLinks.size() << std::endl;
				return this->dependentLinks.size() > 0;} 

		private:
			opendfx::Link* link;
			std::vector<opendfx::Link *> dependentLinks;
			//int layerIndex;
	};

	class Schedule {
		public:
			explicit Schedule(opendfx::ExecutionDependency *eDependency, int index, int size, int offset, int last, int first);
			inline int getIndex(){return this->index;}
			inline int getSize(){return this->size;}
			inline int getOffset(){return this->offset;}
			inline int getStatus(){return this->status;}
			inline int getLast(){return this->last;}
			inline int getFirst(){return this->first;}
			int getInfo();
			inline opendfx::ExecutionDependency* getEDependency(){return this->eDependency;} 
		private:
			ExecutionDependency *eDependency;
			int index;
			int size;
			int offset;
			int status;
			int last;
			int first;
	};
} 

#endif // EXECUTION_DEPENDENCY_HPP_
