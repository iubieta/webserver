// processor_test.cpp
// -----------------------------------------------------------------------------

#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "Tester.hpp"
#include "../inc/Processor.hpp"
#include "../inc/path_utils.hpp"

// Helpers ---------------------------------------------------------------------
static void setBasicConfig(ServerConfig& cfg) {
	cfg.setListen("80");
	cfg.setHost("localhost");
	cfg.setServerName("localhost");
	cfg.setRoot("/var/www");
	cfg.setClientMax("1024");
	cfg.setAutoindex("off");
}

static LocationConfig makeLocation(const std::string& target) {
	LocationConfig loc;
	loc.setTarget(target);
	return loc;
}

static Request makeRequest(const std::string& method, const std::string& path) {
	Request req;
	req.setRequest(method + " " + path);
	req.setStatus(ESTATE_COMPLETE);
	req.setMethod(method);
	req.setPath(path);
	req.setQuery("");
	req.setVersion(HTTP_V1_1);
	return req;
}

// Path utils ------------------------------------------------------------------
static void testDecodePath() {
	using PathUtils::decodePath;
	ASSERT_EQ(decodePath("/hola%20mundo"), "/hola mundo");
	ASSERT_EQ(decodePath("/espa%C3%B1a"), "/espa\xC3\xB1" "a");
	ASSERT_EQ(decodePath("/sin/codificar"), "/sin/codificar");
	ASSERT_EQ(decodePath("/%41%42%43"), "/ABC");
	ASSERT_EQ(decodePath("/%4a%4A"), "/JJ");		// minúsculas y mayúsculas
	ASSERT_EQ(decodePath("/%zz"), "/%zz");			// hex inválido
	ASSERT_EQ(decodePath("/fin%"), "/fin%");		// % al final
	ASSERT_EQ(decodePath("/fin%4"), "/fin%4");		// incompleto
	ASSERT_EQ(decodePath("%2E%2E/x"), "../x");		// .. se trata luego
}

static void testNormalizePath() {
	using PathUtils::normalizePath;
	ASSERT_EQ(normalizePath("/a/b/c"), "/a/b/c");
	ASSERT_EQ(normalizePath("/a/./b//c"), "/a/b/c");		// . y barras dobles
	ASSERT_EQ(normalizePath("/a/b/../c"), "/a/c");			// retroceso
	ASSERT_EQ(normalizePath("/a/b/../../c"), "/c");			// varios retrocesos
	ASSERT_EQ(normalizePath("/a/b/../.."), "/");			// vuelve a la raíz
	ASSERT_EQ(normalizePath("/"), "/");
	ASSERT_EQ(normalizePath(""), "/");
	ASSERT_EQ(normalizePath("/.."), "");					// sale de la raíz
	ASSERT_EQ(normalizePath("/a/../../etc/passwd"), "");	// sale de la raíz
	ASSERT_EQ(normalizePath("/a/..b/c"), "/a/..b/c");	// "..b" es nombre
}

// normalizePath keeps the trailing "/" (RFC 3986 5.2.4)
static void testNormalizeTrailingSlash() {
	using PathUtils::normalizePath;
	ASSERT_EQ(normalizePath("/images/"), "/images/");
	ASSERT_EQ(normalizePath("/a/b/../"), "/a/");
	ASSERT_EQ(normalizePath("/a/b/.."), "/a/");		// RFC 3986 5.2.4
	ASSERT_EQ(normalizePath("/a/."), "/a/");
	ASSERT_EQ(normalizePath("/a/b"), "/a/b");			// sin barra, sigue sin
	ASSERT_EQ(normalizePath("//"), "/");
}

