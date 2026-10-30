// LocationConfig.hpp
//
// Class that stocks the configuration parameters of the server
// ----------------------------------------------------------------------------

#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include "string"
#include "vector"
#include "map"
#include <sys/types.h>

typedef	struct s_return
{
	// TODO: implement return with http status codes not int
	int			code;
	std::string	destination;
	
	s_return():code(0), 
			destination("")
	{}
	
	bool operator==(const s_return &other) const
	{
		return (code == other.code
			&& destination == other.destination);
	}

}	t_return;

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
		bool						root_set_;
		bool						alias_set_;
		bool						autoindex_set_;
		bool						client_max_body_set_;
		t_return					return_;
		bool						return_set_;
		std::map<int, std::string>	error_page_;
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
		bool	getRootSet() const;
		bool	getAliasSet() const;
		bool	getAutoindexSet() const;
		bool	getClientMaxbodySet() const;
		const t_return &getReturn() const;
		bool	getReturnSet() const;
		const	std::string *getErrorPage(int key) const;
		const std::map<int, std::string> &getErrorPages() const;

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
		void setRootSet(bool rootSet);
		void setAliasSet(bool aliasSet);
		void setAutoindexSet(bool autoindexSet);
		void setClientMaxBodySet(bool clientMaxBodySet);
		int setReturn(const std::string &code, const std::string &destination);
		int setReturn(int code, const std::string &destination);
		void setReturnSet(bool returnSet);
		int addErrorPage(const std::string &code, const std::string &page);
		void setErrorPages(const std::map<int, std::string> &error_pages);
};

#endif // !SERVER_CONFIG_HPP
