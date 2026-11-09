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
	req.setMethod(method);
	req.setPath(path);
	req.setQuery("");
	req.setVersion(HTTP_V1_1);
	return req;
}

// FS helpers (definidos más abajo, usados también por testAllowedMethods)
static std::string makeTempRoot(const std::string& suffix);
static void removeDirRecursive(const std::string& path);

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

	Processor proc(&cfg);
	Decision	res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
}

static void testGetRootWithLocations() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/one"));
	cfg.addLocation(makeLocation("/one/two"));

	Processor proc(&cfg);
	Decision	res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
}

static void testPathEscapesRoot() {
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(makeLocation("/"));

	Processor proc(&cfg);
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

	Processor proc(&cfg);
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

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("DELETE", "/old"));
	ASSERT_EQ(res.getStatus(), MOVED_PERMANENTLY);
	ASSERT_EQ(res.getHandler(), REDIRECTION);
}

// Returns the status of `method path` against a "/" location with `allowed`
static e_status_code methodStatus(const std::string& method,
	const std::string& allowed, const std::string& path,
	const std::string& root)
{
	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.setAutoindex(true);
	if (!allowed.empty())
		loc.addMethod(allowed);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	return proc.procRequest(makeRequest(method, path)).getStatus();
}

static void testAllowedMethods() {
	std::string root = makeTempRoot("methods");
	if (root.empty())
		return;

	ASSERT_EQ(methodStatus("GET", "", "/", root), METHOD_NOT_ALLOWED);
	ASSERT_EQ(methodStatus("GET", "GET", "/", root), OK);
	ASSERT_EQ(methodStatus("POST", "GET", "/new.txt", root),
		METHOD_NOT_ALLOWED);
	ASSERT_EQ(methodStatus("POST", "POST", "/new.txt", root), OK);
	ASSERT_EQ(methodStatus("DELETE", "GET", "/", root), METHOD_NOT_ALLOWED);
	ASSERT_EQ(methodStatus("DELETE", "DELETE", "/", root), OK);

	removeDirRecursive(root);
}

