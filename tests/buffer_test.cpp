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
	ASSERT_EQ((*buff.data() == *buff_cp.data()), true);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);

	buff.append("TEST");
	ASSERT_EQ((*buff.data() == *buff_cp.data()), false);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);
	buff_cp = buff;
	ASSERT_EQ((*buff.data() == *buff_cp.data()), true);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);

	buff.consume(4);
	ASSERT_EQ((*buff.data() == *buff_cp.data()), false);
	ASSERT_EQ((buff.data() == buff_cp.data()), false);
	buff_cp = buff;
	ASSERT_EQ((*buff.data() == *buff_cp.data()), true);
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
	std::string str("TEST");
	buff.append(str);
	ASSERT_EQ(buff.empty(), false);
	ASSERT_EQ(buff.size(), 4);
	ASSERT_EQ((buff.data() == std::string("TEST")), true);
	str = "AB\0CD";
	buff.append(str);
	ASSERT_EQ(buff.size(), 6);
	ASSERT_EQ((buff.data() == std::string("TESTAB")), true);
	str = "ñ";
	buff.append(str);
	ASSERT_EQ(buff.size(), 8);
	ASSERT_EQ((buff.data() == std::string("TESTABñ")), true);
}

// Test 7: append char*
void testCharAppend() {
	Buffer buff;
	char str[64];
	std::strcpy(str, "TEST");
	buff.append(str);
	ASSERT_EQ(buff.empty(), false);
	ASSERT_EQ(buff.size(), 4);
	ASSERT_EQ((buff.data() == std::string("TEST")), true);
	buff.append(str, 0);
	ASSERT_EQ(buff.empty(), false);
	ASSERT_EQ(buff.size(), 4);
	ASSERT_EQ((buff.data() == std::string("TEST")), true);
	std::strcpy(str, "AB\0CD");
	buff.append(str);
	ASSERT_EQ(buff.size(), 6);
	ASSERT_EQ((buff.data() == std::string("TESTAB")), true);
	buff.append(str, 5);
	ASSERT_EQ(buff.size(), 11);
	ASSERT_EQ((buff.data() == std::string("TESTABAB\0CD")), true);
	std::strcpy(str, "\0\0\0\0");
	buff.append(str, 4);
	ASSERT_EQ(buff.size(), 15);
	ASSERT_EQ((buff.data() == std::string("TESTABAB\0CD\0\0\0\0")), true);
	std::strcpy(str, "ñ");
	buff.append(str, 1);
	ASSERT_EQ(buff.size(), 16);
	ASSERT_EQ((buff.data() == std::string("TESTABAB\0CD\0\0\0\0ñ")), true);
	std::strcpy(str, "\n\0");
	buff.append(str, 1);
	ASSERT_EQ(buff.size(), 17);
	ASSERT_EQ((buff.data() == std::string("TESTABAB\0CD\0\0\0\0\n")), true);
	std::strcpy(str, "\n\0");
	buff.append(str, 2);
	ASSERT_EQ(buff.size(), 19);
	ASSERT_EQ((buff.data() == std::string("TESTABAB\0CD\0\0\0\0\n\n\0")), true);
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
	Buffer buff;
	char str[1000];
	std::strcpy(str, "TEST: long text to consume part by part\n\
	Line 1: empty\n\
	Line 2: Hello world\n\
	");
	// TODO: null char error -> cout?? 
	buff.append(str, 200);
	std::cout << buff.data();
	// 0 bytes
	// 1 bytes
	// n bytes
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
