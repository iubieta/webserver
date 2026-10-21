#include "../inc/HttpParser.hpp"
#include "Tester.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <string>

void print(const std::map<std::string, std::string> &headers)
{
	std::map<std::string, std::string>::const_iterator	it;

	for (it = headers.begin(); it != headers.end(); ++it)
		std::cout << it->first << ":" << it->second << std::endl;
}

void badRequest()
{
	{
		Request			request;
		HttpParser		parser;
		Buffer			buffer;
		e_status_code	status;

		buffer.append("G(ET / HTTP/1.1\r\n");
		status = parser.stateHandler(buffer, request);

		ASSERT_EQ(status, BAD_REQUEST);
		ASSERT_EQ(request.getStatus(), BAD_REQUEST);
	}

	{
		Request			request;
		HttpParser		parser;
		Buffer			buffer;
		e_status_code	status;

		buffer.append("GET HTTP/1.1\r\n");
		status = parser.stateHandler(buffer, request);

		ASSERT_EQ(status, BAD_REQUEST);
		ASSERT_EQ(request.getStatus(), BAD_REQUEST);
	}

	{
		Request			request;
		HttpParser		parser;
		Buffer			buffer;
		e_status_code	status;

		buffer.append("GET / HTP/1.1\r\n");
		status = parser.stateHandler(buffer, request);

		ASSERT_EQ(status, BAD_REQUEST);
		ASSERT_EQ(request.getStatus(), BAD_REQUEST);
	}

	{
		Request			request;
		HttpParser		parser;
		Buffer			buffer;
		e_status_code	status;

		buffer.append("GET index.html HTTP/1.1\r\n");
		status = parser.stateHandler(buffer, request);

		ASSERT_EQ(status, BAD_REQUEST);
		ASSERT_EQ(request.getStatus(), BAD_REQUEST);
	}
}

void notImplemented()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append("BREW / HTTP/1.1\r\n");
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, NOT_IMPLEMENTED);
	ASSERT_EQ(request.getStatus(), NOT_IMPLEMENTED);
}

void versionNotSupported()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append("GET / HTTP/9.9\r\n");
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, VERSION_NOT_SUPPORTED);
	ASSERT_EQ(request.getStatus(), VERSION_NOT_SUPPORTED);
}

void uriTooLong()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;
	std::string		target;

	target = "GET /";
	target.append(MAX_REQUEST_LINE_SIZE, 'a');

	buffer.append(target);
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, URI_TOO_LONG);
	ASSERT_EQ(request.getStatus(), URI_TOO_LONG);
}

void headerLineTooLarge()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;
	std::string		header;

	buffer.append("GET / HTTP/1.1\r\n");
	status = parser.stateHandler(buffer, request);
	ASSERT_EQ(status, ESTATE_HEADERS);

	header = "X-Test: ";
	header.append(MAX_HEADER_LINE_SIZE, 'a');

	buffer.append(header);
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, HEADERS_TOO_LARGE);
	ASSERT_EQ(request.getStatus(), HEADERS_TOO_LARGE);
}

void totalHeadersTooLarge()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;
	std::string		header_value;

	header_value.assign(7000, 'a');

	buffer.append("GET / HTTP/1.1\r\n");
	buffer.append("Host: localhost:8080\r\n");

	buffer.append("X-Test-One: ");
	buffer.append(header_value);
	buffer.append("\r\n");

	buffer.append("X-Test-Two: ");
	buffer.append(header_value);
	buffer.append("\r\n");

	buffer.append("X-Test-Three: ");
	buffer.append(header_value);
	buffer.append("\r\n");

	buffer.append("X-Test-Four: ");
	buffer.append(header_value);
	buffer.append("\r\n");

	buffer.append("X-Test-Five: ");
	buffer.append(header_value);
	buffer.append("\r\n");

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, HEADERS_TOO_LARGE);
	ASSERT_EQ(request.getStatus(), HEADERS_TOO_LARGE);
}

void tooManyHeaders()
{
	Request				request;
	HttpParser			parser;
	Buffer				buffer;
	e_status_code		status;
	std::ostringstream	number;
	std::string			header;
	size_t				i;

	buffer.append("GET / HTTP/1.1\r\n");
	buffer.append("Host: localhost:8080\r\n");

	i = 0;
	while (i < MAX_HEADER_COUNT)
	{
		number.str("");
		number.clear();
		number << i;

		header = "X-Test-";
		header += number.str();
		header += ": value\r\n";
		buffer.append(header);
		++i;
	}

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, HEADERS_TOO_LARGE);
	ASSERT_EQ(request.getStatus(), HEADERS_TOO_LARGE);
}

void incompleteRequestLine()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append("GET /index");
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_LINE);

	buffer.append(".html?name=yuliano HTTP/1.1\r\n");
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_HEADERS);
	ASSERT_EQ(request.getMethod(), std::string("GET"));
	ASSERT_EQ(request.getPath(), std::string("/index.html"));
	ASSERT_EQ(request.getQuery(), std::string("name=yuliano"));
	ASSERT_EQ(request.getVersion(), std::string("HTTP/1.1"));
}

void successfulRequestByChunks()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append("GET /ind");
	status = parser.stateHandler(buffer, request);
	ASSERT_EQ(status, ESTATE_LINE);

	buffer.append("ex.html?name=yuliano HTTP/1.1\r\nHo");
	status = parser.stateHandler(buffer, request);
	ASSERT_EQ(status, ESTATE_HEADERS);

	buffer.append("st: localhost:8080\r\nContent-Ty");
	status = parser.stateHandler(buffer, request);
	ASSERT_EQ(status, ESTATE_HEADERS);

	buffer.append("pe: text/plain\r\nConnection: close\r\n\r\n");
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getMethod(), std::string("GET"));
	ASSERT_EQ(request.getRequest(),
		std::string("/index.html?name=yuliano"));
	ASSERT_EQ(request.getPath(), std::string("/index.html"));
	ASSERT_EQ(request.getQuery(), std::string("name=yuliano"));
	ASSERT_EQ(request.getVersion(), std::string("HTTP/1.1"));

	print(request.getHeaders());
}

void	test_httpParser()
{
	Tester::runTest("Bad request", badRequest);
	Tester::runTest("Method not implemented", notImplemented);
	Tester::runTest("HTTP version not supported", versionNotSupported);
	Tester::runTest("Request URI too long", uriTooLong);
	Tester::runTest("Header line too large", headerLineTooLarge);
	Tester::runTest("Total headers too large", totalHeadersTooLarge);
	Tester::runTest("Too many headers", tooManyHeaders);
	Tester::runTest("Incomplete request line", incompleteRequestLine);
	Tester::runTest("Successful request by chunks",
		successfulRequestByChunks);
	Tester::report();
}