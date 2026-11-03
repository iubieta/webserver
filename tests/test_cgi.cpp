#include "../inc/ConfigParser.hpp"
#include "Tester.hpp"
#include <map>

void	testCgiWithLocalRoot()
{
	ConfigParse							config(
		"tests/fixtures/config/cgi_valid_local_root.config");
	const std::vector<ServerConfig>		*servers;
	const LocationConfig				*location;
	std::map<std::string, std::string>	handlers;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	location = &servers->at(0).getLocation(0);
	handlers = location->getCgiHandlers();
	ASSERT_EQ(location->getRoot(), std::string("/var/www"));
	ASSERT_EQ(handlers.count(".py"), static_cast<size_t>(1));
	ASSERT_EQ(handlers[".py"], std::string("/usr/bin/python3"));
}

void	testCgiWithInheritedRoot()
{
	ConfigParse							config(
		"tests/fixtures/config/cgi_valid_inherited_root.config");
	const std::vector<ServerConfig>		*servers;
	const LocationConfig				*location;
	std::map<std::string, std::string>	handlers;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	location = &servers->at(0).getLocation(0);
	handlers = location->getCgiHandlers();
	ASSERT_EQ(location->getRoot(), std::string("/var/www"));
	ASSERT_EQ(handlers.count(".py"), static_cast<size_t>(1));
	ASSERT_EQ(handlers[".py"], std::string("/usr/bin/python3"));
}

void	testCgiWithoutRoot()
{
	ConfigParse	config(
		"tests/fixtures/config/cgi_missing_root.config");

	config.fileToken();
	ASSERT_EQ(config.parseManager(), -1);
}

void	testCgiWithAlias()
{
	ConfigParse	config(
		"tests/fixtures/config/cgi_with_alias.config");

	config.fileToken();
	ASSERT_EQ(config.parseManager(), -1);
}

void	testCgiWithReturn()
{
	ConfigParse	config(
		"tests/fixtures/config/cgi_with_return.config");

	config.fileToken();
	ASSERT_EQ(config.parseManager(), -1);
}

void	testDuplicateCgiExtension()
{
	ConfigParse	config(
		"tests/fixtures/config/cgi_duplicate_extension.config");

	config.fileToken();
	ASSERT_EQ(config.parseManager(), -1);
}

void	cgi_tests()
{
	Tester::runTest("CGI with local root", testCgiWithLocalRoot);
	Tester::runTest("CGI with inherited root", testCgiWithInheritedRoot);
	Tester::runTest("CGI without root", testCgiWithoutRoot);
	Tester::runTest("CGI with alias", testCgiWithAlias);
	Tester::runTest("CGI with return", testCgiWithReturn);
	Tester::runTest(
	"Duplicate CGI extension", testDuplicateCgiExtension);
	Tester::report();
}