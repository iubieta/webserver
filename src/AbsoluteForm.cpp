#include "../inc/AbsoluteForm.hpp"
#include "../inc/log_global.hpp"
#include <iostream>

bool AbsoluteForm::isValidAuthority(
	const std::string &authority) const
{
	size_t		colon_pos;
	size_t		i;
	size_t		port;
	size_t		digit;
	std::string	host;
	std::string	port_string;

	if (authority.empty())
		return (false);

	colon_pos = authority.find(':');
	if (colon_pos != std::string::npos)
	{
		if (authority.find(':', colon_pos + 1) != std::string::npos)
			return (false);
		host = authority.substr(0, colon_pos);
		port_string = authority.substr(colon_pos + 1);
	}
	else
		host = authority;

	if (host.empty())
		return (false);

	i = 0;
	while (i < host.size())
	{
		if (!((host[i] >= 'A' && host[i] <= 'Z')
				|| (host[i] >= 'a' && host[i] <= 'z')
				|| (host[i] >= '0' && host[i] <= '9')
				|| host[i] == '.'
				|| host[i] == '-'))
			return (false);

		if (host[i] == '.'
			&& (i == 0 || i + 1 == host.size()
				|| host[i - 1] == '.'))
			return (false);

		if (host[i] == '-'
			&& (i == 0 || i + 1 == host.size()
				|| host[i - 1] == '.'
				|| host[i + 1] == '.'))
			return (false);
		++i;
	}

	if (colon_pos == std::string::npos)
		return (true);

	if (port_string.empty())
		return (false);

	port = 0;
	i = 0;
	while (i < port_string.size())
	{
		if (port_string[i] < '0' || port_string[i] > '9')
			return (false);

		digit = static_cast<size_t>(port_string[i] - '0');
		if (port > (65535 - digit) / 10)
			return (false);

		port = port * 10 + digit;
		++i;
	}

	return (true);
}

int AbsoluteForm::requestTarget(const std::string &target)
{
	size_t schema_pos;
	size_t authority_pos;
	size_t query_pos;
	size_t start;
	size_t end;
	std::string temp;
	std::string authority_temp;
	std::string old_query;
	std::string new_query;
	std::string old_path;
	std::string new_path;

	if (target.empty())
		return -1;
	
	temp = target;
	
	schema_pos = temp.find("://");
	if (schema_pos == std::string::npos)
		return (-1);
	if(temp.substr(0, schema_pos) != "http")
		return -1;

	setSchema(temp.substr(0, schema_pos));
	authority_pos = temp.find_first_of("/?", schema_pos + 3);
	if (authority_pos != std::string::npos &&  
		temp.substr(authority_pos, 1) == "/")
	{
		start = schema_pos + 3;
		end = authority_pos;
		authority_temp = temp.substr(start, end - start);
		if (!isValidAuthority(authority_temp))
			return -1;
		setAuthority(authority_temp);
		query_pos = temp.find('?', authority_pos);
		if (query_pos != std::string::npos) // '?' found
		{
			start = query_pos + 1; // after '?'
			old_query = temp.substr(start);
			if (decodePercentEncoding(old_query, new_query) == -1)
				return -1;
			setQuery(new_query);
			start = authority_pos;
			end = query_pos;
			old_path = temp.substr(start, end - start);
			if(decodePercentEncoding(old_path, new_path) == -1)
				return -1;
			setPath(new_path);
		}
		else
		{
			
			setQuery("");
			old_path = temp.substr(authority_pos);
			if (decodePercentEncoding(old_path, new_path) == -1)
				return -1;
			setPath(new_path);

		}
			
	}
	else if (authority_pos != std::string::npos &&
		temp.substr(authority_pos, 1) == "?") 
	{
		query_pos = authority_pos;
		
		start = schema_pos + 3;
		end = query_pos;
		authority_temp = temp.substr(start, end -start);
		if (!isValidAuthority(authority_temp))
			return -1;
		setAuthority(authority_temp);
		old_query = temp.substr(query_pos + 1);
		if(decodePercentEncoding(old_query, new_query) == -1)
			return -1;
		setQuery(new_query);
		setPath("/");
		
	}
	else
	{
		start = schema_pos + 3;
		authority_temp = temp.substr(start);
		if (!isValidAuthority(authority_temp))
			return -1;
		setAuthority(authority_temp);
		setQuery("");
		setPath("/");

	}
	return 0;

}

int AbsoluteForm::requestLine(const std::vector<std::string> &tokens)
{
	if (tokens.size() != 3)
		return -1;
	
	setMethod(tokens[0]);
	setRequest(tokens[1]);
	setVersion(tokens[2]);

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
	
	
	if (tokens[1].empty() || tokens[1].substr(0, 4) != "http")
	{
		ft_log::global().debug(
			"request: invalid target", __FILE__, __LINE__);
		return (-1);
	}
	if (requestTarget(tokens[1]) == -1)
	{
		ft_log::global().debug(
			"request: invalid target", __FILE__, __LINE__);
		return (-1);
	}
	return 0;
}

AbsoluteForm::~AbsoluteForm(){}