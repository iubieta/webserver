// buffer_test.cpp
//
// tester of buffer class 
// -----------------------------------------------------------------------------

#include "Tester.hpp"
#include "../inc/Buffer.hpp"
#include <cstring>

// Test 1: Constructor
void testConstructor() {
	Buffer	buff;
	ASSERT_EQ(buff.data(), std::string(""));
	ASSERT_EQ(buff.size(), 0);
	ASSERT_EQ(buff.empty(), true);
}

// Test 2: Data
void testData() {
	Buffer buff;
	ASSERT_EQ(buff.data(), std::string(""));
	buff.append("TEST");
	ASSERT_EQ(buff.data(), std::string("TEST"));
	const char *data_cp = buff.data();
	ASSERT_EQ((data_cp == buff.data()), true);
	ASSERT_EQ(data_cp, std::string("TEST"));
	// Constness
	data_cp = "MOD";
	ASSERT_EQ(data_cp, std::string("MOD"));
	ASSERT_EQ(buff.data(), std::string("TEST"));
	std::string data_str(buff.data());
	data_str = "MOD";
	ASSERT_EQ(data_str, std::string("MOD"));
	ASSERT_EQ(buff.data(), std::string("TEST"));
}

// Test 3: Copy
void testCopy() {
	Buffer	buff;
	Buffer	buff_cp = buff;
	unsigned int ret = memcmp(buff.data(), buff_cp.data(), buff.size());
	ASSERT_EQ(ret, 0);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);

	buff.append("TEST");
	ret = memcmp(buff.data(), buff_cp.data(), buff.size());
	ASSERT_NOT_EQ(ret, 0);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);
	buff_cp = buff;
	ret = memcmp(buff.data(), buff_cp.data(), buff.size());
	ASSERT_EQ(ret, 0);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);

	buff.consume(4);
	ret = memcmp(buff.data(), buff_cp.data(), buff_cp.size());
	ASSERT_NOT_EQ(ret, 0);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);
	buff_cp = buff;
	ret = memcmp(buff.data(), buff_cp.data(), buff.size());
	ASSERT_EQ(ret, 0);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);
}


// Test 4: Size
void testSize() {
	Buffer buff;
	ASSERT_EQ(buff.size(), 0);
	buff.append("TEST");
	ASSERT_EQ(buff.size(), 4);
	buff.append("TEST");
	ASSERT_EQ(buff.size(), 8);
}

// Test 5: empty
void testEmpty() {
	Buffer buff;
	ASSERT_EQ(buff.empty(), true);
	buff.append("TEST");
	ASSERT_EQ(buff.empty(), false);
}

// Test 6: append string
void testStringAppend() {
	Buffer buff;
	unsigned int diff;
	std::string str("TEST");
	buff.append(str);
	ASSERT_EQ(buff.empty(), false);
	ASSERT_EQ(buff.size(), 4);
	diff = memcmp(buff.data(), "TEST", 4);
	ASSERT_EQ(diff, 0);
	str = "AB\0CD";
	buff.append(str);
	ASSERT_EQ(buff.size(), 6);
	diff = memcmp(buff.data(), "TESTAB", 6);
	ASSERT_EQ(diff, 0);
	str = "ñ";
	buff.append(str);
	ASSERT_EQ(buff.size(), 8);
	diff = memcmp(buff.data(), "TESTABñ", 8);
	ASSERT_EQ(diff, 0);
}

