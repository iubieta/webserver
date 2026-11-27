#ifndef CGI_DEBUG_HPP
#define CGI_DEBUG_HPP

#ifndef CGI_DEBUG
# define CGI_DEBUG 0
#endif

#if CGI_DEBUG > 0
# include <iostream>
# include <cerrno>
# include <string>

// Escape control bytes and bound diagnostic output without changing CGI data.
inline void cgiDebugPreview(std::ostream& out, const std::string& value) {
    const char* hex = "0123456789abcdef";
    const std::size_t limit = value.size() < 256 ? value.size() : 256;
    for (std::size_t i = 0; i < limit; ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (c >= 32 && c <= 126 && c != '\\')
            out << static_cast<char>(c);
        else
            out << "\\x" << hex[c >> 4] << hex[c & 15];
    }
    if (limit < value.size())
        out << "... [truncated]";
}

// Diagnostics must not change control flow or errno in the parent.
# define CGI_TRACE(level, message) do { \
    if (CGI_DEBUG >= (level)) { \
        const int cgi_saved_errno = errno; \
        try { std::cerr << "[CGI] " << message << '\n'; } catch (...) {} \
        errno = cgi_saved_errno; \
    } \
} while (0)
# define CGI_DETAIL(id, label, value) do { \
    if (CGI_DEBUG >= 2) { \
        const int cgi_saved_errno = errno; \
        try { std::cerr << "[CGI] " << (label) << " id=" << (id) << " bytes=" \
            << (value).size() << " data="; cgiDebugPreview(std::cerr, (value)); \
            std::cerr << '\n'; } catch (...) {} \
        errno = cgi_saved_errno; \
    } \
} while (0)
#else
# define CGI_TRACE(level, message) do {} while (0)
# define CGI_DETAIL(id, label, value) do {} while (0)
#endif
#endif