// Locations -------------------------------------------------------------------
static void testDifferentLocations() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/one"));
	cfg.addLocation(makeLocation("/two"));
	cfg.addLocation(makeLocation("/three"));

	const LocationConfig *found;
	found = PathUtils::findLocation("/one", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/one");
	found = PathUtils::findLocation("/two", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/two");
	found = PathUtils::findLocation("/three", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/three");
	found = PathUtils::findLocation("/four", cfg);
	ASSERT_EQ(found, (const LocationConfig*)NULL);
}

static void testLocationLongestMatch() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/one"));
	cfg.addLocation(makeLocation("/one/two"));
	cfg.addLocation(makeLocation("/one/two/three"));
	cfg.addLocation(makeLocation("/one/one/one"));

	const LocationConfig *found;
	found = PathUtils::findLocation("/one/two", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/one/two");
	found = PathUtils::findLocation("/one/two/three", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/one/two/three");
	found = PathUtils::findLocation("/one/one/one", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/one/one/one");
}

// "/imagesfoo" must not match "/images"
static void testLocationSegmentBoundary() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/"));
	cfg.addLocation(makeLocation("/images"));

	const LocationConfig *found;
	found = PathUtils::findLocation("/imagesfoo", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/");
}

// Nested targets fall back to the longest location prefix
static void testLocationNestedPrefix() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/"));
	cfg.addLocation(makeLocation("/images"));
	cfg.addLocation(makeLocation("/images/icons"));

	const LocationConfig *found;
	found = PathUtils::findLocation("/images/cat.png", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/images");
	found = PathUtils::findLocation("/images/icons/a/b.png", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/images/icons");
	found = PathUtils::findLocation("/images/", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/images");
	found = PathUtils::findLocation("/other/x", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/");
}

// Location written with trailing "/" in the config ("location /images/")
static void testLocationTrailingSlash() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/"));
	cfg.addLocation(makeLocation("/images/"));

	const LocationConfig *found;
	found = PathUtils::findLocation("/images/", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/images/");
	found = PathUtils::findLocation("/images/cat.png", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/images/");
	found = PathUtils::findLocation("/images/icons/a.png", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/images/");
	found = PathUtils::findLocation("/imagesfoo", cfg);
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/");
	found = PathUtils::findLocation("/images", cfg);		// like NGINX
	ASSERT_NOT_EQ(found, (const LocationConfig*)NULL);
	ASSERT_EQ(found->getTarget(), "/");
}

// Processor -------------------------------------------------------------------
static void testGetRootWithoutLocations() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor	proc(cfgs);
	Decision	res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
}

static void testGetRootWithLocations() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/one"));
	cfg.addLocation(makeLocation("/one/two"));
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor	proc(cfgs);
	Decision	res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
}

static void testPathEscapesRoot() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/"));
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor	proc(cfgs);
	Decision	res = proc.procRequest(makeRequest("GET", "/../etc/passwd"));
	ASSERT_EQ(res.getStatus(), BAD_REQUEST);
	res = proc.procRequest(makeRequest("GET", "/%2E%2E/etc/passwd"));
	ASSERT_EQ(res.getStatus(), BAD_REQUEST);
}

// Redirects -------------------------------------------------------------------
static Decision redirectDecision(const std::string& method,
	const std::string& path, const std::string& code,
	const std::string& destination, const std::string& root = "")
{
	LocationConfig loc = makeLocation("/old");
	if (!root.empty())
		loc.setRoot(root);
	loc.setReturn(code, destination);
	loc.setReturnSet(true);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	return proc.procRequest(makeRequest(method, path));
}

// Relative targets are stored verbatim: root is a filesystem mapping, not a URL prefix
static void testRedirectRelativeTarget() {
	Decision res = redirectDecision("GET", "/old", "301", "/new", "/var/www");
	ASSERT_EQ(res.getStatus(), MOVED_PERMANENTLY);
	ASSERT_EQ(res.getHandler(), REDIRECTION);
	ASSERT_EQ(res.getRedirURL(), "/new");
}

// Absolute URLs are preserved as configured
static void testRedirectAbsoluteTarget() {
	Decision res = redirectDecision("GET", "/old", "302",
		"https://example.com/new");
	ASSERT_EQ(res.getStatus(), FOUND);
	ASSERT_EQ(res.getHandler(), REDIRECTION);
	ASSERT_EQ(res.getRedirURL(), "https://example.com/new");
}

// The status comes from the config, not from a hardcoded 301
static void testRedirectConfiguredStatus() {
	Decision res = redirectDecision("GET", "/old", "307", "/tmp");
	ASSERT_EQ(res.getStatus(), TEMPORARY_REDIR);
	res = redirectDecision("GET", "/old", "308", "/tmp");
	ASSERT_EQ(res.getStatus(), PERMANENT_REDIR);
}

// return short-circuits the method check
static void testRedirectBeforeMethodCheck() {
	LocationConfig loc = makeLocation("/old");
	loc.setReturn("301", "/new");
	loc.setReturnSet(true);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Decision res = proc.procRequest(makeRequest("DELETE", "/old"));
	ASSERT_EQ(res.getStatus(), MOVED_PERMANENTLY);
	ASSERT_EQ(res.getHandler(), REDIRECTION);
}

// Returns the status of "method /" against a "/" location with "allowed"
static e_status_code methodStatus(const std::string& method,
	const std::string& allowed)
{
	LocationConfig loc = makeLocation("/");
	loc.setAutoindex(true);
	if (!allowed.empty())
		loc.addMethod(allowed);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	return proc.procRequest(makeRequest(method, "/")).getStatus();
}

static void testAllowedMethods() {
	ASSERT_EQ(methodStatus("GET", ""), METHOD_NOT_ALLOWED);
	ASSERT_EQ(methodStatus("GET", "GET"), OK);
	ASSERT_EQ(methodStatus("POST", "GET"), METHOD_NOT_ALLOWED);
	ASSERT_EQ(methodStatus("POST", "POST"), OK);
	ASSERT_EQ(methodStatus("DELETE", "GET"), METHOD_NOT_ALLOWED);
	ASSERT_EQ(methodStatus("DELETE", "DELETE"), OK);
}

// 405 carries the allowed methods for the "Allow" header
static void testMethodNotAllowedList() {
	LocationConfig loc = makeLocation("/");
	loc.addMethod("GET");
	loc.addMethod("POST");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor	proc(cfgs);
	Decision	res = proc.procRequest(makeRequest("DELETE", "/"));
	ASSERT_EQ(res.getStatus(), METHOD_NOT_ALLOWED);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
	ASSERT_EQ(res.getAllowedMethods().size(), 2u);
	ASSERT_EQ(res.getAllowedMethods()[0], "GET");
	ASSERT_EQ(res.getAllowedMethods()[1], "POST");
}

static Decision diskPathDecision(const std::string& path) {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("GET");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	return proc.procRequest(makeRequest("GET", path));
}

static void testDiskPath() {
	ASSERT_EQ(diskPathDecision("/").getDiskPath(), "/var/www/");
	ASSERT_EQ(diskPathDecision("/images").getDiskPath(), "/var/www/images");
}

// Disk path keeps the trailing "/" and nested segments
static void testDiskPathNested() {
	ASSERT_EQ(diskPathDecision("/images/").getDiskPath(), "/var/www/images/");
	ASSERT_EQ(diskPathDecision("/images/img.jpeg").getDiskPath(),
		"/var/www/images/img.jpeg");
}

// Content-Length helpers ------------------------------------------------------
static Request makeRequestWithCL(const std::string& method,
								const std::string& path,
								size_t contentLength)
{
	Request req = makeRequest(method, path);
	std::stringstream iss;
	iss << contentLength;
	req.addHeader("Content-Length", iss.str());
	return req;
}

static Request makeRequestChunked(const std::string& method, const std::string& path)
{
	Request req = makeRequest(method, path);
	req.addHeader("Transfer-Encoding", "chunked");
	return req;
}

// FS helpers for stat tests ---------------------------------------------------
static std::string makeTempRoot(const std::string& suffix) {
	char tmpl[128];
	std::strncpy(tmpl, "/tmp/webserv_test", sizeof(tmpl) - 1);
	tmpl[sizeof(tmpl) - 1] = '\0';
	char* d = mkdtemp(tmpl);
	if (!d)
		return std::string("");
	std::string base(d);
	(void)suffix;
	return base;
}

static void writeEmptyFile(const std::string& path) {
	FILE* f = std::fopen(path.c_str(), "w");
	if (f)
		std::fclose(f);
}

static void removeDirRecursive(const std::string& path) {
	if (path.empty())
		return;
	std::string cmd = "rm -rf " + path;
	std::system(cmd.c_str());
}

// Content-Length within limit -> no debe ser 413
static void testContentLengthWithinLimit() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("POST");
	loc.setClientMax("1024");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Request req = makeRequestWithCL("POST", "/", 512);
	Decision res = proc.procRequest(req);
	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
}

// Content-Length > limit -> 413 Payload Too Large, ERROR_PAGE
static void testContentLengthExceedsLimit() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("POST");
	loc.setClientMax("10");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Request req = makeRequestWithCL("POST", "/", 100);
	Decision res = proc.procRequest(req);
	ASSERT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
}

// Igual al límite -> permitido (> , no >=)
static void testContentLengthEqualsLimit() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("POST");
	loc.setClientMax("100");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Request req = makeRequestWithCL("POST", "/", 100);
	Decision res = proc.procRequest(req);
	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
}

// client_max_body_size 0 -> sin límite
static void testContentLengthLimitZero() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("POST");
	loc.setClientMax("0");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Request req = makeRequestWithCL("POST", "/", 999999);
	Decision res = proc.procRequest(req);
	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
}

// Chunked: el processor NO debe rechazar por client_max_body_size
static void testContentLengthChunkedIgnoredByProcessor() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("POST");
	loc.setClientMax("10");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Request req = makeRequestChunked("POST", "/");
	Decision res = proc.procRequest(req);
	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
}

// Prioridad: método no permitido + Content-Length excesivo -> debe prevalecer 405 con Allow
static void testContentLengthMethodNotAllowedHasPriority() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("GET");
	loc.setClientMax("10");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Request req = makeRequestWithCL("DELETE", "/", 500);
	Decision res = proc.procRequest(req);
	ASSERT_EQ(res.getStatus(), METHOD_NOT_ALLOWED);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
	ASSERT_EQ(res.getAllowedMethods().size(), 1u);
	ASSERT_EQ(res.getAllowedMethods()[0], "GET");
}

