// utils.hpp
#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <string>

namespace utils{
	int setID(int & id, std::string & strid);
	int genID();
	std::string int2str(int id);
	int str2int(std::string strid);
} 


#define _unused(x) ((void)(x))

#endif // UTILS_HPP_
