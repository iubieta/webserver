#ifndef CGI_MANAGER_HPP
#define CGI_MANAGER_HPP

#include "CgiProcess.hpp"
#include <map>
#include <exception>

// Own CGI processes by stable connection identifier.
class CgiManager {
public:
    CgiManager();
    ~CgiManager();
    CgiProcess* create(unsigned long connectionId, const std::string& body);
    CgiProcess* find(unsigned long connectionId);
    const CgiProcess* find(unsigned long connectionId) const;
    bool remove(unsigned long connectionId);
    std::size_t size() const;
private:
    typedef std::map<unsigned long, CgiProcess*> ProcessMap;
    ProcessMap processes_;
    CgiManager(const CgiManager& other);
    CgiManager& operator=(const CgiManager& other);
};
#endif
