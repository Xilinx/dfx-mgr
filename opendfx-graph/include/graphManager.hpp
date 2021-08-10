// graphManager.hpp
#ifndef GRAPH_MANAGER_HPP_
#define GRAPH_MANAGER_HPP_

#include <string>
#include <vector>
#include <accel.hpp>
#include <buffer.hpp>
#include <link.hpp>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include "graph.hpp"


namespace opendfx {

	class GraphManager {

		public:
			explicit GraphManager();
			std::string getInfo() const;
			int addGraph(opendfx::Graph *graph);
			int delGraph(opendfx::Graph *graph);
			int listGraphs();
			Graph* mergeGraphs();
			int stageGraphs(int slots=3);
			int scheduleGraph();
			int startServices(int slots=3);
			int stopServices();

		private:
			std::vector<opendfx::Graph *> graphsQueue[3];
			std::mutex graphQueue_mutex;
			//std::vector<opendfx::Graph *> graphsQueue1;
			//td::vector<opendfx::Graph *> graphsQueue2;
			std::vector<opendfx::Graph *> stagedGraphs;
			std::mutex stagedGraphs_mutex;
			int id;
			std::string strid;
			//std::queue<opendfx::Graph *> graphsQueues[3];
			std::thread * stageGraphThread;
			static volatile std::atomic<bool> quit;
			static volatile pthread_t main_thread;
			void sigint_handler (int);
	};
} // #end of graphManager
#endif // GRAPH_MANAGER_HPP_