// Sin Content-Length ni chunked -> no debe dar 413
static void testNoContentLengthNo413() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("GET");
	loc.setClientMax("1");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Request req = makeRequest("GET", "/");
	Decision res = proc.procRequest(req);
	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
}

// stat() tests ---------------------------------------------------------------
// Fichero regular legible -> READ_FILE
static void testStatRegularFile() {
	std::string root = makeTempRoot("statfile");
	if (root.empty())
		return;
	std::string file = root + "/index.html";
	writeEmptyFile(file);

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("GET");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Decision res = proc.procRequest(makeRequest("GET", "/index.html"));
	ASSERT_EQ(res.getHandler(), READ_FILE);
	ASSERT_EQ(res.getDiskPath(), file);

	removeDirRecursive(root);
}

// Fichero no existente -> 404
static void testStatMissingFile() {
	std::string root = makeTempRoot("statmiss");
	if (root.empty())
		return;

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("GET");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Decision res = proc.procRequest(makeRequest("GET", "/nope.html"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);

	removeDirRecursive(root);
}

// Directorio sin permisos -> 403 (si el proceso puede detectar EACCES)
static void testStatNoPermission() {
	std::string root = makeTempRoot("statperm");
	if (root.empty())
		return;
	std::string sub = root + "/secret";
	if (mkdir(sub.c_str(), 0000) == 0) {
		LocationConfig loc = makeLocation("/");
		loc.setRoot(root);
		loc.addMethod("GET");
		ServerConfig cfg;
		setBasicConfig(cfg);
		cfg.addLocation(loc);
		std::vector<ServerConfig> cfgs;
		cfgs.push_back(cfg);

		Processor proc(cfgs);
		Decision res = proc.procRequest(makeRequest("GET", "/secret/"));
		ASSERT_EQ(res.getStatus(), FORBIDDEN);
		ASSERT_EQ(res.getHandler(), ERROR_PAGE);

		chmod(sub.c_str(), 0755);
	}
	removeDirRecursive(root);
}

// Directorio con index -> READ_FILE (index tiene prioridad)
static void testStatDirWithIndex() {
	std::string root = makeTempRoot("statidx");
	if (root.empty())
		return;
	std::string index = root + "/index.html";
	writeEmptyFile(index);

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("GET");
	loc.addIndex("index.html");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Decision res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getHandler(), READ_FILE);
	ASSERT_EQ(res.getDiskPath(), index);

	removeDirRecursive(root);
}

