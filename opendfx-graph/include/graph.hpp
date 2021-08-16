// graph.hpp
#ifndef GRAPH_HPP_
#define GRAPH_HPP_

#include <string>
#include <vector>
#include <mutex>
#include <accel.hpp>
#include <buffer.hpp>
#include <link.hpp>
#include <sys/socket.h>
#include <dfx-mgr/dfxmgr_client.h>


namespace opendfx {

	class Graph {

		public:
			explicit Graph(const std::string &name, int priority = 0);
			std::string info() const;
			std::string getInfo() const;
			Accel* addAccel(const std::string &name);
			Accel* addAccel(Accel *accel);
			Accel* addInputNode(const std::string &name, int bSize);
			Accel* addOutputNode(const std::string &name, int bSize);
			Buffer* addBuffer(const std::string &name, int bSize, int bType);
			Buffer* addBuffer(Buffer *buffer);
			Link* connectInputBuffer (Accel *accel, Buffer *buffer,
				int offset, int transactionSize, int transactionIndex, int channel);
			Link* connectOutputBuffer(Accel *accel, Buffer *buffer,
				int offset, int transactionSize, int transactionIndex, int channel);
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
				return 0;
			}
			inline bool getScheduled(){
				return this->scheduled;
			}
			inline int lockAccess(){
				graph_mutex.lock();
				return 0;
			}
			inline int unlockAccess(){
				graph_mutex.unlock();
				return 0;
			}
			inline bool staticGetDeleteFlag(Graph *graph) {
				return graph->getDeleteFlag();
			};
			Graph* operator + (Graph* graph);
			Graph* operator - (Graph *graph);
			int submit(void);
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
			socket_t * domainSocket;
	};
} // #end of graph

#endif // GRAPH_HPP_
