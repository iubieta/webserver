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
		std::string					target_;
		std::string					root_;
		bool						autoindex_;
		std::vector<std::string>	allow_methods_;
		std::vector<std::string>	index_;
		size_t						client_max_body_size_;
		std::string					alias_;
		//std::string					return_;
		//std::vector<std::string>	cgi_path;
		//std::vector<std::string>	cgi_ext;
		
	public:

		LocationConfig();
		LocationConfig(const LocationConfig &other);
		LocationConfig &operator=(const LocationConfig &other);
		~LocationConfig();

		bool operator==(const LocationConfig &other) const;

		const std::string &getRoot() const;
		bool getAutoindex() const;
		size_t getClientMax() const;
		const std::vector<std::string> &getIndexs() const;
		const std::vector<std::string> &getMethods() const;
		const std::string &getIndex(size_t index) const;
		const std::string &getMethod(size_t index) const;
		const std::string &getTarget() const;
		const std::string &getAlias() const;

		int setRoot(const std::string &root);
		int setAutoindex(const std::string &autoindex);
		int setAutoindex(bool autoindex);
		int	setClientMax(const std::string &client_max);
		int setClientMax(size_t clienMax);
		void addIndex(const std::string &index);
		int addMethod(const std::string &method);
		int setVecIndex(const std::vector<std::string> &index);
		int setTarget(const std::string &target);
		int setAlias(const std::string &alias) ;
};

#endif // !SERVER_CONFIG_HPP
