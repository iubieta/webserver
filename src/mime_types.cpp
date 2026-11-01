// mime_types.cpp
// ----------------------------------------------------------------------------

#include "../inc/mime_types.hpp"
#include <iostream>
#include <string>

static const t_mime_type mime_types[] = 
{
	// Text
	{ "html",	"text/html" },
	{ "htm",	"text/html" },
	{ "shtml",	"text/html" },
	{ "css",	"text/css" },
	{ "xml",	"text/xml" },
	{ "txt",	"text/plain" },

	// Images
	{ "gif",	"image/gif" },
	{ "jpeg",	"image/jpeg" },
	{ "jpg",	"image/jpeg" },
	{ "png", 	"image/png" },
	{ "svg", 	"image/svg" },
	{ "svg", 	"image/svgz" },
	{ "webp",	"image/webp" },

	// TODO: Apps / exes (CGI)
};

const std::string getMimeType(const std::string &extension)
{
	for (size_t i = 0; i < sizeof(mime_types) / sizeof(t_mime_type); ++i)
	{
		if (extension == mime_types[i].extension_)
			return mime_types[i].type_;
	}
	return DEFAULT_MIME_TYPE;
}

const std::string getExtension(const std::string &type)
{
	for (size_t i = 0; i < sizeof(mime_types) / sizeof(t_mime_type); ++i)
	{
		if (type == mime_types[i].type_)
			return mime_types[i].extension_;
	}
	return DEFAULT_MIME_EXT;
}

// int main() {
// 	std::cout << getMimeType("xml") << std::endl;
// 	std::cout << getMimeType("unknown") << std::endl;
// 	
// 	std::cout << getExtension("text/html") << std::endl;
// 	std::cout << getExtension("unknown") << std::endl;
// }
