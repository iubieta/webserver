#include "../inc/ConfigParser.hpp"
#include <sstream>

ConfigParse::ConfigParse(const std::string &file_name):file_name_(file_name)
{}

ConfigParse::~ConfigParse() {}

bool ConfigParse::isDelimiter(char c)
{
	if (c == ' ' || c == '\t')
		return true;
	return false;
}

void ConfigParse::ftTokenize(const std::string &line)
{
	std::stringstream ss(line);
	std::string token;

	while(ss >> token)
	{
		this->token_.push_back(token);
	}
}

void ConfigParse::fileToken()
{
	std::vector<std::string>::iterator it;

	std::ifstream file(file_name_.c_str());
	std::string line;
	size_t i;

	if(!file.is_open())
	{
		std::cerr <<"Error opening the file\n";
	}

	while(std::getline(file, line))
	{
		i = 0;
		while(isDelimiter(line[i]) && line[i] != '\0')
			i++;
		line = line.substr(i);
		if(line[0] == '#')
			continue;
	
		ftTokenize(line);
	}

	file.close();

	it = token_.begin();
	int j = 0;
	while(it != token_.end())
	{
		std::cout<<"[" << j << "]" << "token->"<< *it << std::endl;
		++it;
		j++;
	}
	std::cout << "index: "<< j << std::endl;
	std::cout << "size: "<< token_.size() << std::endl;

}

int ConfigParse::parseManager()
{

	size_t	i;
	i = 0;
	while(token_[i] == "server" && token_[i + 1] == "{")
	{
		if(serverBlocks(i) == -1)
		{
			std::cerr << "Syntax error in the configuration file\n";
			return -1;
		}
		i++;
	}
	return 0;
}

int ConfigParse::locationBlocks(size_t &index, ConfigServer &server)
{
	std::string request;
	std::string method;
	std::string attribute;
	ConfigLocation location;
	bool flag;
	size_t i;

	if(token_[index][0] != '/')
			return -1;
	request = token_[index++];
	if(!server.root_.empty())
		location.root_ = server.root_;
	if(token_[index++] != "{")
		return -1;

	while(token_[index] != "}" && index < token_.size())
	{
		if (token_[index] == "autoindex")
		{
			index++;
			if (token_[index][token_[index].size() - 1] != ';' &&
					token_[index][token_[index].size()] != '\0' )
				return -1;

			attribute = token_[index].substr(0, token_[index].size() - 1);

			if (attribute == "on")
				location.autoindex_ = true;
			else if(attribute == "off")
				location.autoindex_ = false;
			else
				return -1;
			index++;
		}
		else if(token_[index] == "allow_methods")
		{
			index++;
			i = 0;
			flag = true;
			while(i < 3 && flag)
			{
				if (token_[index][token_[index].size() - 1] == ';')
				{
					if(token_[index][token_[index].size()] != '\0')
						return -1;
					attribute = token_[index].substr(0, token_[index].size() - 1);
					flag = false;
				}
				else 
				{
					attribute = token_[index];
				}
				if(attribute == "GET")
				{
					location.methods_.push_back(attribute);
					index++;
				}
				else if(attribute == "POST")
				{
					location.methods_.push_back(attribute);
					index++;
				}
				else if(attribute == "DELETE")
				{
					location.methods_.push_back(attribute);
					index++;
				}
				else
					break;
				i++;
			}
		}
	}
	if (token_[index] != "}")
		return -1;
	server.location_.push_back(location);
	return 0;
}

int ConfigParse::serverBlocks(size_t &index)
{
	ConfigServer server;
	std::stringstream ss;
	std::string attribute;
	int number;

	index++;
	if(token_[index] != "{")
		return -1;
	
	index++;
	while(token_[index] != "}" && index < token_.size())
	{
		if(token_[index] == "server_name") // string
		{
			index ++;
			if (token_[index][token_[index].size() - 1] != ';' &&
					token_[index][token_[index].size()] != '\0' )
				return -1;

			attribute = token_[index].substr(0, token_[index].size() - 1);
			server.server_name_ = attribute;
			index++;
		}
		else if(token_[index] == "listen") //int
		{
			index++;

			if (token_[index][token_[index].size() - 1] != ';' &&
					token_[index][token_[index].size()] != '\0' )
				return -1;

			ss.str(token_[index]);
			if (ss.fail())
				return -1;
			ss >> number;
			server.listen_ = number;
			index++;
			
		}
		else if(token_[index] == "root") // string
		{
			index ++;
			if (token_[index][token_[index].size() - 1] != ';' &&
					token_[index][token_[index].size()] != '\0' )
				return -1;


			attribute = token_[index].substr(0, token_[index].size() - 1);
			server.root_ = attribute;
			index++;
		}
		else if (token_[index] == "location")
		{
			index ++;
			if (locationBlocks(index, server) == -1)
				return -1;
			index++;
			
		}
		
	}

	if (token_[index] != "}")// esta llave es del server
			return -1;

	this->server.push_back(server);
	
	return 0;
}

int main()
{
	
	ConfigParse conf("../config/default.conf");
	conf.fileToken();
	conf.parseManager();

}