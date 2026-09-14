// Buffer.cpp
// ----------------------------------------------------------------------------

// Constructors and destructors -----------------------------------------------

Buffer::Buffer() : data_(), offset_(0) {
}

Buffer::Buffer(const Buffer& other) {
	*this = other;
}

Buffer::Buffer& operator=(const Buffer&other) {
	if (this != &other) {
		data_ = other.data_;
		offset_ = other.offset_;
	}
}

Buffer::~Buffer() {}

// Public methods -------------------------------------------------------------

const char *Buffer::data() const {
	return &data_ + offset_;
}

size_t Buffer::size() const {
	return data_.size() - offset_;
}

bool Buffer::empty() const {
	if (offset == data.size()) {
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

void Buffer::consume(size_t n) {
	offset_ += n;
	if (offset >= data.size()) {
		data = "";
		offset = 0;
	}
}
