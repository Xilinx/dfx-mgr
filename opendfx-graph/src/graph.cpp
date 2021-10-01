/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
// wrapper.cpp
#include <iostream>
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

//#include "device.h"
#include "graph.hpp"
#include "accel.hpp"
#include "buffer.hpp"
#include "link.hpp"
#include "nlohmann/json.hpp"
#include "utils.hpp"
#include <dfx-mgr/dfxmgr_client.h>

using json = nlohmann::json;
using opendfx::Graph;

Graph::Graph(const std::string &name, int priority, bool bypass) : m_name(name), bypass(bypass) {
	int i;
	//char * block;
	//short size = 1;
	std::ifstream urandom("/dev/urandom", std::ios::in|std::ios::binary); // Seed initialisation based on /dev/urandom
	urandom.read((char*)&i, sizeof(i));
	srand(i ^ time(0));
	urandom.close();

	id = utils::genID();
	if (priority < 3 && priority >= 0){
		this->priority = priority;
	}
	else{
		this->priority = 0;
	}
	accelCount = 0;
	status = opendfx::graphStatus::GraphIdle;
	abstract = true;
}

Graph::~Graph(){
	//if (abstract){
			//redeallocateIOBuffers();
	//}
	//else{
		//deallocateIOBuffers();
		//deallocateBuffers();
		//deallocateAccelResources();
	//}
}


std::string Graph::info() const {
	return "Graph ID: " + utils::int2str(id);
}


std::string Graph::getInfo() const {
	std::stringstream stream;
	stream << "{\"name\": \"" << m_name << "_" << utils::int2str(id) << "\", \"priority\": " << priority << "}";
	return stream.str();
}

opendfx::Accel* Graph::addAccel(const std::string &name){
	opendfx::Accel *accel = new opendfx::Accel(name, id);
	accel->setBSize(0);
	accels.push_back(accel);
	accelCount ++;
	return accel;
}

opendfx::Accel* Graph::addInputNode(const std::string &name, int bSize){
	opendfx::Accel *accel = new opendfx::Accel(name, id, opendfx::acceltype::inputNode);
	accel->setBSize(bSize);
	accels.push_back(accel);
	return accel;
}

opendfx::Accel* Graph::addOutputNode(const std::string &name, int bSize){
	opendfx::Accel *accel = new opendfx::Accel(name, id, opendfx::acceltype::outputNode);
	accel->setBSize(bSize);
	accels.push_back(accel);
	return accel;
}

opendfx::Accel* Graph::addAccel(opendfx::Accel *accel){
	accels.push_back(accel);
	if (accel->getType() == opendfx::acceltype::accelNode){
		accelCount ++;
	}
	return accel;
}

opendfx::Buffer* Graph::addBuffer(const std::string &name, int bSize, int bType){
	opendfx::Buffer *buffer= new opendfx::Buffer(name, id);
	buffer->setBSize(bSize);
	buffer->setBType(bType);
	buffers.push_back(buffer);
	return buffer;
}

opendfx::Buffer* Graph::addBuffer(opendfx::Buffer *buffer){
	buffers.push_back(buffer);
	return buffer;
}

opendfx::Link* Graph::connectInputBuffer(opendfx::Accel *accel, opendfx::Buffer *buffer,
		int offset, int transactionSize, int transactionIndex, int channel){
	opendfx::Link *link = new opendfx::Link(accel, buffer, opendfx::direction::toAccel, id);
	link->setOffset(offset);
	link->setTransactionSize(transactionSize);
	link->setTransactionIndex(transactionIndex);
	link->setChannel(channel);
	links.push_back(link);
	return link;
}

opendfx::Link* Graph::connectOutputBuffer(opendfx::Accel *accel, opendfx::Buffer *buffer,
		int offset, int transactionSize, int transactionIndex, int channel){
	opendfx::Link *link = new opendfx::Link(accel, buffer, opendfx::direction::fromAccel, id);
	link->setOffset(offset);
	link->setTransactionSize(transactionSize);
	link->setTransactionIndex(transactionIndex);
	link->setChannel(channel);
	links.push_back(link);
	return link;
}

opendfx::Link* Graph::addLink(opendfx::Link *link){
	links.push_back(link);
	return link;
}

