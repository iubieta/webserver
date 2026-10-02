#ifndef ORIGIN_FORM
# define ORIGIN_FORM

#include "RequestLine.hpp"

class OriginForm: public RequestLine
{

	public:

	int requestLine(const std::string &buffer);

	~OriginForm();
};

#endif