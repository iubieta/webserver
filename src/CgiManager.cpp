#include "../inc/CgiManager.hpp"

// Start with an empty registry.
CgiManager::CgiManager() {}

// Shutdown must resolve every child before destroying the manager.
CgiManager::~CgiManager() {
    while (!processes_.empty()) {
        if (!remove(processes_.begin()->first))
            std::terminate();
    }
}

// Allocate and prepare a process before publishing it in the registry.
CgiProcess* CgiManager::create(unsigned long connectionId, const std::string& body) {
    if (find(connectionId) != NULL)
        return NULL;
    CgiProcess* process = new CgiProcess();
    try {
        if (!process->prepare(connectionId, body)) {
            delete process;
            return NULL;
        }
        processes_.insert(std::make_pair(connectionId, process));
    } catch (...) {
        delete process;
        throw;
    }
    return process;
}

// Return a borrowed pointer, never transfer ownership.
CgiProcess* CgiManager::find(unsigned long connectionId) {
    ProcessMap::iterator it = processes_.find(connectionId);
    return it == processes_.end() ? NULL : it->second;
}

// Provide read-only lookup for a const manager.
const CgiProcess* CgiManager::find(unsigned long connectionId) const {
    ProcessMap::const_iterator it = processes_.find(connectionId);
    return it == processes_.end() ? NULL : it->second;
}

// Refuse removal until child supervision and descriptor cleanup are resolved.
// Future event registrations must also be removed before calling this method.
bool CgiManager::remove(unsigned long connectionId) {
    ProcessMap::iterator it = processes_.find(connectionId);
    if (it == processes_.end())
        return false;
    CgiProcess* process = it->second;
    if (process->pid() > 0 && !process->childExited())
        return false;
    if (process->state() == CgiProcess::CGI_RUNNING)
        return false;
    process->closePipes();
    delete process;
    processes_.erase(it);
    return true;
}

// Count registered processes, including those awaiting cleanup.
std::size_t CgiManager::size() const {
    return processes_.size();
}
