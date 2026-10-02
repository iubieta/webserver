#include "../inc/OriginForm.hpp"
#include "../inc/log_global.hpp"
#include <iostream>


int OriginForm::requestLine(const std::string &buffer)
{
	const std::string &tempRequest = getRequest();

	if(tokenLine(buffer) == -1)
		return (-1);
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
	
	
	if (tempRequest[0] != '/')
	{
		ft_log::global().info(
			"request: invalid target", __FILE__, __LINE__);
		return (-1);
	}
	
	std::cout << "method: "<<getMethod() << std::endl;
	std::cout << "target: "<<getRequest() << std::endl;
	std::cout << "version: "<<getVersion() <<std::endl;
	return 0;
}

OriginForm::~OriginForm(){}

