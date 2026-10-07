#ifndef ORIGIN_FORM
# define ORIGIN_FORM

#include "RequestLine.hpp"
#include <vector>

class OriginForm: public RequestLine
{

	public:

	int requestLine(const std::vector<std::string> &token);
	void requestTarget(const std::string &target);
	~OriginForm();
};

#endif