// 405 carries the allowed methods for the "Allow" header
static void testMethodNotAllowedList() {
	LocationConfig loc = makeLocation("/");
	loc.addMethod("GET");
	loc.addMethod("POST");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
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

	Processor proc(&cfg);
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
// Static Request makeRequestWithCL(const std::string& method,
// 								const std::string& path,
// 								size_t contentLength)
// {
// 	Request req = makeRequest(method, path);
// 	std::stringstream iss;
// 	iss << contentLength;
// 	req.addHeader("Content-Length", iss.str());
// 	return req;
// }
// 
// static Request makeRequestChunked(const std::string& method, const std::string& path)
// {
// 	Request req = makeRequest(method, path);
// 	req.addHeader("Transfer-Encoding", "chunked");
// 	return req;
// }

// FS helpers for stat tests ---------------------------------------------------
static std::string makeTempRoot(const std::string& suffix) {
	char tmpl[128];
	std::strncpy(tmpl, "/tmp/webserv_testXXXXXX", sizeof(tmpl) - 1);
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
// static void testContentLengthWithinLimit() {
// 	LocationConfig loc = makeLocation("/");
// 	loc.setRoot("/var/www");
// 	loc.addMethod("POST");
// 	loc.setClientMax("1024");
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc);
// 
// 	Processor proc(&cfg);
// 	Request req = makeRequestWithCL("POST", "/", 512);
// 	Decision res = proc.procRequest(req);
// 	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
// }
// 
// // Content-Length > limit -> 413 Payload Too Large, ERROR_PAGE
// static void testContentLengthExceedsLimit() {
// 	LocationConfig loc = makeLocation("/");
// 	loc.setRoot("/var/www");
// 	loc.addMethod("POST");
// 	loc.setClientMax("10");
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc);
// 
// 	Processor proc(&cfg);
// 	Request req = makeRequestWithCL("POST", "/", 100);
// 	Decision res = proc.procRequest(req);
// 	ASSERT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
// 	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
// }
// 
// // Igual al límite -> permitido (> , no >=)
// static void testContentLengthEqualsLimit() {
// 	LocationConfig loc = makeLocation("/");
// 	loc.setRoot("/var/www");
// 	loc.addMethod("POST");
// 	loc.setClientMax("100");
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc);
// 
// 	Processor proc(&cfg);
// 	Request req = makeRequestWithCL("POST", "/", 100);
// 	Decision res = proc.procRequest(req);
// 	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
// }
// 
// // client_max_body_size 0 -> sin límite
// static void testContentLengthLimitZero() {
// 	LocationConfig loc = makeLocation("/");
// 	loc.setRoot("/var/www");
// 	loc.addMethod("POST");
// 	loc.setClientMax("0");
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc);
// 
// 	Processor proc(&cfg);
// 	Request req = makeRequestWithCL("POST", "/", 999999);
// 	Decision res = proc.procRequest(req);
// 	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
// }
// 
// // Chunked: el processor NO debe rechazar por client_max_body_size
// static void testContentLengthChunkedIgnoredByProcessor() {
// 	LocationConfig loc = makeLocation("/");
// 	loc.setRoot("/var/www");
// 	loc.addMethod("POST");
// 	loc.setClientMax("10");
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc);
// 
// 	Processor proc(&cfg);
// 	Request req = makeRequestChunked("POST", "/");
// 	Decision res = proc.procRequest(req);
// 	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
// }
// 
// // Prioridad: método no permitido + Content-Length excesivo -> debe prevalecer 405 con Allow
// static void testContentLengthMethodNotAllowedHasPriority() {
// 	LocationConfig loc = makeLocation("/");
// 	loc.setRoot("/var/www");
// 	loc.addMethod("GET");
// 	loc.setClientMax("10");
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc);
// 
// 	Processor proc(&cfg);
// 	Request req = makeRequestWithCL("DELETE", "/", 500);
// 	Decision res = proc.procRequest(req);
// 	ASSERT_EQ(res.getStatus(), METHOD_NOT_ALLOWED);
// 	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
// 	ASSERT_EQ(res.getAllowedMethods().size(), 1u);
// 	ASSERT_EQ(res.getAllowedMethods()[0], "GET");
// }
// 
// // Sin Content-Length ni chunked -> no debe dar 413
// static void testNoContentLengthNo413() {
// 	LocationConfig loc = makeLocation("/");
// 	loc.setRoot("/var/www");
// 	loc.addMethod("GET");
// 	loc.setClientMax("1");
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc);
// 
// 	Processor proc(&cfg);
// 	Request req = makeRequest("GET", "/");
// 	Decision res = proc.procRequest(req);
// 	ASSERT_NOT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
// }

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

	Processor proc(&cfg);
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

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("GET", "/nope.html"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);

	removeDirRecursive(root);
}

// Directorio sin permisos de búsqueda -> 403 (si el proceso detecta EACCES)
static void testStatNoPermission() {
	std::string root = makeTempRoot("statperm");
	if (root.empty())
		return;
	// root ignora los permisos, EACCES no se puede provocar
	if (geteuid() == 0) {
		removeDirRecursive(root);
		return;
	}
	std::string sub = root + "/secret";
	if (mkdir(sub.c_str(), 0000) == 0) {
		LocationConfig loc = makeLocation("/");
		loc.setRoot(root);
		loc.addMethod("GET");
		ServerConfig cfg;
		setBasicConfig(cfg);
		cfg.addLocation(loc);

		Processor proc(&cfg);
		// "secret" es un directorio intermedio: stat() no puede atravesarlo
		Decision res = proc.procRequest(makeRequest("GET", "/secret/file"));
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

	Processor proc(&cfg);
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
	loc.setAutoindex(true);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
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
	loc.setAutoindex(false);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
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
	loc.setAutoindex(true);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
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
	loc.setAutoindex(false);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("GET", "/"));
	ASSERT_EQ(res.getStatus(), FORBIDDEN);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);

	removeDirRecursive(root);
}

