#include "../inc/ConfigParser.hpp"
#include "Tester.hpp"

void	testBasicTokenization()
{
	ConfigParse				config(
		"tests/fixtures/config/test_basicToken.config");
	

	config.fileToken();
	const std::vector<std::string>	&token = config.getTokens();

	ASSERT_EQ(token.size(), 5);
	ASSERT_EQ(token[0], "server");
	ASSERT_EQ(token[1], "{");
	ASSERT_EQ(token[2], "listen");
	ASSERT_EQ(token[3], "8080;");
	ASSERT_EQ(token[4], "}");
}

void testComplexTokenization()
{
	ConfigParse				config(
		"tests/fixtures/config/test_complexToken.config");

	config.fileToken();
	const std::vector<std::string>	&token = config.getTokens();

	ASSERT_EQ(token.size(), 21);
	ASSERT_EQ(token[0], "server");
	ASSERT_EQ(token[1], "{");
	ASSERT_EQ(token[2], "listen");
	ASSERT_EQ(token[3], "8080;");
	ASSERT_EQ(token[4], "server_name");
	ASSERT_EQ(token[5], "localhost;");
	ASSERT_EQ(token[6], "root");
	ASSERT_EQ(token[7], "/var/www/html;");
	ASSERT_EQ(token[8], "index");
	ASSERT_EQ(token[9], "index.html");
	ASSERT_EQ(token[10], "index.html;");
	ASSERT_EQ(token[11], "location");
	ASSERT_EQ(token[12], "/images");
	ASSERT_EQ(token[13], "{");
	ASSERT_EQ(token[14], "autoindex");
	ASSERT_EQ(token[15], "on;");
	ASSERT_EQ(token[16], "allow_methods");
	ASSERT_EQ(token[17], "GET");
	ASSERT_EQ(token[18], "POST;");
	ASSERT_EQ(token[19], "}");
	ASSERT_EQ(token[20], "}");

}

void testValid()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/test_valid.config");
	config.fileToken();
	result = config.parseManager();
	ASSERT_EQ(result, 0);
}

void testMissingSemicolon()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/test_semicolon_missing.config");
	config.fileToken();
	result = config.parseManager();
	ASSERT_EQ(result, -1);
}

void testMissingOpenBrace()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/test_braces_missing_open.config");
	config.fileToken();
	result = config.parseManager();
	ASSERT_EQ(result, -1);
}

void testMissingCloseBrace()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/test_braces_missing_close.config");
	config.fileToken();
	result = config.parseManager();
	ASSERT_EQ(result, -1);

}

void testMethodSemicolon()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/allow_methods_missing_semicolon.config");
		config.fileToken();
		result = config.parseManager();
		ASSERT_EQ(result, -1);
}

void testMethodUnkwon()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/allow_methods_unknown_value.config");
		config.fileToken();
		result = config.parseManager();
		ASSERT_EQ(result, -1);
}

void testCharacterAfterSemicolon()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/characters_after_semicolon.config");
		config.fileToken();
		result = config.parseManager();
		ASSERT_EQ(result, -1);
}


void testUnknwonDirective()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/unknown_directive.config");
		config.fileToken();
		result = config.parseManager();
		ASSERT_EQ(result, -1);
}

void testUnknwonValue()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/unknown_value.config");
		config.fileToken();
		result = config.parseManager();
		ASSERT_EQ(result, -1);
}

void testValidThreeServers()
{
	int result;

	ConfigParse config(
		"tests/fixtures/config/valid_three_servers.config");
		config.fileToken();
		result = config.parseManager();
		ASSERT_EQ(result, 0);
}

void	config_tests()
{
	Tester::runTest("Basic tokenization", testBasicTokenization);
	Tester::runTest("Complex tokenization", testComplexTokenization);
	Tester::runTest("valid braces and semicolon", testValid);
	Tester::runTest("Missing opening brace", testMissingOpenBrace);
	Tester::runTest("Missing closing brace", testMissingCloseBrace);
	Tester::runTest("allow methods missing semicolon", testMethodSemicolon);
	Tester::runTest("allow methods unknown value", testMethodUnkwon);
	Tester::runTest("characters after semicolon", testCharacterAfterSemicolon);
	Tester::runTest("unknown directive", testUnknwonDirective);
	Tester::runTest("unknown value", testUnknwonValue);
	Tester::runTest("valid three servers", testValidThreeServers);
	Tester::report();
}
