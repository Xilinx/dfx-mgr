// accel.hpp
#ifndef ACCEL_HPP_
#define ACCEL_HPP_

#include <string>

namespace opendfx {
	class Accel {
		public:
			explicit Accel(const std::string &name);
			std::string info() const;
			std::string getName() const;
			int addLinkRefCount();
			int subsLinkRefCount();
			int getLinkRefCount();
			inline bool operator==(const Accel& rhs) const {
				return (this->id == rhs.id && this->strid == rhs.strid);
			}
			int setDeleteFlag(bool deleteFlag);
			bool getDeleteFlag() const;
			static inline bool staticGetDeleteFlag(Accel *accel) {
				return accel->getDeleteFlag();
			}
			inline std::string getId() const { return this->strid; }
			std::string toJson(bool withDetail = false);
		private:
			std::string name;
			int id;
			int linkRefCount;
			std::string strid;
			bool deleteFlag;
	};

	//inline bool Accel::operator==(const Accel* lhs, const Accel* rhs) {
	//    return (*lhs == *rhs);
	//    //return (lhs->id == rhs->id && this->strid == rhs->strid);
	//}
} // #end of wrapper

#endif // ACCEL_HPP_
