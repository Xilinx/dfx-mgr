// buffer.hpp
#ifndef BUFFER_HPP_
#define BUFFER_HPP_

#include <string>

namespace opendfx {

class Buffer {

	public:
		explicit Buffer(const std::string &name, int parentGraphId);
		explicit Buffer(const std::string &name, int parentGraphId, int id);
		std::string info() const;
		std::string getName() const;
		int addLinkRefCount();
		int subsLinkRefCount();
		int getLinkRefCount();
		inline bool operator==(const Buffer& rhs) const {
		    return (this->id == rhs.id);
		}
		int setDeleteFlag(bool deleteFlag);
		bool getDeleteFlag() const;
		static inline bool staticGetDeleteFlag(Buffer *buffer) {
			return buffer->getDeleteFlag();
		}
		inline int getId() const { return this->id; }
		std::string toJson(bool withDetail = false);
		inline int setBSize(int bSize){
			this->bSize = bSize;
			return 0;
		}
		inline int setBType(int bType){
			this->bType = bType;
			return 0;
		}
	private:
		std::string name;
		int id;
		int parentGraphId;
		int linkRefCount;
		bool deleteFlag;
		int bSize;
		int bType;
	};
} // #end of wrapper
#endif // BUFFER_HPP_
