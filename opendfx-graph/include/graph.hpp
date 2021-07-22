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
		Accel* addAccel(Accel *accel);
		Buffer* addBuffer(const std::string &name);
		Buffer* addBuffer(Buffer *buffer);
		Link* addInputBuffer (Accel *accel, Buffer *buffer);
		Link* addOutputBuffer(Accel *accel, Buffer *buffer);
		Link* addLink(Link *link);
		int copyGraph(Graph &graph);
		int cutGraph (Graph &graph);
		int delAccel(Accel *accel);
		int delBuffer(Buffer *buffer);
		int delLink(Link *link);
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
		};
		opendfx::Graph operator + (opendfx::Graph& graph);
		Graph operator - (Graph &graph);
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
