// LocationConfig.hpp
//
// Class that stocks the configuration parameters of the server
// ----------------------------------------------------------------------------

#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include "string"
#include "vector"
#include <sys/types.h>

class	LocationConfig 
{
	private:
		//std::string					path;
		std::string					root_;
		bool						autoindex_;
		std::vector<std::string>	index_;
		std::vector<std::string>	allow_methods_;
		size_t						client_max_body_size_;
		//std::string					return_;
		//std::string					alias;
		//std::vector<std::string>	cgi_path;
		//std::vector<std::string>	cgi_ext;
	public:

	LocationConfig();
	LocationConfig(const LocationConfig &other);
	LocationConfig &operator=(const LocationConfig &other);
	~LocationConfig();

	const std::string &getRoot() const;
	bool getAutoindex() const;
	size_t getClientMax() const;
	const std::vector<std::string> &getIndexs() const;
	const std::vector<std::string> &getMethods() const;
	const std::string &getIndex(size_t index) const;
	const std::string &getMethod(size_t index) const;

	int setRoot(const std::string root);
	void setAutoindex(bool autoindex);
	int	setClientMax(const std::string &client_max);
	void addIndex(const std::string &index);
	int addMethod(const std::string &method);


};

#endif // !SERVER_CONFIG_HPP
