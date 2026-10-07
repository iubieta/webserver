#include "../inc/OriginForm.hpp"
#include "../inc/log_global.hpp"
#include <iostream>

void OriginForm::requestTarget(const std::string &target)
{
	size_t pos;
	std::string temp;

	temp = target;
	setQuery("");

	if (temp.size() == 1)
	{
		setPath(temp);
		return ;
	}

	pos = temp.find('?');
	if (pos != std::string::npos)
	{
		setPath(temp.substr(0, pos));
		if (temp.substr(pos + 1).size() >= 1)
			setQuery(temp.substr(pos + 1));
	}
	else
		setPath(temp);

}

int OriginForm::requestLine(const std::vector<std::string> &token)
{
	if (token.empty())
		return -1;
	
	setMethod(token[0]);
	setRequest(token[1]);
	setVersion(token[2]);

	if (!isValidMethod(getMethod()))
	{
		ft_log::global().info(
			"request: invalid method", __FILE__, __LINE__);
		return (-1);
	}

	if(!isVaildVersion(getVersion()))
	{
		ft_log::global().info(
			"request: invalid version", __FILE__, __LINE__);
		return (-1);
	}
	
	
	if (token[1].empty() || token[1][0] != '/')
	{
		ft_log::global().info(
			"request: invalid target", __FILE__, __LINE__);
		return (-1);
	}

	requestTarget(token[1]);

	// std::cout << "Method: "<< getMethod() << std::endl;
	// std::cout << "target: "<< getPath() << std:: endl;
	// std::cout << "query: " << getQuery() << std::endl;
	// std::cout << "version: "<< getVersion() << std::endl;


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