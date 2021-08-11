// graph.hpp
#ifndef GRAPH_HPP_
#define GRAPH_HPP_

#include <string>
#include <vector>
#include <mutex>
#include <accel.hpp>
#include <buffer.hpp>
#include <link.hpp>


namespace opendfx {

	class Graph {

		public:
			explicit Graph(const std::string &name, int priority = 0);
			std::string info() const;
			std::string getInfo() const;
			Accel* addAccel(const std::string &name);
			Accel* addAccel(Accel *accel);
			Accel* addInputNode(const std::string &name);
			Accel* addOutputNode(const std::string &name);
			Buffer* addBuffer(const std::string &name);
			Buffer* addBuffer(Buffer *buffer);
			Link* connectInputBuffer (Accel *accel, Buffer *buffer);
			Link* connectOutputBuffer(Accel *accel, Buffer *buffer);
			Link* addLink(Link *link);
			int copyGraph(Graph *graph);
			int cutGraph (Graph *graph);
			int delAccel(Accel *accel);
			int delBuffer(Buffer *buffer);
			int delLink(Link *link);
			int countAccel();
			int countBuffer();
			int countLink();
			opendfx::Accel  * getAccelByID(std::string strid);
			opendfx::Buffer * getBufferByID(std::string strid);
			int listAccels();
			int listBuffers();
			int listLinks();
			std::string jsonAccels(bool withDetail = false);
			std::string jsonBuffers(bool withDetail = false);
			std::string jsonLinks(bool withDetail = false);
			std::string toJson(bool withDetail = false);
			std::string fromJson(std::string jsonstr);

			int setDeleteFlag(bool deleteFlag);
			bool getDeleteFlag() const;
			inline int getPriority() {
				return priority;
			};
			inline int setScheduled(bool scheduled){
				this->scheduled = scheduled;
			}
			inline bool getScheduled(){
				return this->scheduled;
			}
			inline int lockAccess(){
				graph_mutex.lock();
			}
			inline int unlockAccess(){
				graph_mutex.unlock();
			}
			inline bool staticGetDeleteFlag(Graph *graph) {
				return graph->getDeleteFlag();
			};
			Graph* operator + (Graph* graph);
			Graph* operator - (Graph *graph);
		private:
			std::string m_name;
			int id;
			int priority;
			bool scheduled;
			std::string strid;
			std::vector<opendfx::Accel *> accels;
			std::vector<opendfx::Buffer *> buffers;
			std::vector<opendfx::Link *> links;
			bool deleteFlag;
			int accelCount;
			std::mutex graph_mutex;
	};
} // #end of graph

#endif // GRAPH_HPP_