// Directorio sin index y autoindex on -> LIST_DIR
static void testStatDirAutoindexOn() {
	std::string root = makeTempRoot("statautoon");
	if (root.empty())
		return;

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("GET");
	loc.setAutoindex("on");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Decision res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getHandler(), LIST_DIR);
	ASSERT_EQ(res.getDiskPath(), root + "/");

	removeDirRecursive(root);
}

// Directorio sin index y autoindex off -> 403
static void testStatDirAutoindexOff() {
	std::string root = makeTempRoot("statautooff");
	if (root.empty())
		return;

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("GET");
	loc.setAutoindex("off");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Decision res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getStatus(), FORBIDDEN);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);

	removeDirRecursive(root);
}

// Directorio con index definido pero ausente y autoindex on -> LIST_DIR (fallback)
static void testStatDirIndexMissingAutoindexOnFallback() {
	std::string root = makeTempRoot("statidxmiss");
	if (root.empty())
		return;

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("GET");
	loc.addIndex("index.html"); // existe pero no creamos fichero
	loc.setAutoindex("on");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Decision res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getHandler(), LIST_DIR);

	removeDirRecursive(root);
}

// Directorio con index definido pero ausente y autoindex off -> 403
static void testStatDirIndexMissingAutoindexOffFallback() {
	std::string root = makeTempRoot("statidxmiss2");
	if (root.empty())
		return;

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("GET");
	loc.addIndex("index.html");
	loc.setAutoindex("off");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);
	std::vector<ServerConfig> cfgs;
	cfgs.push_back(cfg);

	Processor proc(cfgs);
	Decision res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getStatus(), FORBIDDEN);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);

	removeDirRecursive(root);
}