// Test 7: append char*
void testCharAppend() {
	unsigned int diff;
	
	{
		Buffer buff;
		// Basic append
		std::string str("TEST");
		buff.append(str);
		ASSERT_EQ(buff.empty(), false);
		ASSERT_EQ(buff.size(), 4);
		diff = memcmp(buff.data(), "TEST", 4);

		// 0 Char append
		ASSERT_EQ(diff, 0);
		buff.append(str.c_str(), 0);
		ASSERT_EQ(buff.empty(), false);
		ASSERT_EQ(buff.size(), 4);
		diff = memcmp(buff.data(), "TEST", 4);
		ASSERT_EQ(diff, 0);
	}
	// \0 char
	{
		Buffer buff;
		std::string literal_str("AB\0CD");
		std::string str("AB\0CD", 5);
		// \0 Char: str append only appends first 2 char
		buff.append(literal_str);
		ASSERT_EQ(buff.size(), 2);
		diff = memcmp(buff.data(), "AB", 2);
		ASSERT_EQ(diff, 0);
		
		// \0 Char: char* appends n bytes
		buff.append(str.c_str(), 5);
		ASSERT_EQ(buff.size(), 7);
		diff = memcmp(buff.data(), "ABAB\0CD", 7);
		ASSERT_EQ(diff, 0);
	}
	// Consecutive \0 chars
	{
		Buffer buff;
		std::string str("\0\0\0\0", 4);
		buff.append(str.c_str(), 4);
		ASSERT_EQ(buff.size(), 4);
		diff = memcmp(buff.data(), "\0\0\0\0", 4);
		ASSERT_EQ(diff, 0);
	}
	// TODO: test long char ???
	{
		Buffer buff;
		std::string str("ñ");
		buff.append(str.c_str(), 2);
		ASSERT_EQ(buff.size(), 2);
		diff = memcmp(buff.data(), "ñ", 2);
		ASSERT_EQ(diff, 0);
	} 
	// \n chars
	{
		Buffer buff;
		std::string str("\n\0", 2);
		buff.append(str);
		ASSERT_EQ(buff.size(), 2);
		diff = memcmp(buff.data(), "\n\0", 2);
		ASSERT_EQ(diff, 0);
	}
}

// Test 8: append string as char*
void testStrCharAppend() {
	Buffer buff;
	std::string str("TEST");
	buff.append(str.c_str(), 4);
	ASSERT_EQ(buff.empty(), false);
	ASSERT_EQ(buff.size(), 4);
	ASSERT_EQ((buff.data() == std::string("TEST")), true);
}

// Test 8: consume
void testConsume() {
	unsigned int diff;
	Buffer buff;
	std::string str("TEST: long text to consume part by part\n\
	\0\0Line 1: empty\n\
	\0\0Line 2: Hello world\n\
	", 80);
	buff.append(str.c_str(), 80);
	ASSERT_EQ(buff.size(), 80);
	diff = memcmp(buff.data(), str.c_str(), 80);
	ASSERT_EQ(diff, 0);

	// 0 bytes
	buff.consume(0);
	ASSERT_EQ(buff.size(), 80);
	// std::cout.write(buff.data(), 80);
	// std::cout << std::endl;
	diff = memcmp(buff.data(), str.c_str(), 80);
	ASSERT_EQ(diff, 0);

	// 1 bytes
	buff.consume(1);
	ASSERT_EQ(buff.size(), 79);
	// std::cout.write(buff.data(), 79);
	// std::cout << std::endl;
	diff = memcmp(buff.data(), &str.c_str()[1], 79);
	ASSERT_EQ(diff, 0);
	
	// n bytes
	buff.consume(39);
	ASSERT_EQ(buff.size(), 40);
	// std::cout.write(buff.data(), 40);
	// std::cout << std::endl;
	diff = memcmp(buff.data(), &str.c_str()[40], 40);
	ASSERT_EQ(diff, 0);
	
	// n bytes > size
	buff.consume(100);
	ASSERT_EQ(buff.size(), 0);
	// std::cout << buff.data();
	// std::cout << std::endl;
	ASSERT_EQ(buff.empty(), true);
	diff = memcmp(buff.data(), "\0", 1);
	ASSERT_EQ(diff, 0);
}

// Test 9: NULL chars
// Test 10: 
//

void buffer_tests() {
	Tester::runTest("Constructor:", testConstructor);
	Tester::runTest("Data:", testData);
	Tester::runTest("Copy:", testCopy);
	Tester::runTest("Size:", testSize);
	Tester::runTest("Empty:", testEmpty);
	Tester::runTest("String Append:", testStringAppend);
	Tester::runTest("Char* Append:", testCharAppend);
	Tester::runTest("Str Char* Append:", testStrCharAppend);
	Tester::runTest("Consume:", testConsume);
}