// POST / DELETE ----------------------------------------------------------------
// POST a un fichero inexistente -> orden de crearlo, NO 404
static void testPostMissingFileCreates() {
	std::string root = makeTempRoot("postcreate");
	if (root.empty())
		return;

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("POST");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("POST", "/upload.txt"));
	ASSERT_EQ(res.getStatus(), OK);				// provisional: el handler pone 201/200
	ASSERT_EQ(res.getHandler(), WRITE_FILE);
	ASSERT_EQ(res.getDiskPath(), root + "/upload.txt");

	removeDirRecursive(root);
}

// POST a un fichero existente -> WRITE_FILE (sobrescribir)
static void testPostExistingFile() {
	std::string root = makeTempRoot("postexisting");
	if (root.empty())
		return;
	writeEmptyFile(root + "/upload.txt");

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("POST");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("POST", "/upload.txt"));
	ASSERT_EQ(res.getHandler(), WRITE_FILE);
	ASSERT_EQ(res.getDiskPath(), root + "/upload.txt");

	removeDirRecursive(root);
}

// POST sobre un directorio -> 403 (no sirve index ni listing)
static void testPostDirectoryForbidden() {
	std::string root = makeTempRoot("postdir");
	if (root.empty())
		return;

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("POST");
	loc.setAutoindex(true);
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("POST", "/"));
	ASSERT_EQ(res.getStatus(), FORBIDDEN);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);

	removeDirRecursive(root);
}

// POST no permitido -> 405 (el gate va antes del stat)
static void testPostNotAllowedMissingFile() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/tmp");
	loc.addMethod("GET");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("POST", "/missing.txt"));
	ASSERT_EQ(res.getStatus(), METHOD_NOT_ALLOWED);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
}

// DELETE: existente -> REMOVE_FILE ; inexistente -> 404
static void testDeleteExistingAndMissing() {
	std::string root = makeTempRoot("deletefile");
	if (root.empty())
		return;
	writeEmptyFile(root + "/bye.txt");

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("DELETE");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("DELETE", "/bye.txt"));
	ASSERT_EQ(res.getHandler(), REMOVE_FILE);
	ASSERT_EQ(res.getDiskPath(), root + "/bye.txt");

	res = proc.procRequest(makeRequest("DELETE", "/missing.txt"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);

	removeDirRecursive(root);
}

// Regresión ENOTDIR: componente intermedio es un fichero -> 404, no 500
static void testPathComponentNotDir() {
	std::string root = makeTempRoot("enotdir");
	if (root.empty())
		return;
	writeEmptyFile(root + "/file.txt");

	LocationConfig loc = makeLocation("/");
	loc.setRoot(root);
	loc.addMethod("GET");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("GET", "/file.txt/child"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);

	removeDirRecursive(root);
}

// Decision copia/asigna el puntero al ServerConfig (setError depende de él)
static void testDecisionCopyKeepsServerConfig() {
	ServerConfig cfg;
	setBasicConfig(cfg);

	Decision original;
	original.setServerConfig(&cfg);

	Decision copied(original);
	ASSERT_EQ(copied.getServerConfig(), &cfg);

	Decision assigned;
	assigned = original;
	ASSERT_EQ(assigned.getServerConfig(), &cfg);
}

// Error pages ------------------------------------------------------------------
static void testErrorPageFromLocation() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("GET");
	loc.addErrorPage("404", "/errors/404.html");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("GET", "/missing"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
	ASSERT_EQ(res.getErrorPage(), "/errors/404.html");
}

static void testErrorPageFromServer() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("GET");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addErrorPage("404", "/errors/404.html");
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("GET", "/missing"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
	ASSERT_EQ(res.getErrorPage(), "/errors/404.html");
}

static void testErrorPageLocationOverridesServer() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("GET");
	loc.addErrorPage("404", "/errors/loc404.html");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addErrorPage("404", "/errors/srv404.html");
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("GET", "/missing"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
	ASSERT_EQ(res.getErrorPage(), "/errors/loc404.html");
}

static void testErrorPageDefaultEmpty() {
	LocationConfig loc = makeLocation("/");
	loc.setRoot("/var/www");
	loc.addMethod("GET");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("GET", "/missing"));
	ASSERT_EQ(res.getStatus(), NOT_FOUND);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
	ASSERT_EQ(res.getErrorPage(), std::string(""));
}

