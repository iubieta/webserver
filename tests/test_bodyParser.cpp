#include "../inc/HttpParser.hpp"
#include "Tester.hpp"
#include <string>

void	bodyComplete()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(parser.getBody(), std::string("hello"));
	ASSERT_EQ(request.getBody(), std::string("hello"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	bodyByChunks()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 11\r\n"
		"\r\n"
		"hello");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_BODY);
	ASSERT_EQ(parser.getBody(), std::string("hello"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));

	buffer.append(" world");
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(parser.getBody(), std::string("hello world"));
	ASSERT_EQ(request.getBody(), std::string("hello world"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	incompleteBody()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 10\r\n"
		"\r\n"
		"hello");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_BODY);
	ASSERT_EQ(parser.getBody(), std::string("hello"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	emptyBody()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 0\r\n"
		"\r\n");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(parser.getBody(), std::string(""));
	ASSERT_EQ(request.getBody(), std::string(""));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	bodyWithCrlf()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 6\r\n"
		"\r\n"
		"a\r\nbcd");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(parser.getBody(), std::string("a\r\nbcd"));
	ASSERT_EQ(request.getBody(), std::string("a\r\nbcd"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	bodyWithRemainingData()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;
	std::string		remaining;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"helloWORLD");

	status = parser.stateHandler(buffer, request);
	remaining = buffer.substr(0, buffer.size());

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("hello"));
	ASSERT_EQ(remaining, std::string("WORLD"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(5));
}

void	bodyFollowedByRequest()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;
	std::string		remaining;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello"
		"GET /next HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"\r\n");

	status = parser.stateHandler(buffer, request);
	remaining = buffer.substr(0, buffer.size());

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("hello"));
	ASSERT_EQ(
		remaining,
		std::string(
			"GET /next HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			"\r\n"));
}

void	invalidContentLengthText()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: cinco\r\n"
		"\r\n"
		"hello");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
	ASSERT_EQ(request.getStatus(), BAD_REQUEST);
}

void	negativeContentLength()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: -1\r\n"
		"\r\n");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
	ASSERT_EQ(request.getStatus(), BAD_REQUEST);
}

void	emptyContentLength()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length:\r\n"
		"\r\n");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
	ASSERT_EQ(request.getStatus(), BAD_REQUEST);
}

void	contentLengthOverflow()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: "
		"999999999999999999999999999999999999999999\r\n"
		"\r\n");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
	ASSERT_EQ(request.getStatus(), BAD_REQUEST);
}

void	requestWithoutBody()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"\r\n");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	test_httpParserBody()
{
	Tester::runTest("Complete body", bodyComplete);
	Tester::runTest("Body received by chunks", bodyByChunks);
	Tester::runTest("Incomplete body", incompleteBody);
	Tester::runTest("Empty body", emptyBody);
	Tester::runTest("Body containing CRLF", bodyWithCrlf);
	Tester::runTest("Body with remaining data", bodyWithRemainingData);
	Tester::runTest("Body followed by another request",
		bodyFollowedByRequest);
	Tester::runTest("Non-numeric Content-Length",
		invalidContentLengthText);
	Tester::runTest("Negative Content-Length",
		negativeContentLength);
	Tester::runTest("Empty Content-Length",
		emptyContentLength);
	Tester::runTest("Content-Length overflow",
		contentLengthOverflow);
	Tester::runTest("Request without body",
		requestWithoutBody);
	Tester::report();
}