# ifndef CONFIG_PARSE_HPP
#define CONGI_PARSE_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>


typedef struct ConfigLocation
{
	std::string					root_;
	bool						autoindex_;
	std::string					index_;
	std::vector<std::string>	methods_;
	//std::string					return_;

}	ConfiLocation;

typedef struct ConfigServer
{
	int							host_;
	int							listen_;
	bool						autoindex;
	std::string					server_name_;
	std::string					root_;
	std::string					index_;
	std::vector<ConfiLocation>	location_; 

}	ConfigServer;

class ConfigParse
{
	private:

	std::string file_name_;
	std::vector<std::string> token_;
	void ftTokenize(const std::string &line);
	bool isDelimiter(char c);

	std::vector<ConfigServer>		server;
	
	public:

	ConfigParse(const std::string &file_name);
	void fileToken();
	int parseManager();
	int serverBlocks(size_t &index);
	int locationBlocks(size_t &index, ConfigServer &server);
	
	~ConfigParse();

	
};
#endif