int Graph::delAccel(opendfx::Accel *accel){
	for (std::vector<opendfx::Link *>::iterator it = links.begin() ; it != links.end(); ++it)
	{
		if ((*it)->getAccel() == accel){
			if (accel->getType() == 0){
				accelCount --;
			}
			(*it)->setDeleteFlag(true);
		}
	}
	accel->setDeleteFlag(true);
	links.erase(std::remove_if(links.begin(), links.end(), Link::staticGetDeleteFlag), links.end());
	accels.erase(std::remove_if(accels.begin(), accels.end(), Accel::staticGetDeleteFlag), accels.end());
	return 0;
}

int Graph::delBuffer(opendfx::Buffer *buffer){
	for (std::vector<opendfx::Link *>::iterator it = links.begin() ; it != links.end(); ++it)
	{
		if ((*it)->getBuffer() == buffer){
			(*it)->setDeleteFlag(true);
		}
	}
	buffer->setDeleteFlag(true);
	links.erase(std::remove_if(links.begin(), links.end(), Link::staticGetDeleteFlag), links.end());
	buffers.erase(std::remove_if(buffers.begin(), buffers.end(), Buffer::staticGetDeleteFlag), buffers.end());
	return 0;
}

int Graph::delLink(opendfx::Link *link){
	link->setDeleteFlag(true);
	links.erase(std::remove_if(links.begin(), links.end(), Link::staticGetDeleteFlag), links.end());
	return 0;
}

int Graph::countAccel(){
	return accelCount;
}

int Graph::countBuffer(){
	return buffers.size();
}

int Graph::countLink(){
	return links.size();
}

opendfx::Accel * Graph::getAccelByID(int id)
{
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		opendfx::Accel* accel = *it;
		if (accel->getId() == id){
			return accel;
		}
	}
	return NULL;
}

opendfx::Buffer * Graph::getBufferByID(int id)
{
	for (std::vector<opendfx::Buffer *>::iterator it = buffers.begin() ; it != buffers.end(); ++it)
	{
		opendfx::Buffer* buffer = *it;
		if (buffer->getId() == id){
			//std::cout << buffer->getId() << " : " << strid << std::endl;
			return buffer;
		}
	}
	return NULL;
}

int Graph::listAccels()
{
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		opendfx::Accel* accel = *it;
		std::cout << ' ' << accel->info() << '\n';
	}
	std::cout << '\n';
	return 0;
}

int Graph::listBuffers()
{
	for (std::vector<opendfx::Buffer *>::iterator it = buffers.begin() ; it != buffers.end(); ++it)
	{
		opendfx::Buffer* buffer = *it;
		std::cout << ' ' << buffer->info() << '\n';
	}
	std::cout << '\n';
	return 0;
}

int Graph::listLinks()
{
	for (std::vector<opendfx::Link *>::iterator it = links.begin() ; it != links.end(); ++it)
	{
		opendfx::Link* link = *it;
		link->info();
	}
	std::cout << '\n';
	return 0;
}

std::string Graph::jsonAccels(bool withDetail)
{
	json document;
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		document.push_back(json::parse((*it)->toJson(withDetail)));
	}
	std::stringstream jsonStream;
	jsonStream << document.dump(true);
	return jsonStream.str();
}

std::string Graph::jsonBuffers(bool withDetail)
{
	json document;
	for (std::vector<opendfx::Buffer *>::iterator it = buffers.begin() ; it != buffers.end(); ++it)
	{
		document.push_back(json::parse((*it)->toJson(withDetail)));
	}
	std::stringstream jsonStream;
	jsonStream << document.dump(true);
	return jsonStream.str();
}

std::string Graph::jsonLinks(bool withDetail)
{
	json document;
	for (std::vector<opendfx::Link *>::iterator it = links.begin() ; it != links.end(); ++it)
	{
		document.push_back(json::parse((*it)->toJson(withDetail)));
	}
	std::stringstream jsonStream;
	jsonStream << document.dump(true);
	return jsonStream.str();
}

std::string Graph::toJson(bool withDetail){
	json document;
	document["id"]      = utils::int2str(id);
	document["name"]    = m_name;
	document["bypass"]  = bypass;
	document["priority"]  = priority;
	document["accels"]  = json::parse(jsonAccels(withDetail));
	document["buffers"] = json::parse(jsonBuffers(withDetail));	
	document["links"]   = json::parse(jsonLinks(withDetail));
	std::stringstream jsonStream;
	jsonStream << document.dump(true);
	return jsonStream.str();
}

