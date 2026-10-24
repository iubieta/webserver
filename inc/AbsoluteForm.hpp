# ifndef ABSOLUTE_FORM
#define ABSOLUTE_FORM

#include "RequestLine.hpp"
#include <vector>

class AbsoluteForm: public RequestLine
{
	public:

	bool isValidAuthority(const std::string &authority) const;
	int requestTarget(const std::string &target);
	int requestLine(const std::vector<std::string> &tokens);
	~AbsoluteForm();
};

#endif