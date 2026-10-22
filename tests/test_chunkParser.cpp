#include "../inc/HttpParser.hpp"
#include "Tester.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <string>

void print(const std::map<std::string, std::string> &headers);

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


void	testChunkedSingleChunk()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"hello\r\n"
		"0\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("hello"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	testChunkedMultipleChunks()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"hello\r\n"
		"6\r\n"
		" world\r\n"
		"0\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("hello world"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	testChunkedHexadecimalSize()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"A\r\n"
		"0123456789\r\n"
		"0\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("0123456789"));
}

void	testChunkedBodyContainingCRLF()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"4\r\n"
		"a\r\nb\r\n"
		"0\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("a\r\nb"));
}

void	testChunkedFragmentedBody()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"he"
	);

	status = parser.stateHandler(buffer, request);
	ASSERT_EQ(status, CHUNK_DATA);

	buffer.append("llo\r");
	status = parser.stateHandler(buffer, request);
	ASSERT_EQ(status, CHUNK_DATA);

	buffer.append("\n0\r\n");
	status = parser.stateHandler(buffer, request);
	ASSERT_EQ(status, CHUNK_END);

	buffer.append("\r\n");
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("hello"));
	ASSERT_EQ(buffer.size(), static_cast<size_t>(0));
}

void	testChunkedFragmentedSizeLine()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"A"
	);

	status = parser.stateHandler(buffer, request);
	ASSERT_EQ(status, ESTATE_BODY);

	buffer.append(
		"\r\n"
		"0123456789\r\n"
		"0\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("0123456789"));
}

void	testChunkedLeavesNextRequest()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;
	std::string		remaining;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"hello\r\n"
		"0\r\n"
		"\r\n"
		"GET /second HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);
	remaining = buffer.substr(0, buffer.size());

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("hello"));
	ASSERT_EQ(
		remaining,
		std::string(
			"GET /second HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			"\r\n"
		)
	);
}

void	testChunkedInvalidSize()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"G\r\n"
		"hello\r\n"
		"0\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
}

void	testChunkedEmptySize()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
}

void	testChunkedInvalidDataTerminator()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"helloXX"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
}

void	testChunkedIncompleteFinalTerminator()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"hello\r\n"
		"0\r\n"
		"\r"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, CHUNK_END);

	buffer.append("\n");
	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, ESTATE_COMPLETE);
	ASSERT_EQ(request.getBody(), std::string("hello"));
}

void	testUnsupportedTransferEncoding()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: gzip\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, NOT_IMPLEMENTED);
}

void	testContentLengthAndTransferEncodingConflict()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 5\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"hello\r\n"
		"0\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, BAD_REQUEST);
}

void	testLengthRequired()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, LENGTH_REQUIRED);
	ASSERT_EQ(request.getStatus(), LENGTH_REQUIRED);
}

void	testContentLengthPayloadTooLarge()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	parser.setMaxbodySize(5);

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 6\r\n"
		"\r\n"
		"abcdef"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, PAYLOAD_TOO_LARGE);
	ASSERT_EQ(request.getStatus(), PAYLOAD_TOO_LARGE);
}

void	testChunkedPayloadTooLarge()
{
	Request			request;
	HttpParser		parser;
	Buffer			buffer;
	e_status_code	status;

	parser.setMaxbodySize(10);

	buffer.append(
		"POST /post-test HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"hello\r\n"
		"6\r\n"
		" world\r\n"
		"0\r\n"
		"\r\n"
	);

	status = parser.stateHandler(buffer, request);

	ASSERT_EQ(status, PAYLOAD_TOO_LARGE);
	ASSERT_EQ(request.getStatus(), PAYLOAD_TOO_LARGE);
}

void test_chunk()
{
	Tester::runTest("Successful request by chunks",
		successfulRequestByChunks);
	Tester::runTest(
		"Chunked single chunk",
		testChunkedSingleChunk);
	Tester::runTest(
		"Chunked multiple chunks",
		testChunkedMultipleChunks);
	Tester::runTest(
		"Chunked hexadecimal size",
		testChunkedHexadecimalSize);
	Tester::runTest(
		"Chunked body containing CRLF",
		testChunkedBodyContainingCRLF);
	Tester::runTest(
		"Chunked fragmented body",
		testChunkedFragmentedBody);
	Tester::runTest(
		"Chunked fragmented size line",
		testChunkedFragmentedSizeLine);
	Tester::runTest(
		"Chunked leaves next request",
		testChunkedLeavesNextRequest);
	Tester::runTest(
		"Chunked invalid size",
		testChunkedInvalidSize);
	Tester::runTest(
		"Chunked empty size",
		testChunkedEmptySize);
	Tester::runTest(
		"Chunked invalid data terminator",
		testChunkedInvalidDataTerminator);
	Tester::runTest(
		"Chunked incomplete final terminator",
		testChunkedIncompleteFinalTerminator);
	Tester::runTest(
		"Unsupported Transfer-Encoding",
		testUnsupportedTransferEncoding);
	Tester::runTest(
		"Content-Length and Transfer-Encoding conflict",
		testContentLengthAndTransferEncodingConflict);
	Tester::runTest(
		"POST without body delimiter returns 411",
		testLengthRequired);
	Tester::runTest(
		"Content-Length over limit returns 413",
		testContentLengthPayloadTooLarge);
	Tester::runTest(
		"Chunked body over limit returns 413",
		testChunkedPayloadTooLarge);
	Tester::report();
}