int Graph::fromJson(std::string jsonstr){
	json document = json::parse(jsonstr);
	document.at("name").get_to(m_name);
	document.at("bypass").get_to(bypass);
	document.at("priority").get_to(priority);
	json accelsObj = document["accels"];
	for (json::iterator it = accelsObj.begin(); it != accelsObj.end(); ++it) {
		json accelObj = *it;
		opendfx::Accel *accel = new opendfx::Accel(accelObj["name"].get<std::string>(), id, accelObj["type"].get<int>(), utils::str2int(accelObj["id"].get<std::string>()));
		if (accelObj["type"].get<int>() == opendfx::acceltype::accelNode){
			accelCount ++;
		}
		accel->setBSize(accelObj["bSize"].get<int>());
		accels.push_back(accel);
	}
	json buffersObj = document["buffers"];
	for (json::iterator it = buffersObj.begin(); it != buffersObj.end(); ++it) {
		json bufferObj = *it;
		opendfx::Buffer *buffer = new opendfx::Buffer(bufferObj["name"].get<std::string>(), id, utils::str2int(bufferObj["id"].get<std::string>()));
		buffer->setBSize(bufferObj["bSize"].get<int>());
		buffer->setBType(bufferObj["bType"].get<int>());
		buffers.push_back(buffer);
	}
	json linksObj = document["links"];
	for (json::iterator it = linksObj.begin(); it != linksObj.end(); ++it) {
		json linkObj = *it;

		opendfx::Accel * accel   = getAccelByID (utils::str2int(linkObj["accel" ].get<std::string>()));
		opendfx::Buffer * buffer = getBufferByID(utils::str2int(linkObj["buffer"].get<std::string>()));
		opendfx::Link *link      = new opendfx::Link(accel, buffer, linkObj["dir"].get<int>(), utils::str2int(linkObj["id"].get<std::string>()));
		link->setOffset(linkObj["offset"].get<int>());
		link->setTransactionSize(linkObj["transactionSize"].get<int>());
		link->setTransactionIndex(linkObj["transactionIndex"].get<int>());
		link->setChannel(linkObj["channel"].get<int>());
		links.push_back(link);
	}

	//std::cout << "#No of accels  = " << countAccel() << std::endl;
	//std::cout << "#No of buffers = " << countBuffer() << std::endl; 
	//std::cout << "#No of links   = " << countLink() << std::endl; 
	return id;
}

int Graph::setDeleteFlag(bool deleteFlag){
	this->deleteFlag = deleteFlag;
	return 0;
}

bool Graph::getDeleteFlag() const{
	return this->deleteFlag;
}

Graph* Graph::operator + (Graph *graph){
	Graph *ograph = new Graph("merged");
	ograph->copyGraph(this);
	ograph->copyGraph(graph);
	return ograph;
}

Graph* Graph::operator - (Graph *graph){
	Graph *ograph = new Graph("merged");
	ograph->copyGraph(this);
	ograph->cutGraph(graph);
	return ograph;
}

int Graph::copyGraph(opendfx::Graph *graph){
	for (std::vector<opendfx::Accel  *>::iterator it = graph->accels.begin() ; it != graph->accels.end() ; ++it)
	{
		opendfx::Accel* accel = *it;
		addAccel(accel);
	}
	for (std::vector<opendfx::Buffer *>::iterator it = graph->buffers.begin(); it != graph->buffers.end(); ++it)
	{
		opendfx::Buffer* buffer = *it;
		addBuffer(buffer);
	}
	for (std::vector<opendfx::Link   *>::iterator it = graph->links.begin()  ; it != graph->links.end()  ; ++it)
	{
		opendfx::Link* link = *it;
		addLink(link);
	}
	return 0;
}

int Graph::cutGraph(opendfx::Graph *graph){
	for (std::vector<opendfx::Accel  *>::iterator it = graph->accels.begin() ; it != graph->accels.end() ; ++it)
	{
		for (std::vector<opendfx::Accel  *>::iterator itt = this->accels.begin() ; itt != this->accels.end() ; ++itt)
		{
			if(*itt == *it){
				this->accels.erase(itt);
				break;
			}
		}
	}
	for (std::vector<opendfx::Buffer *>::iterator it = graph->buffers.begin(); it != graph->buffers.end(); ++it)
	{
		for (std::vector<opendfx::Buffer *>::iterator itt = this->buffers.begin(); itt != this->buffers.end(); ++itt)
		{
			if(*itt == *it){
				this->buffers.erase(itt);
				break;
			}
		}
	}
	for (std::vector<opendfx::Link   *>::iterator it = graph->links.begin()  ; it != graph->links.end()  ; ++it)
	{
		for (std::vector<opendfx::Link   *>::iterator itt = this->links.begin()  ; itt != this->links.end()  ; ++itt)
		{
			if(*itt == *it){
				this->links.erase(itt);
				break;
			}
		}
	}
	return 0;
}

