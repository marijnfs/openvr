#ifndef __UTIL_REFRACTOR_H__
#define __UTIL_REFRACTOR_H__

#include <string>
#include <fstream>
#include <streambuf>

struct StringException : public std::exception {
	StringException(std::string msg_): msg(msg_){}
	char const* what() const throw() {return msg.c_str();}
	~StringException() throw() {}
	std::string msg;
};


std::string read_all(std::string path) {
	std::ifstream t(path.c_str());
	if (!t)
		throw "failed to open";

	std::string str;

	t.seekg(0, std::ios::end);   
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(t)),
	            std::istreambuf_iterator<char>());
	return str;
}

#endif