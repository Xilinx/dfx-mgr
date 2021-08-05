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
			Graph mergeGraphs();
			int stageGraphs(int slots=3);
			int scheduleGraph();

		private:
			std::vector<opendfx::Graph *> graphsQueue0;
			std::vector<opendfx::Graph *> graphsQueue1;
			std::vector<opendfx::Graph *> graphsQueue2;
			std::vector<opendfx::Graph *> stagedGraphs;
			int id;
			std::string strid;
	};
} // #end of graphManager
#endif // GRAPH_MANAGER_HPP_