int Graph::submit(void){
	struct message send_message, recv_message;
	int ret;
	int fd[25];
	int fdcount = 0; 
	int size;	
	domainSocket = (socket_t *)malloc(sizeof(socket_t));
	int sock = initSocket(domainSocket);
	if (sock < 0){
		return -1;
	}

	std::string str = toJson();
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());

	memset(&send_message, '\0', sizeof(struct message));
	send_message.id = GRAPH_INIT;
	send_message.size = str.length();
	send_message.fdcount = 0;
	memcpy(send_message.data, cstr, str.length());
	ret = write(domainSocket->sock_fd, &send_message, HEADERSIZE + send_message.size);
	if (ret < 0){
		std::cout << __func__ << " : socket write failed" << std::endl;
		return -1;
	}
	memset(&recv_message, '\0', sizeof(struct message));
	size = sock_fd_read(domainSocket->sock_fd, &recv_message, fd, &fdcount);
	if (size <= 0){
		return -1;
	}
	int *idptr = (int*)recv_message.data;
	id = *idptr; 
	//std::cout << std::hex << id << std::endl;
	return 0;
}

int Graph::isScheduled(void){
	struct message send_message, recv_message;
	int ret;
	int size;	
	int statusP;
	int iofd[25];
	int fdcount = 0; 
	int ioid[25] = {0, 0};
	usleep(100000);

	memset(&send_message, '\0', sizeof(struct message));
	send_message.id = GRAPH_STAGED;
	send_message.size = sizeof(id);
	send_message.fdcount = 0;
	//std::cout << "@@" << std::endl;
	memcpy(send_message.data, &id, sizeof(id));
	ret = write(domainSocket->sock_fd, &send_message, HEADERSIZE + send_message.size);
	if (ret < 0){
		std::cout << __func__ << " : socket write failed" << std::endl;
		return -1;
	}
	memset(&recv_message, '\0', sizeof(struct message));
	//std::cout << "@@" << std::endl;
	size = sock_fd_read(domainSocket->sock_fd, &recv_message, iofd, &fdcount);
	
	if (size <= 0){
		return -1;
	}
	memcpy(&statusP, (char *)recv_message.data, sizeof(int));
	//std::cout << statusP << std::endl; 
	if (statusP == 1){
		memcpy(ioid, (char *)recv_message.data + sizeof(int), fdcount * sizeof(int));
		for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
		{
			opendfx::Accel* accel = *it;
			if (accel->getType() == opendfx::acceltype::inputNode || accel->getType() == opendfx::acceltype::outputNode){
				for (int i=0; i < fdcount; i++){
					if (accel->getId() == ioid[i]){
						accel->setFd(iofd[i]);
						accel->setParentGraphId(id);
						accel->reAllocateBuffer();
						//std::cout << "#############" << std::endl;
					}	
            		//std::cout << "id : " << std::hex << ioid[i] << " : " << accel->getId() << std::endl;
           			//std::cout << "fd : " << std::hex << iofd[i] << std::endl;
				}
			}
		}
	}
	return statusP;
}

int Graph::getIODescriptors(int **fd, int **id, int *size){
	int count = accels.size() - accelCount;
	int *ioid = (int*) malloc(sizeof(int) * count);
	int *iofd = (int*) malloc(sizeof(int) * count);
	int i = 0;
	//std::cout <<  "$$$$$$$$$$$$$$$$" << accelCount << ":" << accels.size() << std::endl;
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		opendfx::Accel* accel = *it;
		if (accel->getType() == opendfx::acceltype::inputNode || accel->getType() == opendfx::acceltype::outputNode){
			ioid[i] = accel->getId();
			iofd[i] = accel->getFd();
			//std::cout << "id : " << std::hex << ioid[i] << std::endl;
			//std::cout << "fd : " << std::hex << iofd[i] << std::endl;
			i++;
		}
	}

	*id = ioid;
	*fd = iofd;
	*size = count;
	return 0;
}

int Graph::allocateIOBuffers()
{
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		(*it)->allocateBuffer(xrt_fd);
	}
	abstract = false;
	return 0;
}

