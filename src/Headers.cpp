#include "../inc/Headers.hpp"
#include "../inc/log_global.hpp"
#include <cctype>
#include <iostream>

Headers::Headers():headers_() {}

Headers::~Headers(){}


int Headers::addHeader(const std::string &key,const std::string &value)
{
	if (headers_.count(key) != 0)
	{
		ft_log::global().info(
			"Headers: repeated field",
			__FILE__, __LINE__);

		return (-1);
	}
	headers_.insert(std::make_pair(key, value));
	return (0);
}

bool Headers::isInvalidKeyCharacter(char c)
{
	if ((c >= 'A' && c <= 'Z')
		|| (c >= 'a' && c <= 'z')
		|| (c >= '0' && c <= '9'))
		return (false);
	if (c == '!' || c == '#' || c == '$' || c == '%'
		|| c == '&' || c == '\'' || c == '*'
		|| c == '+' || c == '-' || c == '.'
		|| c == '^' || c == '_' || c == '`'
		|| c == '|' || c == '~')
		return (false);
	return (true);
}

bool Headers::isInvalidValueCharacter(char c)
{
	unsigned char	uc;

	uc = static_cast<unsigned char>(c);
	if (uc == ' ' || uc == '\t')
		return (false);
	if (uc >= 33 && uc <= 126)
		return (false);
	if (uc >= 128)
		return (false);
	return (true);
}

bool Headers::isInvalidKey(const std::string &key)
{
	size_t	i;

	if (key.empty())
		return (true);
	i = 0;
	while (i < key.size())
	{
		if (isInvalidKeyCharacter(key[i]))
		{
			ft_log::global().info(
				"Headers: invalid field name",
				__FILE__, __LINE__);
			return (true);
		}
		i++;
	}
	return (false);
}

bool Headers::isInvalidValue(const std::string &value)
{
	size_t	i;

	i = 0;
	while (i < value.size())
	{
		if (isInvalidValueCharacter(value[i]))
		{
			ft_log::global().info(
				"Headers: invalid field value",
				__FILE__, __LINE__);
			return (true);
		}
		i++;
	}
	return (false);
}

void Headers::normalize(std::string &key)
{
	for(size_t i = 0; i < key.size(); i++)
		key[i] = std::tolower(static_cast<unsigned char>(key[i]));
}

const std::string *Headers::getHeader(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = headers_.find(key);

	if (it != headers_.end())
		return &(it->second);
	return NULL;
}

const std::map<std::string, std::string> &Headers::getHeaders() const
{
	return headers_;
}

int Headers::tokenHeader(const std::string &line)
{
	std::string key;
	std::string value;
	std::string temp;
	size_t pos;
	size_t start;

	if (line.empty())
	{
		ft_log::global().info(
			"Headers: empty string", __FILE__, __LINE__);
		return -1;
	}
		
	pos = line.size();
	if (pos < 2 || line[pos - 2] != '\r'
					|| line[pos - 1] != '\n')
	{
		ft_log::global().info(
			"Headers: Incorrect line ending", __FILE__, __LINE__);
		return -1;
	}


	pos = line.find(':');
	if (pos == std::string::npos)
	{
		ft_log::global().info(
			"Headers: separator character not found", __FILE__, __LINE__
		);
		return -1;
	}
	
	key = line.substr(0, pos);
	
	temp = line.substr(pos + 1);

	start = 0;
	while (start < temp.size() && 
		(temp[start] == ' ' || temp[start] == '\t'))
		start++;
	
	pos = temp.size() - 2;
	while(pos > start && 
		(temp[pos - 1] == ' ' || temp[pos -1] == '\t'))
		pos --;
	
	value = temp.substr(start, pos - start);

	if (isInvalidKey(key))
		return (-1);
	
	if (isInvalidValue(value))
		return (-1);

	normalize(key);

	if (addHeader(key, value) == -1)
		return (-1);

	//std::cout << *getHeader(key) << std::endl;
	return 0;
	
}

// int main()
// {
// 	Headers h;
// 	h.tokenHeader("host:  localhost:8080   \r\n");
// 	h.tokenHeader("content-type:text/plain\r\n");
// 	h.tokenHeader("   host:localhost8080\r\n");
// 	h.tokenHeader("host:	localhost:8080\r\n"); // tab
// 	h.tokenHeader("   host: localhost8080\r\n");
// 	h.tokenHeader("   host :localhost8080\r\n");
// 	h.tokenHeader("content-type	:text/plain\r\n"); //tab
// 	h.tokenHeader("content-type:text/plain\r\n     ");

// 	return 0;
// }