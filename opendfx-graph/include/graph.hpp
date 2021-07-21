// graph.hpp
#ifndef GRAPH_HPP_
#define GRAPH_HPP_

#include <string>
#include <vector>
#include <accel.hpp>
#include <buffer.hpp>
#include <link.hpp>


namespace opendfx {

class Graph {

	public:
		explicit Graph(const std::string &name);
		std::string info() const;
		std::string getInfo() const;
		Accel* addAccel(const std::string &name);
		Buffer* addBuffer(const std::string &name);
		Link* addInputBuffer (opendfx::Accel *accel, opendfx::Buffer *buffer);
		Link* addOutputBuffer(opendfx::Accel *accel, opendfx::Buffer *buffer);
		int delAccel(opendfx::Accel *accel);
		int delBuffer(opendfx::Buffer *buffer);
		int delLink(opendfx::Link *link);
		int listAccels();
		int listBuffers();
		int listLinks();
		std::string jsonAccels(bool withDetail = false);
		std::string jsonBuffers(bool withDetail = false);
		std::string jsonLinks(bool withDetail = false);
		std::string toJson(bool withDetail = false);
		int setDeleteFlag(bool deleteFlag);
		bool getDeleteFlag() const;
		static inline bool staticGetDeleteFlag(Graph *graph) {
			return graph->getDeleteFlag();
		}
		
	private:
		std::string m_name;
		int id;
		std::string strid;
		std::vector<opendfx::Accel *> accels;
		std::vector<opendfx::Buffer *> buffers;
		std::vector<opendfx::Link *> links;
		bool deleteFlag;
	};
} // #end of graph
#endif // GRAPH_HPP_
