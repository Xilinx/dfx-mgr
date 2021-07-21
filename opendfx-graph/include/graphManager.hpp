// graphManager.hpp
#ifndef GRAPH_MANAGER_HPP_
#define GRAPH_MANAGER_HPP_

#include <string>
#include <vector>
#include <accel.hpp>
#include <buffer.hpp>
#include <link.hpp>
#include "graph.hpp"


namespace opendfx {

class GraphManager {

	public:
		explicit GraphManager();
		std::string getInfo() const;
		int addGraph(opendfx::Graph *graph);
		int delGraph(opendfx::Graph *graph);
		int listGraphs();
		//std::string jsonAccels(bool withDetail = false);
		//std::string jsonBuffers(bool withDetail = false);
		//std::string jsonLinks(bool withDetail = false);
		//std::string toJson(bool withDetail = false);
		
	private:
		std::vector<opendfx::Graph *> graphs;
		int id;
		std::string strid;
	};
} // #end of graphManager
#endif // GRAPH_MANAGER_HPP_
