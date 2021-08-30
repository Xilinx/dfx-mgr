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
			//inline int getLayerIndex(){return this->layerIndex;} 

		private:
			opendfx::Link* link;
			std::vector<opendfx::Link *> dependentLinks;
			//int layerIndex;
	};
} 

#endif // EXECUTION_DEPENDENCY_HPP_
