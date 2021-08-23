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
		explicit Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, int parentGraphId);
		explicit Link(opendfx::Accel *accel, opendfx::Buffer *buffer, int dir, int parentGraphId, int id);
		int info();
		inline bool operator==(const Link& rhs) const {
		    return (this->id == rhs.id);
		}
		opendfx::Accel* getAccel() const;
		opendfx::Buffer* getBuffer() const;
		int setDeleteFlag(bool deleteFlag);
		bool getDeleteFlag() const;
		static inline bool staticGetDeleteFlag(Link *link) {
			return link->getDeleteFlag();
		}
		inline int getId() const { return this->id; }
		std::string toJson(bool withDetail = false);
		inline int setOffset(int offset){
			this->offset = offset;
			return 0;
		}
		inline int setTransactionSize(int transactionSize){
			this->transactionSize = transactionSize;
			return 0;
		}
		inline int setTransactionIndex(int transactionIndex){
			this->transactionIndex = transactionIndex;
			return 0;
		}
		inline int setChannel(int channel){
			this->channel = channel;
			return 0;
		}
	private:
		opendfx::Accel *accel;
		opendfx::Buffer *buffer;
		int dir;
		int id;
		int parentGraphId;
		bool deleteFlag;
		int offset;
		int transactionSize;
		int transactionIndex;
		int channel;
	};
} 
#endif // LINK_HPP_
