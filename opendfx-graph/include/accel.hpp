// accel.hpp
#ifndef ACCEL_HPP_
#define ACCEL_HPP_

#include <string>

namespace opendfx {

	enum acceltype {accelNode=0, inputNode=1, outputNode=2, hwAccelNode=3, softAccelNode=4, aiAccelNode=5};

	class Accel {
		public:
			explicit Accel(const std::string &name, std::string &parentGraphId, int type = opendfx::acceltype::accelNode);
			explicit Accel(const std::string &name, std::string &parentGraphId, int type, const std::string &strid);
			std::string info() const;
			std::string getName() const;
			int addLinkRefCount();
			int subsLinkRefCount();
			int getLinkRefCount();
			inline std::string getParentGraphId(){
				return this->parentGraphId;
			}
			inline int setBSize(int bSize){
				this->bSize = bSize;
				return 0;
			}
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
			inline int getType(){
				return type;
			};
			inline int setType(int type){
				this->type = type;
				return 0;
			};
		private:
			std::string name;
			int id;
			int gid;
			int linkRefCount;
			std::string parentGraphId;
			int type;
			std::string strid;
			bool deleteFlag;
			int bSize;
	};

	//inline bool Accel::operator==(const Accel* lhs, const Accel* rhs) {
	//    return (*lhs == *rhs);
	//    //return (lhs->id == rhs->id && this->strid == rhs->strid);
	//}
} // #end of wrapper

#endif // ACCEL_HPP_
