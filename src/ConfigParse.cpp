#include "../inc/ConfigParser.hpp"

ConfigParse::ConfigParse(const std::string &file_name):file_name_(file_name)
{}

ConfigParse::ConfigParse(const ConfigParse &other)
	: file_name_(other.file_name_),
	  token_(other.token_),
	  server_(other.server_)
{}

ConfigParse &ConfigParse::operator=(const ConfigParse &other)
{
	if (this != &other)
	{
		file_name_ = other.file_name_;
		token_	= other.token_;
		server_ = other.server_;
	}
	return *this;
}

ConfigParse::~ConfigParse() {}

bool ConfigParse::isDelimiter(char c) const
{
	if (c == ' ' || c == '\t')
		return true;
	return false;
}

bool ConfigParse::isBrace(char c) const
{
	if (c == '{' || c == '}')
		return true;
	return false;
}

bool ConfigParse::endSemicolon(const std::string &token) const
{
	if (token.empty())
		return false;
	if(token[token.size() - 1] == ';')
		return true;
	return false;
}

std::string ConfigParse::clearSemicolon(const std::string &token) const
{
	std::string result;

	result = token.substr(0, token.size() - 1);
	return result;
}

void ConfigParse::ftTokenize(const std::string &line)
{
	std::string token;
	size_t i;
	size_t start;

	i = 0;
	while(i  < line.size())
	{
		while (isDelimiter(line[i]) && line[i] != '\0')
			i++;
		if (i == line.size())
			break ; // avoid empty lines
		start = i;
		if (isBrace(line[i]))
		{
			token = line.substr(start, 1);
			i++;
		}
		else
		{
	
			while (i < line.size() && !isDelimiter(line[i]) 
					&& !isBrace(line[i]))
				i++;
			token = line.substr(start, i - start);
			
		}
		token_.push_back(token);
		
	}
}
void ConfigParse::fileToken()
{
	std::ifstream file(file_name_.c_str());
	std::string line;
	size_t i;

	if(!file.is_open())
	{
		std::cerr <<"Error opening the file\n";
		return ;
	}

	while(std::getline(file, line))
	{
		i = 0;
		while(i < line.size() && isDelimiter(line[i]))
			i++;
		line = line.substr(i);
		if(line.empty() ||line[0] == '#')
			continue;
	
		ftTokenize(line);
	}

	file.close();
	
}

const std::vector<std::string> &ConfigParse::getTokens() const
{
	return this->token_;
}

int ConfigParse::parseManager()
{
	size_t	i;

	if (token_.empty())
		return -1;
	i = 0;
	while (i < token_.size())
	{
		if (token_[i] != "server")
			return -1;
		if (i + 1 >= token_.size() || token_[i + 1] != "{")
			return -1;
		if (serverBlocks(i) == -1)
		{
			std::cerr << "Syntax error in the configuration file\n";
			return -1;
		}
		i++;
	}
	return 0;
}

