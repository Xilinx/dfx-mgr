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
			int addGraph(opendfx::Graph &graph);
			int delGraph(opendfx::Graph &graph);
			int listGraphs();
			opendfx::Graph mergeGraphs();

		private:
			std::vector<opendfx::Graph *> graphs;
			int id;
			std::string strid;
	};
} // #end of graphManager
#endif // GRAPH_MANAGER_HPP_
