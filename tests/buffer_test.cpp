// buffer_test.cpp
//
// tester of buffer class 
// -----------------------------------------------------------------------------

#include "Tester.hpp"
#include "../inc/Buffer.hpp"

// Test 1: Constructor
void testConstructor() {
	Buffer	buff();
	ASSERT(buff.data(), "");
	ASSERT(buff.size(), 0);
	ASSERT(buff.empty(), true);
}

// Test 2: Data
void testData() {
	Buffer buff();
	ASSERT(buff.data(), "");
	buff.append("TEST");
	ASSERT(buff.data(), "TEST");
	const char *data_cp = buff.data();
	ASSERT(&data_cp == buff.data(), true);
	ASSERT(data_cp, "TEST");
	// Constness
	data_cp = "MOD";
	ASSERT(data_cp, "MOD");
	ASSERT(buff.data(), "TEST");
	std::string data_str(buff.data());
	data_str = "MOD";
	ASSERT(data_str, "MOD");
	ASSERT(buff.data(), "TEST");
}

// Test 3: Copy
void testCopy() {
	Buffer	buff();
	Buffer	buff_cp = buff;
	ASSERT(buff.data() == buff_cp.data(), true);
	ASSERT(&buff.data() == &buff_cp.data(), false);

	buff.append("TEST");
	ASSERT(buff.data() == buff_cp.data(), false);
	ASSERT(&buff.data() == &buff_cp.data(), false);
	buff_cp = buff;
	ASSERT(buff.data() == buff_cp.data(), true);
	ASSERT(&buff.data() == &buff_cp.data(), false);

	buff.consume(4);
	ASSERT(buff.data() == buff_cp.data(), false);
	ASSERT(&buff.data() == &buff_cp.data(), false);
	buff_cp = buff;
	ASSERT(buff.data() == buff_cp.data(), true);
	ASSERT(&buff.data() == &buff_cp.data(), false);
}


// Test 4: Size
void testSize() {
	Buffer buff();
	ASSERT(buff.size(), 0);
	buff.append("TEST");
	ASSERT(buff.size(), 4);
}

// Test 5: empty

// Test 6: append string

// Test 7: append char*

// Test 8: consume
	// 0 bytes
	// 1 bytes
	// n bytes

// Test 9: NULL chars
// Test 10: 