static void testErrorPageFor405() {
	LocationConfig loc = makeLocation("/");
	loc.addMethod("GET");
	loc.addErrorPage("405", "/errors/405.html");
	ServerConfig cfg;
	setBasicConfig(cfg);
	cfg.addLocation(loc);

	Processor proc(&cfg);
	Decision res = proc.procRequest(makeRequest("DELETE", "/"));
	ASSERT_EQ(res.getStatus(), METHOD_NOT_ALLOWED);
	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
	ASSERT_EQ(res.getErrorPage(), "/errors/405.html");
}

// static void testErrorPageFor413() {
// 	LocationConfig loc = makeLocation("/");
// 	loc.setRoot("/var/www");
// 	loc.addMethod("POST");
// 	loc.setClientMax("10");
// 	loc.addErrorPage("413", "/errors/413.html");
// 	ServerConfig cfg;
// 	setBasicConfig(cfg);
// 	cfg.addLocation(loc);
// 
// 	Processor proc(&cfg);
// 	Request req = makeRequestWithCL("POST", "/", 100);
// 	Decision res = proc.procRequest(req);
// 	ASSERT_EQ(res.getStatus(), PAYLOAD_TOO_LARGE);
// 	ASSERT_EQ(res.getHandler(), ERROR_PAGE);
// 	ASSERT_EQ(res.getErrorPage(), "/errors/413.html");
// }

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
	// Tester::runTest("Content-Length within limit", testContentLengthWithinLimit);
	// Tester::runTest("Content-Length equals limit allowed", testContentLengthEqualsLimit);
	// Tester::runTest("Content-Length exceeds limit -> 413", testContentLengthExceedsLimit);
	// Tester::runTest("client_max_body_size 0 = no limit", testContentLengthLimitZero);
	// Tester::runTest("Chunked ignored by processor", testContentLengthChunkedIgnoredByProcessor);
	// Tester::runTest("413 vs 405 priority (method not allowed)", testContentLengthMethodNotAllowedHasPriority);
	// Tester::runTest("No Content-Length -> no 413", testNoContentLengthNo413);
	Tester::runTest("stat regular file -> READ_FILE", testStatRegularFile);
	Tester::runTest("stat missing -> 404", testStatMissingFile);
	Tester::runTest("stat no permission -> 403", testStatNoPermission);
	Tester::runTest("stat dir with index -> READ_FILE", testStatDirWithIndex);
	Tester::runTest("stat dir autoindex on -> LIST_DIR", testStatDirAutoindexOn);
	Tester::runTest("stat dir autoindex off -> 403", testStatDirAutoindexOff);
	Tester::runTest("stat dir index missing + autoindex on -> LIST_DIR", testStatDirIndexMissingAutoindexOnFallback);
	Tester::runTest("stat dir index missing + autoindex off -> 403", testStatDirIndexMissingAutoindexOffFallback);
	Tester::runTest("POST missing file -> WRITE_FILE", testPostMissingFileCreates);
	Tester::runTest("POST existing file -> WRITE_FILE", testPostExistingFile);
	Tester::runTest("POST directory -> 403", testPostDirectoryForbidden);
	Tester::runTest("POST not allowed -> 405", testPostNotAllowedMissingFile);
	Tester::runTest("DELETE existing/missing", testDeleteExistingAndMissing);
	Tester::runTest("Path component not dir -> 404", testPathComponentNotDir);
	Tester::runTest("Decision copy keeps server config", testDecisionCopyKeepsServerConfig);
	Tester::runTest("Error page from location", testErrorPageFromLocation);
	Tester::runTest("Error page from server", testErrorPageFromServer);
	Tester::runTest("Error page location overrides server", testErrorPageLocationOverridesServer);
	Tester::runTest("Error page default empty", testErrorPageDefaultEmpty);
	Tester::runTest("Error page for 405", testErrorPageFor405);
	// Tester::runTest("Error page for 413", testErrorPageFor413);

	Tester::report();
}
