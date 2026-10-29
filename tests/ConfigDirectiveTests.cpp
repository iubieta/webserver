#include "../inc/ConfigParser.hpp"
#include "Tester.hpp"

static int	parseConfig(const std::string &path)
{
	ConfigParse	config(path);

	config.fileToken();
	return (config.parseManager());
}

void	testInheritedRoot()
{
	ConfigParse							config(
		"tests/fixtures/config/valid_inherited_root.config");
	const std::vector<ServerConfig>		*servers;
	const LocationConfig				*location;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	ASSERT_EQ(servers->size(), 1);
	location = &servers->at(0).getLocation(0);
	ASSERT_EQ(location->getRoot(), "/var/www/html");
	ASSERT_EQ(location->getRootSet(), false);
}

void	testLocalRoot()
{
	ConfigParse							config(
		"tests/fixtures/config/valid_local_root.config");
	const std::vector<ServerConfig>		*servers;
	const LocationConfig				*location;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	// Stores the address of the first location from the first parsed server.
	location = &servers->at(0).getLocation(0);
	ASSERT_EQ(location->getRoot(), "/data");
	ASSERT_EQ(location->getRootSet(), true);
}

void	testValidAlias()
{
	ConfigParse							config(
		"tests/fixtures/config/valid_alias.config");
	const std::vector<ServerConfig>		*servers;
	const LocationConfig				*location;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	location = &servers->at(0).getLocation(0);
	ASSERT_EQ(location->getTarget(), "/images/");
	ASSERT_EQ(location->getAlias(), "/data/photos/");
	ASSERT_EQ(location->getAliasSet(), true);
	ASSERT_EQ(location->getRootSet(), false);
	ASSERT_EQ(location->getRoot(), "");
}

void	testValidReturn()
{
	ConfigParse							config(
		"tests/fixtures/config/valid_return.config");
	const std::vector<ServerConfig>		*servers;
	const LocationConfig				*location;
	const t_return						*redirect;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	location = &servers->at(0).getLocation(0);
	redirect = &location->getReturn();
	ASSERT_EQ(location->getReturnSet(), true);
	ASSERT_EQ(redirect->code, 301);
	ASSERT_EQ(redirect->destination, "/new");
	ASSERT_EQ(location->getRoot(), "");
}

void	testRootAliasConflict()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/invalid_root_alias.config"), -1);
}

void	testAliasRootConflict()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/invalid_alias_root.config"), -1);
}

void	testRootReturnConflict()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/invalid_root_return.config"), -1);
}

void	testReturnAliasConflict()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/invalid_return_alias.config"), -1);
}

void	testDuplicateReturn()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/invalid_duplicate_return.config"), -1);
}

void	testInvalidReturnCode()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/invalid_return_code.config"), -1);
}

void	testInvalidReturnDestination()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/invalid_return_destination.config"), -1);
}

void	testGlobalDirectivesAfterLocation()
{
	ConfigParse							config(
		"tests/fixtures/config/"
		"valid_globals_after_location.config");
	const std::vector<ServerConfig>		*servers;
	const LocationConfig				*location;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	ASSERT_EQ(servers->size(), 1);
	// Stores the address of the first location from the first parsed server.
	location = &servers->at(0).getLocation(0);
	ASSERT_EQ(location->getRoot(), "/var/www/html");
	ASSERT_EQ(location->getRootSet(), false);
	ASSERT_EQ(location->getAutoindex(), true);
	ASSERT_EQ(location->getAutoindexSet(), false);
	ASSERT_EQ(location->getClientMax(), static_cast<size_t>(4096));
	ASSERT_EQ(location->getClientMaxbodySet(), false);
	ASSERT_EQ(location->getIndexs().size(), 2);
	ASSERT_EQ(location->getIndex(0), "index.html");
	ASSERT_EQ(location->getIndex(1), "home.html");
}

void	testGlobalDirectivesAroundLocation()
{
	ConfigParse							config(
		"tests/fixtures/config/"
		"valid_globals_around_location.config");
	const std::vector<ServerConfig>		*servers;
	const LocationConfig				*location;

	config.fileToken();
	ASSERT_EQ(config.parseManager(), 0);
	servers = &config.getServers();
	ASSERT_EQ(servers->size(), 1);
	// Stores the address of the first location from the first parsed server.
	location = &servers->at(0).getLocation(0);
	ASSERT_EQ(location->getRoot(), "/srv/www");
	ASSERT_EQ(location->getRootSet(), false);
	ASSERT_EQ(location->getAutoindex(), true);
	ASSERT_EQ(location->getAutoindexSet(), false);
	ASSERT_EQ(location->getClientMax(), static_cast<size_t>(8192));
	ASSERT_EQ(location->getClientMaxbodySet(), false);
	ASSERT_EQ(location->getIndexs().size(), 2);
	ASSERT_EQ(location->getIndex(0), "index.html");
	ASSERT_EQ(location->getIndex(1), "upload.html");
	ASSERT_EQ(location->getMethods().size(), 1);
	ASSERT_EQ(location->getMethod(0), "POST");
}

void	testReturnMissingDestination()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/"
		"invalid_return_missing_destination.config"), -1);
}

void	testReturnMissingSemicolon()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/"
		"invalid_return_missing_semicolon.config"), -1);
}

void	testReturnExtraArgument()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/"
		"invalid_return_extra_argument.config"), -1);
}

void	testAliasMissingSemicolon()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/"
		"invalid_alias_missing_semicolon.config"), -1);
}

void	testAliasMissingTrailingSlash()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/"
		"invalid_alias_missing_trailing_slash.config"), -1);
}

void	testRelativeRoot()
{
	ASSERT_EQ(parseConfig(
		"tests/fixtures/config/"
		"invalid_relative_root.config"), -1);
}

void	config_directive_tests()
{
	Tester::runTest("inherit server root", testInheritedRoot);
	Tester::runTest("use local root", testLocalRoot);
	Tester::runTest("parse alias directive", testValidAlias);
	Tester::runTest("parse return directive", testValidReturn);
	Tester::runTest("reject root and alias", testRootAliasConflict);
	Tester::runTest("reject alias and root", testAliasRootConflict);
	Tester::runTest("reject root and return", testRootReturnConflict);
	Tester::runTest("reject return and alias", testReturnAliasConflict);
	Tester::runTest("reject duplicate return", testDuplicateReturn);
	Tester::runTest("reject invalid return code", testInvalidReturnCode);
	Tester::runTest("reject invalid return target",
		testInvalidReturnDestination);
	Tester::runTest("inherit global directives after location",
		testGlobalDirectivesAfterLocation);
	Tester::runTest("inherit global directives around location",
		testGlobalDirectivesAroundLocation);
	Tester::runTest("reject return without destination",
		testReturnMissingDestination);
	Tester::runTest("reject return without semicolon",
		testReturnMissingSemicolon);
	Tester::runTest("reject return extra argument",
		testReturnExtraArgument);
	Tester::runTest("reject alias without semicolon",
		testAliasMissingSemicolon);
	Tester::runTest("reject alias without trailing slash",
		testAliasMissingTrailingSlash);
	Tester::runTest("reject relative root", testRelativeRoot);
	Tester::report();
}