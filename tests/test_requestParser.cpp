#include "../inc/HttpParser.hpp"
#include "Tester.hpp"
#include <iostream>


void	testOriginFormRequestTarget()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"GET /search?name=yuliano HTTP/1.1\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_HEADERS);
	ASSERT_EQ(request.getRequest(),
		std::string("/search?name=yuliano"));
	ASSERT_EQ(request.getPath(), std::string("/search"));
	ASSERT_EQ(request.getQuery(), std::string("name=yuliano"));
	ASSERT_EQ(request.getSchema(), std::string(""));
	ASSERT_EQ(request.getAuthority(), std::string(""));
}

void	testAbsoluteFormRequestTarget()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"GET http://localhost:8080/search?name=yuliano HTTP/1.1\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_HEADERS);
	ASSERT_EQ(
		request.getRequest(),
		std::string(
			"http://localhost:8080/search?name=yuliano"));
	ASSERT_EQ(request.getSchema(), std::string("http"));
	ASSERT_EQ(
		request.getAuthority(),
		std::string("localhost:8080"));
	ASSERT_EQ(request.getPath(), std::string("/search"));
	ASSERT_EQ(request.getQuery(), std::string("name=yuliano"));
}

void	testFragmentInRequestTarget()
{
	{
		Request			request;
		HttpParser		parser;
		Buffer			buffer;
		e_status_code	status;

		buffer.append(
			"GET /index.html#section HTTP/1.1\r\n"
		);

		status = parser.stateHandler(buffer, request);

		ASSERT_EQ(status, BAD_REQUEST);
		ASSERT_EQ(request.getStatus(), BAD_REQUEST);
	}

	{
		Request			request;
		HttpParser		parser;
		Buffer			buffer;
		e_status_code	status;

		buffer.append(
			"GET http://localhost:8080/index.html#section"
			" HTTP/1.1\r\n"
		);

		status = parser.stateHandler(buffer, request);

		ASSERT_EQ(status, BAD_REQUEST);
		ASSERT_EQ(request.getStatus(), BAD_REQUEST);
	}
}

void	testInvalidRequestTargetForm()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"GET index.html HTTP/1.1\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
	ASSERT_EQ(request.getStatus(), BAD_REQUEST);
}

void test_requestParser()
{
	Tester::runTest(
		"Valid origin-form request-target",
		testOriginFormRequestTarget);
	Tester::runTest(
		"Valid absolute-form request-target",
		testAbsoluteFormRequestTarget);
	Tester::runTest(
		"Reject fragment in request-target",
		testFragmentInRequestTarget);
	Tester::runTest(
		"Reject unknown request-target form",
		testInvalidRequestTargetForm);
}