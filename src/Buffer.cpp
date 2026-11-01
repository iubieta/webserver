// Buffer.cpp
// ----------------------------------------------------------------------------
#include "../inc/Buffer.hpp"

// Constructors and destructors -----------------------------------------------

Buffer::Buffer() : data_(), offset_(0) {
}

Buffer::Buffer(const Buffer& other) {
	*this = other;
}

Buffer& Buffer::operator=(const Buffer&other) {
	if (this != &other) {
		data_ = other.data_;
		offset_ = other.offset_;
	}
	return *this;
}

Buffer::~Buffer() {}

// Public methods -------------------------------------------------------------

const char *Buffer::data() const {
	return &data_[offset_];
}

size_t Buffer::size() const {
	return data_.size() - offset_;
}

bool Buffer::empty() const {
	if (offset_ == data_.size()) {
		return true;
	}
	return false;
}

void Buffer::append(const std::string &str) {
	data_.append(str);
}

void Buffer::append(const char *str, size_t n) {
	data_.append(str, n);
}

void Buffer::consume(size_t n) 
{
	offset_ += n;
	if (offset_ >= data_.size()) 
	{
		data_ = "";
		offset_ = 0;
	}
}

size_t Buffer::find(const std::string &delimiter) const
{
	size_t	absolute_pos;

	absolute_pos = data_.find(delimiter, offset_);
	if (absolute_pos == std::string::npos)
		return (std::string::npos);
	return (absolute_pos - offset_);
}

std::string Buffer::substr(size_t pos, size_t n) const
{
	return (data_.substr(offset_ + pos, n));
}
