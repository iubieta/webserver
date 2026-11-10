// response_builder_test.cpp
// -----------------------------------------------------------------------------

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "Tester.hpp"
#include "../inc/Decision.hpp"
#include "../inc/ResponseBuilder.hpp"
#include "../inc/ServerConfig.hpp"
#include "../inc/file_utils.hpp"

// Helpers ---------------------------------------------------------------------
static std::string makeTempRoot(const std::string& suffix)
{
	char tmpl[128];

	std::strncpy(tmpl, "/tmp/webserv_rbXXXXXX", sizeof(tmpl) - 1);
	tmpl[sizeof(tmpl) - 1] = '\0';
	char* dir = mkdtemp(tmpl);
	if (!dir)
		return std::string("");
	(void)suffix;
	return std::string(dir);
}

static void removeDirRecursive(const std::string& path)
{
	if (path.empty())
		return;
	std::string cmd = "rm -rf " + path;
	std::system(cmd.c_str());
}

static bool writeFile(const std::string& path, const std::string& content)
{
	std::ofstream file(path.c_str(),
		std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file.is_open())
		return false;
	file.write(content.data(), static_cast<std::streamsize>(content.size()));
	return file.good();
}

static bool contains(const std::string& haystack, const std::string& needle)
{
	return haystack.find(needle) != std::string::npos;
}

static Decision makeErrorDecision(e_status_code status, const std::string& page)
{
	Decision decision;
	decision.setStatus(status);
	decision.setHandler(ERROR_PAGE);
	decision.setErrorPage(page);
	return decision;
}

static void assertContentType(const HttpResponse& res, const std::string& expected)
{
	const std::string* type = res.getHeader("Content-Type");

	ASSERT_EQ(type != NULL, true);
	if (type)
		ASSERT_EQ(*type, expected);
}

static Decision makeReadDecision(const std::string& path)
{
	Decision decision;

	decision.setStatus(OK);
	decision.setHandler(READ_FILE);
	decision.setDiskPath(path);
	return decision;
}

// Read file handling ----------------------------------------------------------

// Serving an existing regular file returns its bytes unchanged.
static void testReadExistingFile()
{
	std::string root = makeTempRoot("readok");
	if (root.empty())
		return;

	std::string path = root + "/index.html";
	std::string content = "<!DOCTYPE html><html><body>hello</body></html>\n";
	if (!writeFile(path, content))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeReadDecision(path);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), OK);
	ASSERT_EQ(res.getBody(), content);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// The Content-Type header must follow the file extension.
static void testReadFileMimeTypes()
{
	struct { const char* extension; const char* type; } cases[] = {
		{ "html", "text/html" },
		{ "htm", "text/html" },
		{ "shtml", "text/html" },
		{ "css", "text/css" },
		{ "xml", "text/xml" },
		{ "txt", "text/plain" },
		{ "gif", "image/gif" },
		{ "jpeg", "image/jpeg" },
		{ "jpg", "image/jpeg" },
		{ "png", "image/png" },
		{ "svg", "image/svg" },
		{ "webp", "image/webp" },
		{ "unknownext", "application/octet-stream" }
	};

	std::string root = makeTempRoot("readmime");
	if (root.empty())
		return;

	ResponseBuilder builder;
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
	{
		std::string path = root + "/file." + cases[i].extension;
		if (!writeFile(path, "data"))
		{
			removeDirRecursive(root);
			return;
		}
		Decision decision = makeReadDecision(path);
		HttpResponse res = builder.execute(decision);

		ASSERT_EQ(res.getStatusCode(), OK);
		assertContentType(res, cases[i].type);
	}

	removeDirRecursive(root);
}

// Binary files must be served byte by byte, NUL bytes included.
static void testReadBinaryFile()
{
	std::string root = makeTempRoot("readbinary");
	if (root.empty())
		return;

	std::string path = root + "/data.bin";
	std::string content("a\0b\0c", 5);
	if (!writeFile(path, content))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeReadDecision(path);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), OK);
	ASSERT_EQ(res.getBody().size(), 5u);
	ASSERT_EQ(res.getBody(), content);
	assertContentType(res, "application/octet-stream");

	removeDirRecursive(root);
}