int Graph::deallocateIOBuffers()
{
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		(*it)->deallocateBuffer(xrt_fd);
	}
	return 0;
}

int Graph::redeallocateIOBuffers()
{
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		(*it)->reDeallocateBuffer();
	}
	return 0;
}

int Graph::InterRMParser(){
	for (std::vector<opendfx::Link *>::iterator it = links.begin() ; it != links.end(); ++it)
	{
		opendfx::Link *link = *it;
		if (link->getDir() == 0){
			link->getBuffer()->setInterRMReqIn(link->getAccel()->getiInterRMCompatible());
			link->getBuffer()->setSinkSlot(link->getAccel()->getSlot());
			std::cout << link->getAccel()->getiInterRMCompatible() << "::" << link->getAccel()->getSlot() << std::endl;
		}
		else{
			link->getBuffer()->setInterRMReqOut(link->getAccel()->getiInterRMCompatible());
			link->getBuffer()->setSourceSlot(link->getAccel()->getSlot());
			std::cout << link->getAccel()->getiInterRMCompatible() << "::" << link->getAccel()->getSlot() << std::endl;
		}
	}
	return 0;
}

int Graph::allocateBuffers(){
	for (std::vector<opendfx::Buffer *>::iterator it = buffers.begin() ; it != buffers.end(); ++it)
	{
		(*it)->allocateBuffer(xrt_fd);
	}
	abstract = false;
	return 0;
}

int Graph::deallocateBuffers(){
	for (std::vector<opendfx::Buffer *>::iterator it = buffers.begin() ; it != buffers.end(); ++it)
	{
		(*it)->deallocateBuffer(xrt_fd);
	}
	return 0;
}

int Graph::allocateAccelResources()
{
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		(*it)->allocateAccelResource();
	}
	abstract = false;
	return 0;
}

int Graph::deallocateAccelResources()
{
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		(*it)->deallocateAccelResource();
	}
	return 0;
}

int Graph::createExecutionDependencyList(){
	std::cout << "creating Execution Dependency List .." << std::endl;
	for (std::vector<opendfx::Link   *>::iterator it = links.begin()  ; it != links.end()  ; ++it)
	{
		opendfx::Link* link = *it; 
		opendfx::ExecutionDependency* eDependency = new opendfx::ExecutionDependency(link);
		executionDependencyList.push_back(eDependency);
	}

	for (std::vector<opendfx::ExecutionDependency *>::iterator it = executionDependencyList.begin()  ; it != executionDependencyList.end()  ; ++it)
	{
		opendfx::ExecutionDependency *eDependency = *it; 
		opendfx::Link* link = eDependency->getLink(); 
		for (std::vector<opendfx::Link   *>::iterator itt = links.begin()  ; itt != links.end()  ; ++itt)
		{
			opendfx::Link* dependentLink = *itt;
			if (link->getDir() == opendfx::direction::toAccel && dependentLink->getDir() == opendfx::direction::fromAccel &&
					link->getTransactionIndex() == dependentLink->getTransactionIndex()){
				eDependency->addDependency(dependentLink);
			}
			if (link->getDir() == opendfx::direction::fromAccel && dependentLink->getDir() == opendfx::direction::toAccel &&
					link->getTransactionIndex() == dependentLink->getTransactionIndex()){
				eDependency->addDependency(dependentLink);
			}

		}
	}
	return 0;
}

int Graph::createScheduleList(){
	int index = 0, size = 0, last = 0, first = 0;
	int pending = 0;
	std::cout << "Creating Schedule List" << std::endl;
	while(1){
		for (std::vector<opendfx::ExecutionDependency *>::iterator it = executionDependencyList.begin()  ; it != executionDependencyList.end()  ; ++it)
		{
			opendfx::ExecutionDependency *eDependency = *it; 
			opendfx::Link* link = eDependency->getLink();
			size = link->getTransactionSize() - link->getTransactionSizeScheduled();
			if(size > link->getBuffer()->getBSize()){
				size = link->getBuffer()->getBSize();
			}
			if(size > 0){
				if(link->getTransactionSizeScheduled() <= 0){
					first = 1;
				}
				else{
					first = 0;
				}
				link->setTransactionSizeScheduled(size + link->getTransactionSizeScheduled());
				if(link->getTransactionSize() - link->getTransactionSizeScheduled() <= 0){
					last = 1;
				}
				else{
					last = 0;
				} 
				opendfx::Schedule* schedule = new opendfx::Schedule(eDependency, index, size, link->getOffset(), last, first);
				index ++;
				scheduleList.push_back(schedule);
				pending += size;
			}
		}
		if (pending == 0) break;
		pending = 0;
	}
	return 0;
}

