// file_utils.cpp
//------------------------------------------------------------------------------

#include "../inc/file_utils.hpp"
#include <cerrno>
#include <cstddef>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>

static e_file_result fileError(int err) {
	if (err == EACCES || err == EPERM || err == EROFS)
		return FILE_FORBIDDEN;
	if (err == EISDIR)
		return FILE_FORBIDDEN;
	if (err == ENOTEMPTY || err == EEXIST)
		return FILE_FORBIDDEN;
	if (err == ENOTDIR || err == ENOENT)
		return FILE_NOT_FOUND;
	return FILE_ERROR;
}

e_file_result checkFile(const std::string &path) {
	struct stat st;
	// stat error
	if (stat(path.c_str(), &st) == -1)
		return fileError(errno);
	// If the path doesnt correspond to regular file
	if (!S_ISREG(st.st_mode))
		return FILE_FORBIDDEN;
	return FILE_OK;
}

e_file_result loadFile(const std::string& path, std::string& out) {
	e_file_result file_result = checkFile(path);
	if (file_result != FILE_OK)
		return file_result;
	
	int fd = open(path.c_str(), O_RDONLY);
	if (fd == -1)
		return fileError(errno);

	// Read the content
	out.clear();
	char read_buff[FILE_BUFF_LEN];
	while (true) {
		ssize_t n = read(fd, read_buff, FILE_BUFF_LEN);

		// Read error
		if (n < 0) {
			// TODO: report error
			out.clear();
			close(fd);
			return FILE_ERROR;
		}
		
		// File end
		if (n == 0)
			break;

		// Normal read
		out.append(read_buff, static_cast<size_t>(n));
	}
	close(fd);
	return FILE_OK;
}

e_file_result writeToFile(const std::string& path, std::string& in, bool& existed) {
	e_file_result fr = checkFile(path);
	switch (fr) {
		case FILE_OK:
			existed = true;
			break;
		case FILE_NOT_FOUND:
			existed = false;
			break;
		default:
			return fr;
	}
	
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		return fileError(errno);

	const char *data = in.data();
	size_t size = in.size();
	size_t written = 0;
	while (written < size) {
		ssize_t n = write(fd, data + written, size - written);

		if (n <= 0) {
			// TODO: report error
			close(fd);
			return FILE_ERROR;
		}
		
		written += static_cast<size_t>(n);
	}
	close(fd);
	return FILE_OK;
}

e_file_result removeFileAt(const std::string& path) {
	e_file_result fr = checkFile(path);
	if (fr != FILE_OK)
		return fr;
	
	if (std::remove(path.c_str()) != 0)
		return fileError(errno);
	
	return  FILE_OK;
}
