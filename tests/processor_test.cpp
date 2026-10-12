// processor_test.cpp
// -----------------------------------------------------------------------------

#include "Tester.hpp"

#include "../inc/Processor.hpp"

void testRedirs() {
	ServerConfig cfg("../
	Request		req;
	Response	res;

	req.setRequest("GET /redir_1");
	req.setStatus(ESTATE_COMPLETE);
	req.setMethod("GET");
	req.setLocation("/redir_1");
	req.setPath(NULL);
	req.setQuery(NULL);
	req.setVersion(HTTP_V1_1);

	res.setVersion(HTTP_V1_1);
	res.setStatus(PERMANENT_REDIR);
	res.addHeader("Server", "webserver");
	res.addHeader("Date", "");
	res.addHeader("Content-Type", "text/html");
	res.addHeader("Content-Length", "");
	res.addHeader("Location", "http:://localhost/permanent_redir");
	res.addHeader("Connection", "keep-alive");

	Processor	proc(;
	Response my_res = proc.procRequest(req);

}