void processor_tests() {
	Tester::runTest("Path decoder", testDecodePath);
	Tester::runTest("Path normalizer", testNormalizePath);
	Tester::runTest("Trailing slash", testNormalizeTrailingSlash);
	Tester::runTest("Nested prefix", testLocationNestedPrefix);
	Tester::runTest("Disk path nested", testDiskPathNested);

	Tester::runTest("Different locations", testDifferentLocations);
	Tester::runTest("Longest location match", testLocationLongestMatch);
	Tester::runTest("Location segment boundary", testLocationSegmentBoundary);
	Tester::runTest("Location with trailing slash", testLocationTrailingSlash);
	Tester::runTest("GET / w/o location", testGetRootWithoutLocations);
	Tester::runTest("GET / with locations", testGetRootWithLocations);
	Tester::runTest("Path escapes root", testPathEscapesRoot);
	Tester::runTest("Redirect relative target", testRedirectRelativeTarget);
	Tester::runTest("Redirect absolute target", testRedirectAbsoluteTarget);
	Tester::runTest("Redirect configured status", testRedirectConfiguredStatus);
	Tester::runTest("Redirect before method check", testRedirectBeforeMethodCheck);
	Tester::runTest("Allowed methods", testAllowedMethods);
	Tester::runTest("405 allowed list", testMethodNotAllowedList);
	Tester::runTest("Disk path", testDiskPath);
	Tester::runTest("Content-Length within limit", testContentLengthWithinLimit);
	Tester::runTest("Content-Length equals limit allowed", testContentLengthEqualsLimit);
	Tester::runTest("Content-Length exceeds limit -> 413", testContentLengthExceedsLimit);
	Tester::runTest("client_max_body_size 0 = no limit", testContentLengthLimitZero);
	Tester::runTest("Chunked ignored by processor", testContentLengthChunkedIgnoredByProcessor);
	Tester::runTest("413 vs 405 priority (method not allowed)", testContentLengthMethodNotAllowedHasPriority);
	Tester::runTest("No Content-Length -> no 413", testNoContentLengthNo413);
	Tester::runTest("stat regular file -> READ_FILE", testStatRegularFile);
	Tester::runTest("stat missing -> 404", testStatMissingFile);
	Tester::runTest("stat no permission -> 403", testStatNoPermission);
	Tester::runTest("stat dir with index -> READ_FILE", testStatDirWithIndex);
	Tester::runTest("stat dir autoindex on -> LIST_DIR", testStatDirAutoindexOn);
	Tester::runTest("stat dir autoindex off -> 403", testStatDirAutoindexOff);
	Tester::runTest("stat dir index missing + autoindex on -> LIST_DIR", testStatDirIndexMissingAutoindexOnFallback);
	Tester::runTest("stat dir index missing + autoindex off -> 403", testStatDirIndexMissingAutoindexOffFallback);

	Tester::report();
}
