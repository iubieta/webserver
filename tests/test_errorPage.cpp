#include "../inc/ConfigParser.hpp"
#include "Tester.hpp"

static int	parseConfig(const std::string &path)
{
	ConfigParse	config(path);

	config.fileToken();
	return (config.parseManager());
}

void	testServerErrorPages()
{
	ConfigParse				config(
		"tests/fixtures/config/error_page_server.config");
	const std::vector<ServerConfig>	*servers;
	const std::string			*page;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	page = servers->at(0).getErrorPage(INTERNAL_ERROR);
	ASSERT_EQ(page != NULL, true);
	ASSERT_EQ(*page, std::string("/errors/50x.html"));
	page = servers->at(0).getErrorPage(BAD_GATEWAY);
	ASSERT_EQ(page != NULL, true);
	ASSERT_EQ(*page, std::string("/errors/50x.html"));
	page = servers->at(0).getErrorPage(SERVICE_UNAVAILABLE);
	ASSERT_EQ(page != NULL, true);
	ASSERT_EQ(*page, std::string("/errors/50x.html"));
}

void	testLocationErrorPage()
{
	ConfigParse				config(
		"tests/fixtures/config/error_page_location.config");
	const std::vector<ServerConfig>	*servers;
	const LocationConfig			*location;
	const std::string			*page;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	location = &servers->at(0).getLocation(0);
	page = location->getErrorPage(FORBIDDEN);
	ASSERT_EQ(page != NULL, true);
	ASSERT_EQ(*page, std::string("/errors/private.html"));
	ASSERT_EQ(location->getErrorPage(NOT_FOUND) == NULL, true);
	ASSERT_EQ(servers->at(0).getErrorPage(FORBIDDEN) == NULL, true);
}

void	testErrorPageInheritance()
{
	ConfigParse				config(
		"tests/fixtures/config/error_page_inheritance.config");
	const std::vector<ServerConfig>	*servers;
	const LocationConfig			*location;
	const std::string			*page;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	location = &servers->at(0).getLocation(0);
	page = location->getErrorPage(NOT_FOUND);
	ASSERT_EQ(page != NULL, true);
	ASSERT_EQ(*page, std::string("/errors/404.html"));
}

void	testInvalidErrorPageCode()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/error_page_invalid_code.config"), -1);
}

void	testDuplicateErrorPageCode()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/error_page_duplicate.config"), -1);
}

void	error_page_config_tests()
{
	Tester::runTest(
		"server error pages", testServerErrorPages);
	Tester::runTest(
		"location error page", testLocationErrorPage);
	Tester::runTest(
		"error page inheritance", testErrorPageInheritance);
	Tester::runTest(
		"invalid error page code", testInvalidErrorPageCode);
	Tester::runTest(
		"duplicate error page code", testDuplicateErrorPageCode);
	Tester::report();
}