// Files larger than the read buffer must be served completely.
static void testReadLargeFile()
{
	std::string root = makeTempRoot("readlarge");
	if (root.empty())
		return;

	std::string content;
	for (size_t i = 0; i < (3 * 32768) + 123; ++i)
		content += static_cast<char>('a' + (i % 26));

	std::string path = root + "/large.txt";
	if (!writeFile(path, content))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeReadDecision(path);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), OK);
	ASSERT_EQ(res.getBody().size(), content.size());
	ASSERT_EQ(res.getBody(), content);

	removeDirRecursive(root);
}

// A zero-length file is a valid representation: 200 OK with an empty body.
static void testReadEmptyFile()
{
	std::string root = makeTempRoot("readempty");
	if (root.empty())
		return;

	std::string path = root + "/empty.txt";
	if (!writeFile(path, ""))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeReadDecision(path);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), OK);
	ASSERT_EQ(res.getBody(), std::string(""));
	ASSERT_EQ(contains(res.build(), "content-length: 0\r\n"), true);

	removeDirRecursive(root);
}

// The serialized response must carry the file bytes and the right length.
static void testReadFileResponseBuild()
{
	std::string root = makeTempRoot("readbuild");
	if (root.empty())
		return;

	std::string path = root + "/page.txt";
	std::string content = "hello from disk\n";
	if (!writeFile(path, content))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeReadDecision(path);
	HttpResponse res = builder.execute(decision);

	std::ostringstream contentLength;
	contentLength << "content-length: " << content.size() << "\r\n";

	std::string raw = res.build();
	ASSERT_EQ(raw.find("HTTP/1.1 200 OK\r\n"), 0u);
	ASSERT_EQ(contains(raw, contentLength.str()), true);
	ASSERT_EQ(contains(raw, "\r\n\r\n" + content), true);

	removeDirRecursive(root);
}

// A missing file is a 404 and uses the default error page.
static void testReadMissingFile()
{
	std::string root = makeTempRoot("readmiss");
	if (root.empty())
		return;

	ResponseBuilder builder;
	Decision decision = makeReadDecision(root + "/missing.html");
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), NOT_FOUND);
	ASSERT_EQ(contains(res.getBody(), "<h1>404 Not Found</h1>"), true);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// A directory is not a regular file and must not be served as one.
static void testReadDirectory()
{
	std::string root = makeTempRoot("readdir");
	if (root.empty())
		return;

	std::string directory = root + "/assets";
	if (mkdir(directory.c_str(), 0755) != 0)
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeReadDecision(directory);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), FORBIDDEN);
	ASSERT_EQ(contains(res.getBody(), "<h1>403 Forbidden</h1>"), true);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// A 404 uses the configured error page when one exists.
static void testReadMissingFileUsesConfiguredErrorPage()
{
	std::string root = makeTempRoot("read404page");
	if (root.empty())
		return;

	std::string content = "<h1>custom not found</h1>\n";
	if (mkdir((root + "/errors").c_str(), 0755) != 0
		|| !writeFile(root + "/errors/404.html", content))
	{
		removeDirRecursive(root);
		return;
	}

	ServerConfig cfg;
	cfg.setRoot(root + "/");
	cfg.addErrorPage("404", "/errors/404.html");

	ResponseBuilder builder;
	Decision decision = makeReadDecision(root + "/missing.html");
	decision.setServerConfig(&cfg);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), NOT_FOUND);
	ASSERT_EQ(res.getBody(), content);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// A file that cannot be opened is a 403, not a 404.
static void testReadUnreadableFile()
{
	if (geteuid() == 0)
		return;

	std::string root = makeTempRoot("readperm");
	if (root.empty())
		return;

	std::string path = root + "/secret.txt";
	if (!writeFile(path, "secret\n") || chmod(path.c_str(), 0000) != 0)
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeReadDecision(path);
	HttpResponse res = builder.execute(decision);

	chmod(path.c_str(), 0644);

	ASSERT_EQ(res.getStatusCode(), FORBIDDEN);
	ASSERT_EQ(contains(res.getBody(), "<h1>403 Forbidden</h1>"), true);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// Unexpected filesystem failures are a 500, not a client error.
static void testReadFileSystemError()
{
	std::string root = makeTempRoot("readloop");
	if (root.empty())
		return;

	std::string path = root + "/loop";
	if (symlink(path.c_str(), path.c_str()) != 0)
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeReadDecision(path);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), INTERNAL_ERROR);
	ASSERT_EQ(contains(res.getBody(),
		"<h1>500 Internal Server Error</h1>"), true);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// Error page handling ---------------------------------------------------------

