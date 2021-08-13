// wrapper.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include <sys/socket.h>
#include "graph.hpp"
#include "accel.hpp"
#include "buffer.hpp"
#include "link.hpp"
#include "nlohmann/json.hpp"
#include "utils.hpp"
#include <dfx-mgr/dfxmgr_client.h>

using json = nlohmann::json;
using opendfx::Graph;

Graph::Graph(const std::string &name, int priority) : m_name(name) {
	int i;
	//char * block;
	//short size = 1;
	std::ifstream urandom("/dev/urandom", std::ios::in|std::ios::binary); // Seed initialisation based on /dev/urandom
	urandom.read((char*)&i, sizeof(i));
	srand(i ^ time(0));
	urandom.close();

	utils::setID(id, strid);
	if (priority < 3 && priority >= 0){
		this->priority = priority;
	}
	else{
		this->priority = 0;
	}
	accelCount = 0;
}

std::string Graph::info() const {
	return "Graph ID: " + strid;
}

std::string Graph::getInfo() const {
	std::stringstream stream;
	stream << "{\"name\": \"" << m_name << "_" << strid << "\", \"priority\": " << priority << "}";
	return stream.str();
}

opendfx::Accel* Graph::addAccel(const std::string &name){
	opendfx::Accel *accel = new opendfx::Accel(name, strid);
	accels.push_back(accel);
	accelCount ++;
	return accel;
}

opendfx::Accel* Graph::addInputNode(const std::string &name){
	opendfx::Accel *accel = new opendfx::Accel(name, strid, opendfx::acceltype::inputNode);
	accels.push_back(accel);
	return accel;
}

opendfx::Accel* Graph::addOutputNode(const std::string &name){
	opendfx::Accel *accel = new opendfx::Accel(name, strid, opendfx::acceltype::outputNode);
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

opendfx::Buffer* Graph::addBuffer(const std::string &name){
	opendfx::Buffer *buffer= new opendfx::Buffer(name, strid);
	buffers.push_back(buffer);
	return buffer;
}

opendfx::Buffer* Graph::addBuffer(opendfx::Buffer *buffer){
	buffers.push_back(buffer);
	return buffer;
}

opendfx::Link* Graph::connectInputBuffer(opendfx::Accel *accel, opendfx::Buffer *buffer){
	opendfx::Link *link = new opendfx::Link(accel, buffer, opendfx::direction::toAccel, strid);
	links.push_back(link);
	return link;
}

opendfx::Link* Graph::connectOutputBuffer(opendfx::Accel *accel, opendfx::Buffer *buffer){
	opendfx::Link *link = new opendfx::Link(accel, buffer, opendfx::direction::fromAccel, strid);
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

opendfx::Accel * Graph::getAccelByID(std::string strid)
{
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		opendfx::Accel* accel = *it;
			std::cout << accel->getId() << " : " << strid << std::endl;
		if (accel->getId() == strid){
			std::cout << accel->getId() << " : " << strid << std::endl;
			std::cout << ' ' << accel->info() << '\n';
			return accel;
		}
	}
	return NULL;
}

opendfx::Buffer * Graph::getBufferByID(std::string strid)
{
	for (std::vector<opendfx::Buffer *>::iterator it = buffers.begin() ; it != buffers.end(); ++it)
	{
		opendfx::Buffer* buffer = *it;
		if (buffer->getId() == strid){
			std::cout << buffer->getId() << " : " << strid << std::endl;
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
	document["id"]      = strid;
	document["name"]    = m_name;
	document["accels"]  = json::parse(jsonAccels(withDetail));
	document["buffers"] = json::parse(jsonBuffers(withDetail));	
	document["links"]   = json::parse(jsonLinks(withDetail));
	std::stringstream jsonStream;
	jsonStream << document.dump(true);
	return jsonStream.str();
}

std::string Graph::fromJson(std::string jsonstr){
	json document = json::parse(jsonstr);
	//document.at("id").get_to(strid);
	document.at("name").get_to(m_name);
	json accelsObj = document["accels"];
	for (json::iterator it = accelsObj.begin(); it != accelsObj.end(); ++it) {
		json accelObj = *it;
		opendfx::Accel *accel = new opendfx::Accel(accelObj["name"].get<std::string>(), strid, accelObj["type"].get<int>(), accelObj["id"].get<std::string>());
		accels.push_back(accel);
	}
	json buffersObj = document["buffers"];
	for (json::iterator it = buffersObj.begin(); it != buffersObj.end(); ++it) {
		json bufferObj = *it;

		opendfx::Buffer *buffer = new opendfx::Buffer(bufferObj["name"].get<std::string>(), strid, bufferObj["id"].get<std::string>());
		buffers.push_back(buffer);
	}
	json linksObj = document["links"];
	for (json::iterator it = linksObj.begin(); it != linksObj.end(); ++it) {
		json linkObj = *it;

		opendfx::Accel * accel   = getAccelByID (linkObj["accel" ].get<std::string>());
		opendfx::Buffer * buffer = getBufferByID(linkObj["buffer"].get<std::string>());
		opendfx::Link *link      = new opendfx::Link(accel, buffer, linkObj["dir"].get<int>(), linkObj["id"].get<std::string>());
		links.push_back(link);
	}
	return strid;
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
	int fd[25];
	int fdcount = 0; 
	
	this->domainSocket = (socket_t *)malloc(sizeof(socket_t));
	int status = initSocket(domainSocket);
	if (status < 0){
		return -1;
	}
	
	std::string str = toJson();
	char *cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	
	status = graphClientSubmit(domainSocket, cstr, str.length(), fd, &fdcount);
	if (status < 0){
		return -1;
	}
	std::cout << fdcount << std::endl;
	return 0;
}
