// wrapper.cpp
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include "graph.hpp"
#include "accel.hpp"
#include "buffer.hpp"
#include "link.hpp"

using opendfx::Graph;

Graph::Graph(const std::string &name, int priority) : m_name(name) {
	id = rand() % 0x10000;
	std::stringstream stream;
	stream << std::hex << id;
	strid = stream.str();
	if (priority < 3 && priority >= 0){
		this->priority = priority;
	}
	else{
		this->priority = 0;
	}
	accelCount = 0;
	//std::cout << strid << "\n";
}

std::string Graph::info() const {
	//std::cout << "Graph ID: " << strid << "\n";
	return "Graph ID: " + strid;
}

std::string Graph::getInfo() const {
	std::stringstream stream;
	stream << "{\"name\": \"" << m_name << "_" << strid << "\", \"priority\": " << priority << "}";
	return stream.str();

	//return "{\"name\": \"" + m_name + "_" + strid + "\", \"priority\": " + priority + "}";
}

opendfx::Accel* Graph::addAccel(const std::string &name){
	opendfx::Accel *accel = new opendfx::Accel(name);
	accels.push_back(accel);
	accelCount ++;
	return accel;
}

opendfx::Accel* Graph::addInputNode(const std::string &name){
	opendfx::Accel *accel = new opendfx::Accel(name, opendfx::acceltype::inputNode);
	accels.push_back(accel);
	return accel;
}

opendfx::Accel* Graph::addOutputNode(const std::string &name){
	opendfx::Accel *accel = new opendfx::Accel(name, opendfx::acceltype::outputNode);
	accels.push_back(accel);
	return accel;
}

opendfx::Accel* Graph::addAccel(opendfx::Accel *accel){
	accels.push_back(accel);
	return accel;
}

opendfx::Buffer* Graph::addBuffer(const std::string &name){
	opendfx::Buffer *buffer= new opendfx::Buffer(name);
	buffers.push_back(buffer);
	return buffer;
}

opendfx::Buffer* Graph::addBuffer(opendfx::Buffer *buffer){
	buffers.push_back(buffer);
	return buffer;
}

opendfx::Link* Graph::addInputBuffer(opendfx::Accel *accel, opendfx::Buffer *buffer){
	opendfx::Link *link = new opendfx::Link(accel, buffer, opendfx::direction::toAccel);
	links.push_back(link);
	return link;
}

opendfx::Link* Graph::addOutputBuffer(opendfx::Accel *accel, opendfx::Buffer *buffer){
	opendfx::Link *link = new opendfx::Link(accel, buffer, opendfx::direction::fromAccel);
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
	std::stringstream jsonStream;
	jsonStream << "[ ";
	bool first = true;
	for (std::vector<opendfx::Accel *>::iterator it = accels.begin() ; it != accels.end(); ++it)
	{
		if (first){
			first = false;
			jsonStream << "\n";
		}
		else{
			jsonStream << ",\n";
		}
		jsonStream << "\t" << (*it)->toJson(withDetail);
	}
	jsonStream << "]";
	return jsonStream.str();
}

std::string Graph::jsonBuffers(bool withDetail)
{
	std::stringstream jsonStream;
	jsonStream << "[ ";
	bool first = true;
	for (std::vector<opendfx::Buffer *>::iterator it = buffers.begin() ; it != buffers.end(); ++it)
	{
		if (first){
			first = false;
			jsonStream << "\n";
		}
		else{
			jsonStream << ",\n";
		}
		jsonStream << "\t" << (*it)->toJson(withDetail);
	}
	jsonStream << "]";
	return jsonStream.str();
}

std::string Graph::jsonLinks(bool withDetail)
{
	std::stringstream jsonStream;
	jsonStream << "[ ";
	bool first = true;
	for (std::vector<opendfx::Link *>::iterator it = links.begin() ; it != links.end(); ++it)
	{
		if (first){
			first = false;
			jsonStream << "\n";
		}
		else{
			jsonStream << ",\n";
		}
		jsonStream << "\t" << (*it)->toJson(withDetail);
	}
	jsonStream << "]";
	return jsonStream.str();
}

std::string Graph::toJson(bool withDetail){
	std::stringstream jsonStream;
	jsonStream << "{\n";
	jsonStream << "\"id\"\t: "      << strid << ",\n";
	jsonStream << "\"name\"\t: "      << m_name << ",\n";
	jsonStream << "\"accels\"\t: "  << jsonAccels(withDetail) << ",\n";
	jsonStream << "\"buffers\"\t: " << jsonBuffers(withDetail) << ",\n";
	jsonStream << "\"links\"\t: "   << jsonLinks(withDetail) << "\n";
	jsonStream << "}\n";
	return jsonStream.str();
}

int Graph::setDeleteFlag(bool deleteFlag){
	this->deleteFlag = deleteFlag;
	return 0;
}

bool Graph::getDeleteFlag() const{
	return this->deleteFlag;
}

Graph Graph::operator + (Graph &graph){
	Graph ograph("merged");
	ograph.copyGraph(*this);
	ograph.copyGraph(graph);
	return ograph;
}

Graph Graph::operator - (Graph &graph){
	Graph ograph("merged");
	ograph.copyGraph(*this);
	ograph.cutGraph(graph);
	return ograph;
}

int Graph::copyGraph(opendfx::Graph &graph){
	for (std::vector<opendfx::Accel  *>::iterator it = graph.accels.begin() ; it != graph.accels.end() ; ++it)
	{
		opendfx::Accel* accel = *it;
		addAccel(accel);
	}
	for (std::vector<opendfx::Buffer *>::iterator it = graph.buffers.begin(); it != graph.buffers.end(); ++it)
	{
		opendfx::Buffer* buffer = *it;
		addBuffer(buffer);
	}
	for (std::vector<opendfx::Link   *>::iterator it = graph.links.begin()  ; it != graph.links.end()  ; ++it)
	{
		opendfx::Link* link = *it;
		addLink(link);
	}
	return 0;
}

int Graph::cutGraph(opendfx::Graph &graph){
	for (std::vector<opendfx::Accel  *>::iterator it = graph.accels.begin() ; it != graph.accels.end() ; ++it)
	{
		for (std::vector<opendfx::Accel  *>::iterator itt = this->accels.begin() ; itt != this->accels.end() ; ++itt)
		{
			if(*itt == *it){
				this->accels.erase(itt);
				break;
			}
		}
	}
	for (std::vector<opendfx::Buffer *>::iterator it = graph.buffers.begin(); it != graph.buffers.end(); ++it)
	{
		for (std::vector<opendfx::Buffer *>::iterator itt = this->buffers.begin(); itt != this->buffers.end(); ++itt)
		{
			if(*itt == *it){
				this->buffers.erase(itt);
				break;
			}
		}
	}
	for (std::vector<opendfx::Link   *>::iterator it = graph.links.begin()  ; it != graph.links.end()  ; ++it)
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
