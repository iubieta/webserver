// connection_test.cpp
//
// tester of Connection class pure cpp methods
// -----------------------------------------------------------------------------

#include "Tester.hpp"
#include "../inc/Connection.hpp"

// Test 1: Constructor values
void testConstructor() {
	Connection conn(0);
	ASSERT_EQ(conn.isClosed(), false);
	ASSERT_EQ(conn.getFd(), 0);
	ASSERT_EQ(conn.getReadOffset(), 0);
	ASSERT_EQ(conn.getWriteOffset(), 0);
}

// Test 2: Discconnect sets closed to true 
void testDisconnect() {
	Connection conn(0);
	ASSERT_EQ(conn.isClosed(), false);
	conn.disconnect();
	ASSERT_EQ(conn.isClosed(), true);
}

// Test 3: Destructor closes the fd
void testDestructor() {
}

// Test 5: read buffer 
void e 

// Test 6: consume

// Test 7: clear read buffer

// Test 8: append

// Test 9: wantsWrite

// Test 10: clear write buffer

