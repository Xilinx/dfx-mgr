// link.hpp
#ifndef LINK_HPP_
#define LINK_HPP_

#include <string>
#include "accel.hpp"
#include "buffer.hpp"

namespace opendfx {

	enum direction {toAccel=0, fromAccel=1};

	class Link {

	public:
		explicit Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, std::string parentGraphId);
		explicit Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, std::string parentGraphId, const std::string &strid);
		int info();
		inline bool operator==(const Link& rhs) const {
		    return (this->id == rhs.id && this->strid == rhs.strid);
		}
		opendfx::Accel* getAccel() const;
		opendfx::Buffer* getBuffer() const;
		int setDeleteFlag(bool deleteFlag);
		bool getDeleteFlag() const;
		static inline bool staticGetDeleteFlag(Link *link) {
			return link->getDeleteFlag();
		}
		inline std::string getId() const { return this->strid; }
		std::string toJson(bool withDetail = false);
	private:
		opendfx::Accel *accel;
		opendfx::Buffer *buffer;
		int dir;
		int id;
		std::string parentGraphId;
		std::string strid;
		bool deleteFlag;
	};
} 
#endif // LINK_HPP_
