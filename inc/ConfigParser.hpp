#ifndef CONFIG_PARSE_HPP
#define CONFIG_PARSE_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "ServerConfig.hpp"

class ConfigParse
{
	private:
		std::string				file_name_;
		std::vector<std::string>	token_;
		std::vector<ServerConfig>	server_;

		void		ftTokenize(const std::string &line);
		bool		isDelimiter(char c) const;
		bool		isBrace(char c) const;
		bool		endSemicolon(const std::string &token) const;
		std::string	clearSemicolon(const std::string &token) const;
		int			serverBlocks(size_t &index);
		int			locationBlocks(size_t &index,
						ServerConfig &server);

	public:
		ConfigParse(const std::string &file_name);
		ConfigParse(const ConfigParse &other);
		ConfigParse &operator=(const ConfigParse &other);
		~ConfigParse();

		void	fileToken();
		int		parseManager();

		const std::vector<std::string>	&getTokens() const;
		const std::vector<ServerConfig>	&getServers() const;
};

#endif