// A configured error page is loaded into the response body.
static void testConfiguredErrorPage()
{
	std::string root = makeTempRoot("configured");
	if (root.empty())
		return;

	std::string page = root + "/404.html";
	std::string content = "<h1>custom not found</h1>\n";
	if (!writeFile(page, content))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeErrorDecision(NOT_FOUND, page);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), NOT_FOUND);
	ASSERT_EQ(res.getBody(), content);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// A missing error page falls back to the generated default body.
static void testMissingErrorPageUsesDefaultBody()
{
	std::string root = makeTempRoot("missing");
	if (root.empty())
		return;

	ResponseBuilder builder;
	Decision decision = makeErrorDecision(NOT_FOUND, root + "/missing.html");
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), NOT_FOUND);
	ASSERT_EQ(contains(res.getBody(), "<h1>404 Not Found</h1>"), true);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// Without an error page configured the default body is used.
static void testNoErrorPageUsesDefaultBody()
{
	ResponseBuilder builder;
	Decision decision = makeErrorDecision(INTERNAL_ERROR, "");
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), INTERNAL_ERROR);
	ASSERT_EQ(contains(res.getBody(), "<h1>500 Internal Server Error</h1>"),
		true);
	assertContentType(res, "text/html");
}

// Directories are not regular files and must not be served as error pages.
static void testDirectoryErrorPageUsesDefaultBody()
{
	std::string root = makeTempRoot("directory");
	if (root.empty())
		return;

	std::string directory = root + "/errors";
	if (mkdir(directory.c_str(), 0755) != 0)
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeErrorDecision(FORBIDDEN, directory);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(contains(res.getBody(), "<h1>403 Forbidden</h1>"), true);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// An empty error page is valid: it must not fall back to the default body.
static void testEmptyErrorPageIsValid()
{
	std::string root = makeTempRoot("empty");
	if (root.empty())
		return;

	std::string page = root + "/empty.html";
	if (!writeFile(page, ""))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeErrorDecision(NOT_FOUND, page);
	HttpResponse res = builder.execute(decision);

	ASSERT_EQ(res.getStatusCode(), NOT_FOUND);
	ASSERT_EQ(res.getBody(), std::string(""));
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// Content-Type follows the configured error page extension.
static void testErrorPageMimeType()
{
	std::string root = makeTempRoot("mime");
	if (root.empty())
		return;

	std::string page = root + "/404.txt";
	if (!writeFile(page, "plain error\n"))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeErrorDecision(NOT_FOUND, page);
	HttpResponse res = builder.execute(decision);

	assertContentType(res, "text/plain");

	removeDirRecursive(root);
}

// Unknown extensions use the MIME fallback.
static void testErrorPageUnknownMimeType()
{
	std::string root = makeTempRoot("unknownmime");
	if (root.empty())
		return;

	std::string page = root + "/404.unknownext";
	if (!writeFile(page, "data\n"))
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeErrorDecision(NOT_FOUND, page);
	HttpResponse res = builder.execute(decision);

	assertContentType(res, "application/octet-stream");

	removeDirRecursive(root);
}

// The full response must contain the body and the right Content-Length.
static void testErrorResponseBuild()
{
	ResponseBuilder builder;
	Decision decision = makeErrorDecision(NOT_FOUND, "");
	HttpResponse res = builder.execute(decision);

	std::string raw = res.build();
	std::ostringstream contentLength;

	contentLength << "content-length: " << res.getBody().size() << "\r\n";

	ASSERT_EQ(raw.find("HTTP/1.1 404 Not Found\r\n"), 0u);
	ASSERT_EQ(contains(raw, contentLength.str()), true);
	ASSERT_EQ(contains(raw, "\r\n\r\n" + res.getBody()), true);
}

// Permission errors fall back to the default body. Root bypasses permissions,
// so the test is skipped when running as root.
static void testUnreadableErrorPageUsesDefaultBody()
{
	if (geteuid() == 0)
		return;

	std::string root = makeTempRoot("permissions");
	if (root.empty())
		return;

	std::string secret = root + "/secret";
	std::string page = secret + "/404.html";
	if (mkdir(secret.c_str(), 0755) != 0 || !writeFile(page, "secret\n"))
	{
		removeDirRecursive(root);
		return;
	}
	if (chmod(secret.c_str(), 0000) != 0)
	{
		removeDirRecursive(root);
		return;
	}

	ResponseBuilder builder;
	Decision decision = makeErrorDecision(NOT_FOUND, page);
	HttpResponse res = builder.execute(decision);

	chmod(secret.c_str(), 0755);

	ASSERT_EQ(contains(res.getBody(), "<h1>404 Not Found</h1>"), true);
	assertContentType(res, "text/html");

	removeDirRecursive(root);
}

// File helpers ----------------------------------------------------------------

// loadFile must distinguish missing files, directories and regular files.
static void testLoadFileResults()
{
	std::string root = makeTempRoot("fileutils");
	if (root.empty())
		return;

	std::string directory = root + "/dir";
	std::string file = root + "/file.txt";
	std::string out;

	if (mkdir(directory.c_str(), 0755) != 0 || !writeFile(file, "hello"))
	{
		removeDirRecursive(root);
		return;
	}

	ASSERT_EQ(checkFile(root + "/missing.txt"), FILE_NOT_FOUND);
	ASSERT_EQ(checkFile(directory), FILE_FORBIDDEN);
	ASSERT_EQ(checkFile(file), FILE_OK);

	ASSERT_EQ(loadFile(root + "/missing.txt", out), FILE_NOT_FOUND);
	ASSERT_EQ(loadFile(directory, out), FILE_FORBIDDEN);

	out = "stale";
	ASSERT_EQ(loadFile(file, out), FILE_OK);
	ASSERT_EQ(out, std::string("hello"));

	removeDirRecursive(root);
}

// Binary content, including NUL bytes, must be preserved.
static void testLoadFileBinaryContent()
{
	std::string root = makeTempRoot("binary");
	if (root.empty())
		return;

	std::string path = root + "/data.bin";
	std::string content("a\0b\0c", 5);
	if (!writeFile(path, content))
	{
		removeDirRecursive(root);
		return;
	}

	std::string out;
	ASSERT_EQ(loadFile(path, out), FILE_OK);
	ASSERT_EQ(out.size(), 5u);
	ASSERT_EQ(out, content);

	removeDirRecursive(root);
}

// An empty regular file is a successful read with an empty body.
static void testLoadFileEmpty()
{
	std::string root = makeTempRoot("emptyfile");
	if (root.empty())
		return;

	std::string path = root + "/empty.bin";
	if (!writeFile(path, ""))
	{
		removeDirRecursive(root);
		return;
	}

	std::string out = "stale";
	ASSERT_EQ(loadFile(path, out), FILE_OK);
	ASSERT_EQ(out, std::string(""));

	removeDirRecursive(root);
}

// Entry point -----------------------------------------------------------------
void response_builder_tests()
{
	Tester::runTest("read file: existing file",
		testReadExistingFile);
	Tester::runTest("read file: mime from extension",
		testReadFileMimeTypes);
	Tester::runTest("read file: binary content",
		testReadBinaryFile);
	Tester::runTest("read file: large file",
		testReadLargeFile);
	Tester::runTest("read file: empty file is 200",
		testReadEmptyFile);
	Tester::runTest("read file: build body + content-length",
		testReadFileResponseBuild);
	Tester::runTest("read file: missing -> 404 default",
		testReadMissingFile);
	Tester::runTest("read file: directory -> 403",
		testReadDirectory);
	Tester::runTest("read file: missing uses configured 404 page",
		testReadMissingFileUsesConfiguredErrorPage);
	Tester::runTest("read file: unreadable -> 403",
		testReadUnreadableFile);
	Tester::runTest("read file: filesystem error -> 500",
		testReadFileSystemError);
	Tester::runTest("error page: configured file",
		testConfiguredErrorPage);
	Tester::runTest("error page: missing -> default",
		testMissingErrorPageUsesDefaultBody);
	Tester::runTest("error page: not configured -> default",
		testNoErrorPageUsesDefaultBody);
	Tester::runTest("error page: directory -> default",
		testDirectoryErrorPageUsesDefaultBody);
	Tester::runTest("error page: empty file is valid",
		testEmptyErrorPageIsValid);
	Tester::runTest("error page: mime from extension",
		testErrorPageMimeType);
	Tester::runTest("error page: unknown mime fallback",
		testErrorPageUnknownMimeType);
	Tester::runTest("error page: build body + content-length",
		testErrorResponseBuild);
	Tester::runTest("error page: unreadable -> default",
		testUnreadableErrorPageUsesDefaultBody);
	Tester::runTest("file utils: check/load results",
		testLoadFileResults);
	Tester::runTest("file utils: binary content",
		testLoadFileBinaryContent);
	Tester::runTest("file utils: empty file",
		testLoadFileEmpty);

	Tester::report();
}
