// mime_typse.hpp
// ----------------------------------------------------------------------------

#ifndef MIME_TYPES_HPP

#define MIME_TYPES_HPP

#include <string>

#define DEFAULT_MIME_TYPE "aplication/octet-stream"
#define DEFAULT_MIME_EXT "bin"

typedef struct s_mime_type {
	const char* extension_;
	const char* type_;
} t_mime_type;

const std::string getMimeType(const std::string &extension);
const std::string getExtension(const std::string &type);

#endif // !MIME_TYPES_HPP
