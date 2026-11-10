// file_utils.hpp
//------------------------------------------------------------------------------
#ifndef FILE_UTILS_HPP
# define FILE_UTILS_HPP

#include <string>

#ifndef FILE_BUFF_LEN
#define FILE_BUFF_LEN 32768 // 32KB
#endif

enum e_file_result {
	FILE_OK,
	FILE_NOT_FOUND,
	FILE_FORBIDDEN,
	FILE_ERROR,
};

e_file_result checkFile(const std::string &path);
e_file_result loadFile(const std::string& path, std::string& out);

#endif // !FILE_UTILS_HPP