int Graph::getScheduleListInfo(){
	for (std::vector<opendfx::Schedule *>::iterator it = scheduleList.begin()  ; it != scheduleList.end()  ; ++it){
		opendfx::Schedule * schedule = *it;
		schedule->getInfo();
	}
	return 0;
}

int Graph::getExecutionDependencyList(){
	std::cout << "Execution Dependency Info .." << std::endl;
	for (std::vector<opendfx::ExecutionDependency *>::iterator it = executionDependencyList.begin()  ; it != executionDependencyList.end()  ; ++it)
	{
		opendfx::ExecutionDependency *eDependency = *it; 
		std::cout << eDependency->getInfo(); 
	}
	return 0;
}

int Graph::removeCompletedSchedule(){
	scheduleList.erase(std::remove_if(scheduleList.begin(), scheduleList.end(), Schedule::staticGetDeleteFlag), scheduleList.end());
	return 0;
}

int Graph::execute(){
	if (bypass == true && countAccel() == 1){	
		return Graph::executeBypass();
	}
	else{
		return Graph::executeScheduler();
	}
}

int Graph::executeBypass(){
	std::cout << "!!##########################################" << std::endl;
	//std::cout << accels[0]->info() << std::endl;
	for (std::vector<opendfx::Schedule *>::iterator it = scheduleList.begin()  ; it != scheduleList.end()  ; ++it){
		opendfx::Schedule * schedule = *it;
		opendfx::Schedule * lastSchedule = *it;
		//int index = schedule->getIndex();
		int size = schedule->getSize();
		int offset = schedule->getOffset();
		//int last = schedule->getLast();
		int first = schedule->getFirst();
		ExecutionDependency *eDependency = schedule->getEDependency();
		opendfx::Link* link = eDependency->getLink();
		opendfx::Accel* accel = link->getAccel();
		opendfx::Buffer* buffer = link->getBuffer();
		Device_t* device = accel->getDevice();
		DeviceConfig_t *deviceConfig = accel->getConfig();
		BuffConfig_t *buffConfig = buffer->getConfig();
		if (link->getDir() == opendfx::direction::fromAccel){
			if(eDependency->isDependent()){
				int lastSize = lastSchedule->getSize();
				int lastOffset = lastSchedule->getOffset();
				int lastFirst = lastSchedule->getFirst();
				ExecutionDependency *lastEDependency = lastSchedule->getEDependency();
				opendfx::Link* lastLink = lastEDependency->getLink();
				opendfx::Accel* lastAccel = lastLink->getAccel();
				opendfx::Buffer* lastBuffer = lastLink->getBuffer();
				Device_t* lastDevice = lastAccel->getDevice();
				DeviceConfig_t *lastDeviceConfig = lastAccel->getConfig();
				BuffConfig_t *lastBuffConfig = lastBuffer->getConfig();
				device->S2MMData(deviceConfig, buffConfig, offset, size, first);
				device->S2MMData(deviceConfig, buffConfig, offset, size, first);
				lastDevice->MM2SData(lastDeviceConfig, lastBuffConfig, lastOffset, lastSize, lastFirst, lastLink->getChannel());
				while(device->S2MMDone(deviceConfig) <= 0){
				}
				while(lastDevice->S2MMDone(lastDeviceConfig) <= 0){
				}
				schedule->setDeleteFlag(true);
				lastSchedule->setDeleteFlag(true);

			}
			else{
				device->S2MMData(deviceConfig, buffConfig, offset, size, first);
				while(device->S2MMDone(deviceConfig) <= 0){
				}
				schedule->setDeleteFlag(true);
			}
		}
		else if (link->getDir() == opendfx::direction::toAccel){
			if(eDependency->isDependent()){
				lastSchedule = schedule;
			}
			else{
				device->MM2SData(deviceConfig, buffConfig, offset, size, first, link->getChannel());
				while(device->MM2SDone(deviceConfig) <= 0){
				}
				schedule->setDeleteFlag(true);
			}
		}
	}
	std::cout << "############################################" << std::endl;
	return scheduleList.size();
}
int Graph::executeScheduler(){
	//std::cout << "executing graph ..." << std::endl;
	usleep(10000);
	for (std::vector<opendfx::Schedule *>::iterator it = scheduleList.begin()  ; it != scheduleList.end()  ; ++it)
	{
		opendfx::Schedule * schedule = *it;
		ExecutionDependency *eDependency = schedule->getEDependency();
		int index = schedule->getIndex();
		int size = schedule->getSize();
		int offset = schedule->getOffset();
		int last = schedule->getLast();
		int first = schedule->getFirst();
		opendfx::Link* link = eDependency->getLink();
		opendfx::Accel* accel = link->getAccel();
		opendfx::Buffer* buffer = link->getBuffer();
		Device_t* device = accel->getDevice();
		DeviceConfig_t *deviceConfig = accel->getConfig();
		BuffConfig_t *buffConfig = buffer->getConfig();
		if (link->getDir() == opendfx::direction::fromAccel){
			//std::cout << ">>>" << index  << "B" << buffer->getStatus() << "A" << accel->getS2MMStatus() << std::endl;
			//std::cout << buffConfig->interRMEnabled;
			if (accel->getS2MMStatus() == opendfx::status::Idle){
				if(buffer->getStatus() == opendfx::bufferStatus::BuffIsEmpty){
					accel->setS2MMCurrentIndex(index);
					accel->setCurrentIndex(link->getTransactionIndex());
					accel->setS2MMStatus(opendfx::status::Ready);
				}
			}
			else{
				if (accel->getS2MMCurrentIndex() == index){
					if (accel->getS2MMStatus() == opendfx::status::Ready){
						if(eDependency->isDependent()){
							if(accel->getMM2SStatus() == opendfx::status::Ready && accel->getCurrentIndex() == link->getTransactionIndex()){
								if (device->S2MMData(deviceConfig, buffConfig, offset, size, first) >= 0){
									accel->setS2MMStatus(opendfx::status::Busy);
									if (buffer->getInterRMEnabled()){
										buffer->setStatus(opendfx::bufferStatus::BuffIsStream);
									}else{
										buffer->setStatus(opendfx::bufferStatus::BuffIsBusy);
									}
									std::cout << schedule->getDeleteFlag() << " " << index << " : " << accel->getCurrentIndex() << " [S2MM] ";
									std::cout << accel->getMM2SCurrentIndex() << " MM2S : " << accel->getMM2SStatus() << " : ";
									std::cout << accel->getS2MMCurrentIndex() << " S2MM : " << accel->getS2MMStatus() << " : ";
									std::cout << "Buffer : " << buffer->getStatus() << "Transaction"<< std::endl;
								}
								else{
								}
							}
						}
						else
						{
								std::cout << buffConfig->interRMEnabled;
							if(device->S2MMData(deviceConfig, buffConfig, offset, size, first) >= 0){
								accel->setS2MMStatus(opendfx::status::Busy);
								if (buffer->getInterRMEnabled()){
									buffer->setStatus(opendfx::bufferStatus::BuffIsStream);
								}else{
									buffer->setStatus(opendfx::bufferStatus::BuffIsBusy);
								}
								std::cout << schedule->getDeleteFlag() << " " << index << " : " << accel->getCurrentIndex() << " [S2MM] ";
								std::cout << accel->getMM2SCurrentIndex() << " MM2S : " << accel->getMM2SStatus() << " : ";
								std::cout << accel->getS2MMCurrentIndex() << " S2MM : " << accel->getS2MMStatus() << " : ";
								std::cout << "Buffer : " << buffer->getStatus() << "Transaction"<< std::endl;
							}
							else{
							}
						}
					}
					else if (accel->getS2MMStatus() == opendfx::status::Busy){
						int ret = device->S2MMDone(deviceConfig);
						if(ret > 0){
							accel->setS2MMStatus(opendfx::status::Idle);
							buffer->setStatus(opendfx::bufferStatus::BuffIsFull);
							schedule->setDeleteFlag(true);
							std::cout << schedule->getDeleteFlag() << " " << index << " : " << accel->getCurrentIndex() << " [S2MM] ";
							std::cout << accel->getMM2SCurrentIndex() << " MM2S : " << accel->getMM2SStatus() << " : ";
							std::cout << accel->getS2MMCurrentIndex() << " S2MM : " << accel->getS2MMStatus() << " : ";
							std::cout << "Buffer : " << buffer->getStatus() << "Done"<< std::endl;
						}
						else{
							accel->setS2MMStatus(opendfx::status::Busy);
							if (buffer->getInterRMEnabled()){
								buffer->setStatus(opendfx::bufferStatus::BuffIsStream);
							}else{
								buffer->setStatus(opendfx::bufferStatus::BuffIsBusy);
							}
						}
					}
				}
			}
		}
		else{
			//std::cout << "<<<" << index  << "B" << buffer->getStatus() << "A" << accel->getMM2SStatus() << std::endl;
			//std::cout << buffConfig->interRMEnabled;
			if (accel->getMM2SStatus() == opendfx::status::Idle){
				if(buffer->getStatus() == opendfx::bufferStatus::BuffIsFull ||
					buffer->getStatus() == opendfx::bufferStatus::BuffIsStream){
					accel->setMM2SCurrentIndex(index);
					accel->setCurrentIndex(link->getTransactionIndex());
					accel->setMM2SStatus(opendfx::status::Ready);
				}
			}
			else{
				if (accel->getMM2SCurrentIndex() == index){
					if (accel->getMM2SStatus() == opendfx::status::Ready){
						if(buffer->getStatus() == opendfx::bufferStatus::BuffIsStream || 
							buffer->getStatus() == opendfx::bufferStatus::BuffIsFull){
							if(eDependency->isDependent()){
								if (accel->getS2MMStatus() == opendfx::status::Busy){
								std::cout << buffConfig->interRMEnabled;
									if (device->MM2SData(deviceConfig, buffConfig, offset, size, last, link->getChannel()) >= 0){
										accel->setMM2SStatus(opendfx::status::Busy);
										if (buffer->getInterRMEnabled()){
											buffer->setStatus(opendfx::bufferStatus::BuffIsStream);
										}else{
											buffer->setStatus(opendfx::bufferStatus::BuffIsBusy);
										}
										std::cout << schedule->getDeleteFlag() << " " << index << " : " << accel->getCurrentIndex() << " [MM2S] ";
										std::cout << accel->getMM2SCurrentIndex() << " MM2S : " << accel->getMM2SStatus() << " : ";
										std::cout << accel->getS2MMCurrentIndex() << " S2MM : " << accel->getS2MMStatus() << " : ";
										std::cout << "Buffer : " << buffer->getStatus() << "Transaction"<< std::endl;
									}
									else{
										std::cout << "blocked" << std::endl;
									}
								}
							}
							else{
								std::cout << buffConfig->interRMEnabled;
								if (device->MM2SData(deviceConfig, buffConfig, offset, size, last, link->getChannel()) >= 0){
									accel->setMM2SStatus(opendfx::status::Busy);
									std::cout << schedule->getDeleteFlag() << " " << index << " : " << accel->getCurrentIndex() << " [MM2S] ";
									std::cout << accel->getMM2SCurrentIndex() << " MM2S : " << accel->getMM2SStatus() << " : ";
									std::cout << accel->getS2MMCurrentIndex() << " S2MM : " << accel->getS2MMStatus() << " : ";
									std::cout << "Buffer : " << buffer->getStatus() << "Transaction"<< std::endl;
								}
								else{
									std::cout << "blocked" << std::endl;
								}
							}
						}
					}
					else if (accel->getMM2SStatus() == opendfx::status::Busy){
						int ret = device->MM2SDone(deviceConfig);
						if(ret > 0){
							accel->setMM2SStatus(opendfx::status::Idle);
							buffer->setStatus(opendfx::bufferStatus::BuffIsEmpty);
							schedule->setDeleteFlag(true);
							std::cout << schedule->getDeleteFlag() << " " << index << " : " << accel->getCurrentIndex() << " [MM2S] ";
							std::cout << accel->getMM2SCurrentIndex() << " MM2S : " << accel->getMM2SStatus() << " : ";
							std::cout << accel->getS2MMCurrentIndex() << " S2MM : " << accel->getS2MMStatus() << " : ";
							std::cout << "Buffer : " << buffer->getStatus() << "Done"<< std::endl;
						}
						else{
							accel->setMM2SStatus(opendfx::status::Busy);
							if (buffer->getInterRMEnabled()){
								buffer->setStatus(opendfx::bufferStatus::BuffIsStream);
							}else{
								buffer->setStatus(opendfx::bufferStatus::BuffIsBusy);
							}
						}
					}

				}
			}
		}
	}


	return scheduleList.size();
}
