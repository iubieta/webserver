// Processor.cpp
// -----------------------------------------------------------------------------

#include "../inc/Processor.hpp"
#include <vector>

// Builders --------------------------------------------------------------------
Processor::Processor(const std::vector<ServerConfig>& cfgs):
	cfgs_(cfgs)
{}

Processor::~Processor() {};

// Private methods -------------------------------------------------------------

// Finds the specific config based on the host of the request
ServerConfig& Processor::findConfig(const Request& req) {
	// TODO: implement this method or return first config on the list?
	return cfgs_->begin();
}


