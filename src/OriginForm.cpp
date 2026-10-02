#include "../inc/OriginForm.hpp"
#include <iostream>


int OriginForm::requestLine(const std::string &buffer)
{
	if(tokenLine(buffer) == -1)
		return (-1);
	std::cout << "method: "<<getMethod() << std::endl;
	std::cout << "target: "<<getRequest() << std::endl;
	std::cout << "version: "<<getVersion() <<std::endl;
	return 0;
}

OriginForm::~OriginForm(){}