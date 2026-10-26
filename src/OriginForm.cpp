#include "../inc/OriginForm.hpp"
#include "../inc/log_global.hpp"
#include <iostream>

int OriginForm::requestTarget(const std::string &target)
{
	size_t		pos;
	std::string	temp;
	std::string	old_path;
	std::string	new_path;
	std::string	old_query;
	std::string	new_query;

	temp = target;
	setQuery("");

	if (temp.size() == 1)
	{
		setPath(temp);
		return (0);
	}

	pos = temp.find('?');
	if (pos != std::string::npos)
	{
		old_path = temp.substr(0, pos);
		if (decodePercentEncoding(old_path, new_path) == -1)
			return (-1);
		setPath(new_path);

		//Is there at least one character after '?'
		if (pos + 1 < temp.size())
		{
			old_query = temp.substr(pos + 1);
			if (decodePercentEncoding(old_query, new_query) == -1)
				return (-1);
			setQuery(new_query);
		}
	}
	else
	{
		old_path = temp;
		if (decodePercentEncoding(old_path, new_path) == -1)
			return (-1);
		setPath(new_path);
	}

	return (0);
}

int OriginForm::requestLine(const std::vector<std::string> &token)
{
	if (token.size() != 3)
		return -1;
	
	setMethod(token[0]);
	setRequest(token[1]);
	setVersion(token[2]);

	if (!isValidMethod(getMethod()))
	{
		ft_log::global().debug(
			"request: invalid method", __FILE__, __LINE__);
		return (-1);
	}

	if(!isVaildVersion(getVersion()))
	{
		ft_log::global().debug(
			"request: invalid version", __FILE__, __LINE__);
		return (-1);
	}
	
	
	if (token[1].empty() || token[1][0] != '/')
	{
		ft_log::global().debug(
			"request: invalid target", __FILE__, __LINE__);
		return (-1);
	}

	if (requestTarget(token[1]) == -1)
		return -1;
	return 0;
}

OriginForm::~OriginForm(){}

// int main()
// {
// 	RequestLine *r = new OriginForm();

// 	r ->requestLine (" GET   /    HTTP/1.1    \r\n");
// 	std::cout<< "========================="<<std::endl;
// 	r -> requestLine("GET /index?=Yuliano HTTP/1.1\r\n");
// 	std::cout<< "========================="<<std::endl;
// 	r -> requestLine("GET  / HTTP/1.1\r\n     ");
// 	std::cout<< "========================="<<std::endl;
// 	r -> requestLine("GET /		HTTP/1.1     \r\n"); // you have tab
// 	std::cout<< "========================="<<std::endl;
// 	r -> requestLine("   GET   /   HTTP/1.1\r\n");

// 	delete r;
// 	return 0;
// }