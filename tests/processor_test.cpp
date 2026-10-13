// processor_test.cpp
// -----------------------------------------------------------------------------

#include "Tester.hpp"

#include "../inc/Processor.hpp"
#include <vector>

static void setBasicConfig(ServerConfig& cfg) {
	cfg.setListen("80");
	cfg.setHost("localhost");
	cfg.setServerName("localhost");
	cfg.setRoot("/var/www");
	cfg.setClientMax("1024");
	cfg.setAutoindex("off");
}

// void testDifferentLocations()
// {
// 	LocationConfig loc_1;
// 	loc_1.setTarget("/one");
// 	LocationConfig loc_2;
// 	loc_2.setTarget("/two");
// 	LocationConfig loc_3;
// 	loc_3.setTarget("/three");
// 	LocationConfig loc_4;
// 	loc_4.setTarget("/four");
// 	LocationConfig loc_5;
// 	loc_5.setTarget("/five");
// 
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc_1);
// 	cfg.addLocation(loc_2);
// 	cfg.addLocation(loc_3);
// 	cfg.addLocation(loc_4);
// 	cfg.addLocation(loc_5);
// 
// 	std::vector<ServerConfig> cfgs;
// 	cfgs.push_back(cfg);
// 
// 	Processor	proc(cfgs);
// 	Request		req;
// 	const LocationConfig	*found_location;
// 
// 	req.setPath("/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_1), true);
// 
// 	req.setPath("/two");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_2), true);
// 
// 	req.setPath("/three");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_3), true);
// 
// 	req.setPath("/four");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_4), true);
// 
// 	req.setPath("/five");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_5), true);
// }
// 
// void testLocationLongestMatch_1()
// {
// 	LocationConfig loc_1;
// 	loc_1.setTarget("/one");
// 	LocationConfig loc_2;
// 	loc_2.setTarget("/one/two");
// 	LocationConfig loc_3;
// 	loc_3.setTarget("/one/two/three");
// 	LocationConfig loc_4;
// 	loc_4.setTarget("/one/two/three/four");
// 	LocationConfig loc_5;
// 	loc_5.setTarget("/one/two/three/four/five");
// 	
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc_1);
// 	cfg.addLocation(loc_2);
// 	cfg.addLocation(loc_3);
// 	cfg.addLocation(loc_4);
// 	cfg.addLocation(loc_5);
// 
// 	std::vector<ServerConfig> cfgs;
// 	cfgs.push_back(cfg);
// 
// 	Processor	proc(cfgs);
// 	Request		req;
// 	const LocationConfig	*found_location;
// 
// 	req.setPath("/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_1), true);
// 	
// 	req.setPath("/one/two");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_2), true);
// 	
// 	req.setPath("/one/two/three");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_3), true);
// 	
// 	req.setPath("/one/two/three/four");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_4), true);
// 	
// 	req.setPath("/one/two/three/four/five");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_5), true);
// }
// 
// void testLocationLongestMatch_2()
// {
// 	LocationConfig loc_1;
// 	loc_1.setTarget("/one");
// 	LocationConfig loc_2;
// 	loc_2.setTarget("/one/one");
// 	LocationConfig loc_3;
// 	loc_3.setTarget("/one/one/one");
// 	LocationConfig loc_4;
// 	loc_4.setTarget("/one/one/one/one");
// 	LocationConfig loc_5;
// 	loc_5.setTarget("/one/one/one/one/one");
// 	
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc_1);
// 	cfg.addLocation(loc_2);
// 	cfg.addLocation(loc_3);
// 	cfg.addLocation(loc_4);
// 	cfg.addLocation(loc_5);
// 
// 	std::vector<ServerConfig> cfgs;
// 	cfgs.push_back(cfg);
// 
// 	Processor	proc(cfgs);
// 	Request		req;
// 	const LocationConfig	*found_location;
// 
// 	req.setPath("/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_1), true);
// 	
// 	req.setPath("/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_2), true);
// 	
// 	req.setPath("/one/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_3), true);
// 	
// 	req.setPath("/one/one/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_4), true);
// 	
// 	req.setPath("/one/one/one/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_5), true);
// }
// 
// void testLocationLongestMatch_3()
// {
// 	LocationConfig loc_1;
// 	loc_1.setTarget("/one/one/one/one/one");
// 	LocationConfig loc_2;
// 	loc_2.setTarget("/one/one/two/one/one");
// 	LocationConfig loc_3;
// 	loc_3.setTarget("/one/one/three/one/one");
// 	LocationConfig loc_4;
// 	loc_4.setTarget("/one/one/four/one/one");
// 	LocationConfig loc_5;
// 	loc_5.setTarget("/one/one/five/one/one");
// 	
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc_1);
// 	cfg.addLocation(loc_2);
// 	cfg.addLocation(loc_3);
// 	cfg.addLocation(loc_4);
// 	cfg.addLocation(loc_5);
// 
// 	std::vector<ServerConfig> cfgs;
// 	cfgs.push_back(cfg);
// 
// 	Processor	proc(cfgs);
// 	Request		req;
// 	const LocationConfig	*found_location;
// 
// 	req.setPath("/one/one/one/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_1), true);
// 	
// 	req.setPath("/one/one/two/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_2), true);
// 	
// 	req.setPath("/one/one/three/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_3), true);
// 	
// 	req.setPath("/one/one/four/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_4), true);
// 	
// 	req.setPath("/one/one/five/one/one");
// 	found_location = proc.findLocation(req, cfg);
// 	ASSERT_EQ((found_location != NULL), true);
// 	ASSERT_EQ((*found_location == loc_5), true);
// }


void testRootIndex()
{
	
	// TODO: should every cfg has at least one location?
	std::vector<ServerConfig> cfgs;
	ServerConfig cfg;
	
	setBasicConfig(cfg);
	cfgs.push_back(cfg);

	Request		req;

	req.setRequest("GET /");
	req.setStatus(ESTATE_COMPLETE);
	req.setMethod("GET");
	req.setPath("/");
	req.setPath("");
	req.setQuery("");
	req.setVersion(HTTP_V1_1);

	Decision	res;
	res.setStatus(FORBIDDEN);
	res.setHandler(ERROR_PAGE);

	Processor	proc(cfgs);
	Decision	my_res = proc.procRequest(req);
	ASSERT_EQ(my_res.getStatus(), res.getStatus());
	ASSERT_EQ(my_res.getHandler(), res.getHandler());
}

int main() {
	// Tester::runTest("Different Locations", testDifferentLocations);
	// Tester::runTest("Longest Location Match 1", testLocationLongestMatch_1);
	// Tester::runTest("Longest Location Match 2", testLocationLongestMatch_2);
	// Tester::runTest("Longest Location Match 3", testLocationLongestMatch_3);
	Tester::runTest("GET /", testRootIndex);
	Tester::report();
}
