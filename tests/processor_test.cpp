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

void testFindLocation() {
	{
		LocationConfig loc_1;
		loc_1.setTarget("/one");
		LocationConfig loc_2;
		loc_2.setTarget("/two");
		LocationConfig loc_3;
		loc_3.setTarget("/three");
		LocationConfig loc_4;
		loc_4.setTarget("/four");
		LocationConfig loc_5;
		loc_5.setTarget("/five");
		
		ServerConfig cfg;
		setBasicConfig(cfg);
		cfg.addLocation(loc_1);
		cfg.addLocation(loc_2);
		cfg.addLocation(loc_3);
		cfg.addLocation(loc_4);
		cfg.addLocation(loc_5);

		std::vector<ServerConfig> cfgs;
		cfgs.push_back(cfg);

		std::cout << "Location: " << &loc_1 << std::endl;
		std::cout << "Config: " << &cfg.getLocation(0) << std::endl;
		std::cout << "Configs: " << &cfgs[0].getLocation(0) << std::endl;

		Processor	proc(cfgs);
		Request		req;
		const LocationConfig	*found_location;
	
		req.setPath("/one");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location->getTarget(), "/one"); 
		
		req.setPath("/two");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_2);
		
		req.setPath("/three");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_2);
		
		req.setPath("/four");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_2);
		
		req.setPath("/five");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_2);
	}

	{
		LocationConfig loc_1;
		loc_1.setTarget("/one");
		LocationConfig loc_2;
		loc_2.setTarget("/one/two");
		LocationConfig loc_3;
		loc_3.setTarget("/one/two/three");
		LocationConfig loc_4;
		loc_4.setTarget("/one/two/three/four");
		LocationConfig loc_5;
		loc_5.setTarget("/one/two/three/four/five");
		
		ServerConfig cfg;
		setBasicConfig(cfg);

		std::vector<ServerConfig> cfgs;
		cfgs.push_back(cfg);

		Processor	proc(cfgs);
		Request		req;
		const LocationConfig	*found_location;
	
		req.setPath("/one");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_1);
		
		req.setPath("/one/two");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_2);
		
		req.setPath("/one/two/three");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_2);
		
		req.setPath("/one/two/three/four");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_2);
		
		req.setPath("/one/two/three/four/five");
		found_location = proc.findLocation(req, cfg);
		ASSERT_EQ(found_location, &loc_2);
	}

}


// void testRootIndex() {
// 
// 	std::vector<ServerConfig> cfgs;
// 	ServerConfig cfg;
// 	
// 	setBasicConfig(cfg);
// 	cfgs.push_back(cfg);
// 
// 	Request		req;
// 
// 	req.setRequest("GET /");
// 	req.setStatus(ESTATE_COMPLETE);
// 	req.setMethod("GET");
// 	req.setPath("/");
// 	req.setPath(NULL);
// 	req.setQuery(NULL);
// 	req.setVersion(HTTP_V1_1);
// 
// 	Decision	res;
// 	res.setStatus(FORBIDDEN);
// 	res.setHandler(ERROR_PAGE);
// 
// 	Processor	proc(cfgs);
// 	Decision	my_res = proc.procRequest(req);
// 
// }

int main() {
	Tester::runTest("Find location", testFindLocation);
	Tester::report();
}