int ConfigParse::locationBlocks(size_t &index, ServerConfig &server)
{
	std::string temp;
	LocationConfig location;

	if (index >= token_.size() || token_[index].empty()
		|| token_[index][0] != '/')
		return -1;

	if(location.setTarget(token_[index]) == -1)
		return -1;
	index++;

	if(!server.getRoot().empty())
		location.setRoot(server.getRoot());
	if(!server.getIndexs().empty())
		location.setVecIndex(server.getIndexs());
	if(server.getClientMaxBody() > 0)
		location.setClientMax(server.getClientMaxBody());
	if (server.getAutoindex() == true)
		location.setAutoindex(server.getAutoindex());

	if (index >= token_.size() || token_[index] != "{")
		return -1;


	index++;
	while (index < token_.size() && token_[index] != "}")
	{
		if (token_[index] == "autoindex")
		{
			index++;
			if (index >= token_.size()
				|| !endSemicolon(token_[index]))
				return -1;
			temp = clearSemicolon(token_[index]);
			if (location.setAutoindex(temp) == -1)
				return -1;
		}
		else if (token_[index] == "root")
		{
			index++;
			if (index >= token_.size()
				|| !endSemicolon(token_[index]))
				return -1;
			temp = clearSemicolon(token_[index]);
			if (location.setRoot(temp) == -1)
				return -1;
		}
		else if (token_[index] == "allow_methods")
		{
			index++;
			if (index >= token_.size())
				return -1;
			while (index < token_.size()
				&& !endSemicolon(token_[index]))
			{
				if (location.addMethod(token_[index]) == -1)
					return -1;
				index++;
			}
			if (index >= token_.size())
				return -1;
			temp = clearSemicolon(token_[index]);
			if (location.addMethod(temp) == -1)
				return -1;
		}
		else if (token_[index] == "index")
		{
			index++;
			if (index >= token_.size())
				return -1;
			while (index < token_.size()
				&& !endSemicolon(token_[index]))
			{
				location.addIndex(token_[index]);
				index++;
			}
			if (index >= token_.size())
				return -1;
			temp = clearSemicolon(token_[index]);
			if (temp.empty())
				return -1;
			location.addIndex(temp);
		}
		else
			return -1;
		index++;
	}
	if (index >= token_.size() || token_[index] != "}")
		return -1;
	if (location.getMethods().empty())
		location.addMethod("GET");
	server.addLocation(location);
	return 0;
}

int ConfigParse::serverBlocks(size_t &index)
{
	ServerConfig server;
	std::string temp;

	index++;
	if (index >= token_.size() || token_[index] != "{")
		return -1;
	index++;
	while (index < token_.size() && token_[index] != "}")
	{
		if (token_[index] == "server_name")
		{
			index++;
			if (index >= token_.size()
				|| !endSemicolon(token_[index]))
				return -1;
			temp = clearSemicolon(token_[index]);
			if (server.setServerName(temp) == -1)
				return -1;
		}
		else if (token_[index] == "host")
		{
			index++;
			if (index >= token_.size()
				|| !endSemicolon(token_[index]))
				return -1;
			temp = clearSemicolon(token_[index]);
			if (server.setHost(temp) == -1)
				return -1;
		}
		else if (token_[index] == "listen")
		{
			index++;
			if (index >= token_.size()
				|| !endSemicolon(token_[index]))
				return -1;
			temp = clearSemicolon(token_[index]);
			if (server.setListen(temp) == -1)
				return -1;
		}
		else if (token_[index] == "root")
		{
			index++;
			if (index >= token_.size()
				|| !endSemicolon(token_[index]))
				return -1;
			temp = clearSemicolon(token_[index]);
			if (server.setRoot(temp) == -1)
				return -1;
		}
		else if (token_[index] == "autoindex")
		{
			index++;
			if (index >= token_.size()
				|| !endSemicolon(token_[index]))
				return -1;
			temp = clearSemicolon(token_[index]);
			if (server.setAutoindex(temp) == -1)
				return -1;
		}
		else if (token_[index] == "client_max_body_size")
		{
			index++;
			if (index >= token_.size()
				|| !endSemicolon(token_[index]))
				return -1;
			temp = clearSemicolon(token_[index]);
			if (server.setClientMax(temp) == -1)
				return -1;
		}
		else if (token_[index] == "index")
		{
			index++;
			if (index >= token_.size())
				return -1;
			while (index < token_.size()
				&& !endSemicolon(token_[index]))
			{
				server.addIndex(token_[index]);
				index++;
			}
			if (index >= token_.size())
				return -1;
			temp = clearSemicolon(token_[index]);
			if (temp.empty())
				return -1;
			server.addIndex(temp);
		}
		else if (token_[index] == "location")
		{
			index++;
			if (locationBlocks(index, server) == -1)
				return -1;
		}
		else
			return -1;
		index++;
	}
	if (index >= token_.size() || token_[index] != "}")
		return -1;
	this->server_.push_back(server);
	return 0;
	
}

