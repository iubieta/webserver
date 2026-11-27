#include "CgiManager.hpp"
#include "CgiDebug.hpp"
#include "Request.hpp"
#include "Decision.hpp"
#include "ServerConfig.hpp"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <map>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

typedef CgiManager M;

// Test-only core: one epoll instance and one token allocator for all endpoints.
class Harness {
public:
    M manager;
    int ep;
    M::EventToken token;
    struct Client { unsigned long id; int fd; bool echo; bool pending; };
    std::map<M::EventToken, Client> clients;
    std::map<unsigned long, std::string> results;
    std::map<unsigned long, M::ResultStatus> failures;
    unsigned int echoes;
    Harness() : ep(epoll_create(16)), token(0), echoes(0) {
        assert(ep >= 0); assert(fcntl(ep, F_SETFD, FD_CLOEXEC) == 0);
    }
    ~Harness() { assert(clients.empty()); assert(manager.shutdownComplete()); close(ep); }
    // Keep every test socket out of the executed child's descriptor table.
    int connectClient(unsigned long id, bool echo) {
        int pair[2]; assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
        for (int i = 0; i < 2; ++i) {
            assert(fcntl(pair[i], F_SETFD, FD_CLOEXEC) == 0);
            assert(fcntl(pair[i], F_SETFL, O_NONBLOCK) == 0);
        }
        Client c = {id, pair[0], echo, false};
        clients[++token] = c;
        add(pair[0], EPOLLIN | EPOLLRDHUP, token);
        CGI_TRACE(2, "client id=" << id << " token=" << token << " fd=" << pair[0]);
        return pair[1];
    }
    void add(int fd, uint32_t flags, M::EventToken t) {
        epoll_event e; std::memset(&e, 0, sizeof(e)); e.events = flags; e.data.u64 = t;
        assert(epoll_ctl(ep, EPOLL_CTL_ADD, fd, &e) == 0);
        CGI_TRACE(2, "harness epoll ADD token=" << t << " fd=" << fd << " flags=" << flags);
    }
    void registerPending() {
        M::EventInterest i;
        while (manager.nextPendingInterest(i)) {
            assert(manager.bindEvent(++token, i.connectionId, i.channel) == M::CGI_BIND_OK);
            add(i.fd, i.flags, token);
        }
    }
    // Disconnect by logical identity, not by the reusable descriptor number.
    void disconnect(M::EventToken t) {
        std::map<M::EventToken, Client>::iterator it = clients.find(t);
        assert(it != clients.end());
        manager.detach(it->second.id);
        assert(epoll_ctl(ep, EPOLL_CTL_DEL, it->second.fd, NULL) == 0);
        close(it->second.fd); clients.erase(it);
    }
    void socketEvent(M::EventToken t, uint32_t flags) {
        Client& c = clients.find(t)->second;
        if (flags & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) { disconnect(t); return; }
        if ((flags & EPOLLIN) && !c.pending) {
            char byte;
            const ssize_t n = recv(c.fd, &byte, 1, 0);
            if (n <= 0) { disconnect(t); return; }
            assert(c.echo && byte == 'p');
            c.pending = true;
            modify(c.fd, EPOLLOUT | EPOLLRDHUP, t);
        } else if ((flags & EPOLLOUT) && c.pending) {
            const ssize_t n = send(c.fd, "q", 1, 0);
            if (n <= 0) { disconnect(t); return; }
            c.pending = false;
            ++echoes;
            modify(c.fd, EPOLLIN | EPOLLRDHUP, t);
        }
    }
    // Change interest only when the pending response changes.
    void modify(int fd, uint32_t flags, M::EventToken t) {
        epoll_event e; std::memset(&e, 0, sizeof(e)); e.events = flags; e.data.u64 = t;
        assert(epoll_ctl(ep, EPOLL_CTL_MOD, fd, &e) == 0);
    }

    void cycle() {
        epoll_event ev[16]; int n = epoll_wait(ep, ev, 16, 5);
        assert(n >= 0 || errno == EINTR);
        for (int j = 0; j < n; ++j) {
            M::EventToken t = ev[j].data.u64;
            if (clients.count(t)) socketEvent(t, ev[j].events);
            else manager.dispatchEvent(t, ev[j].events);
        }
        assert(manager.tick() != M::CGI_SUPERVISION_ERROR);
        M::EventToken t; M::EventBinding b;
        while (manager.nextRetirement(t, b)) {
            assert(epoll_ctl(ep, EPOLL_CTL_DEL, b.fd, NULL) == 0);
            CGI_TRACE(2, "harness epoll DEL token=" << t << " fd=" << b.fd);
            assert(manager.retireEvent(t));
        }
        assert(manager.tick() != M::CGI_SUPERVISION_ERROR);
        registerPending();
        unsigned long id;
        while (manager.nextReady(id)) {
            bool live = false;
            for (std::map<M::EventToken, Client>::const_iterator it = clients.begin(); it != clients.end(); ++it)
                if (it->second.id == id) live = true;
            assert(live);
            M::CgiResult r; assert(manager.takeResult(id, r));
            if (r.status == M::CGI_RESULT_SUCCESS)
                assert(results.insert(std::make_pair(id, r.output)).second);
            else
                assert(failures.insert(std::make_pair(id, r.status)).second);
        }
    }
private:
    Harness(const Harness&);
    Harness& operator=(const Harness&);
};

int main() {
    signal(SIGPIPE, SIG_IGN);
    Harness h;
    ServerConfig server; server.setListen("8080"); server.setServerName("localhost");
    Decision d; d.setHandler(CGI); d.setServerConfig(&server);
    d.setCgiInterpreter("/bin/cat"); d.setDiskPath("/dev/stdin"); d.setCgiScriptName("/echo");
    Request r; r.setMethod("POST"); r.setVersion("HTTP/1.1");
    r.setBody(std::string(256 * 1024, 'x') + std::string("\0end", 4));
    CgiEnvironmentBuilder::ConnectionInfo c; c.remoteAddress = "192.0.2.10";
    int a = h.connectClient(101, false);
    int b = h.connectClient(102, false);
    int ping = h.connectClient(103, true);
    assert(h.manager.start(101, r, d, c) == M::CGI_START_STARTED);
    assert(h.manager.start(102, r, d, c) == M::CGI_START_STARTED);
    h.registerPending();
    // A real socket closure must abandon only its own CGI.
    close(a);
    assert(send(ping, "p", 1, 0) == 1);
    for (int i = 0; i < 2000 && h.echoes == 0; ++i) h.cycle();
    assert(h.echoes == 1 && h.manager.contains(102));
    char reply; assert(recv(ping, &reply, 1, 0) == 1 && reply == 'q');
    for (int i = 0; i < 2000 && (h.manager.contains(101) || !h.results.count(102)); ++i) h.cycle();
    assert(!h.manager.contains(101) && !h.results.count(101));
    assert(h.results.count(102) && h.results[102] == r.getBody());
    // Resolve a relative script before changing only the child directory.
    int cwd = h.connectClient(105, false);
    d.setCgiInterpreter("/bin/sh");
    d.setDiskPath("tests/CGI/fixtures/cwd.sh");
    Request get; get.setMethod("GET"); get.setVersion("HTTP/1.1");
    assert(h.manager.start(105, get, d, c) == M::CGI_START_STARTED);
    h.registerPending();
    for (int i = 0; i < 2000 && !h.results.count(105); ++i) h.cycle();
    assert(h.results[105] == "Content-Type: text/plain\r\n\r\nrelative file OK\n");
    close(cwd);
    // A missing working directory must report a launch error, not success.
    int missing = h.connectClient(106, false);
    d.setDiskPath("tests/CGI/fixtures/nonexistent-directory/script.sh");
    assert(h.manager.start(106, get, d, c) == M::CGI_START_STARTED);
    h.registerPending();
    for (int i = 0; i < 2000 && !h.failures.count(106); ++i) h.cycle();
    assert(h.failures.count(106) && h.failures[106] == M::CGI_RESULT_EXEC_ERROR);
    close(missing);
    d.setCgiInterpreter("/bin/cat"); d.setDiskPath("/dev/stdin");
    // Start another execution and shut down while its endpoints are registered.
    int last = h.connectClient(104, false);
    assert(h.manager.start(104, r, d, c) == M::CGI_START_STARTED);
    h.registerPending(); h.manager.beginShutdown();
    close(last); close(b); close(ping);
    for (int i = 0; i < 2000 && (!h.manager.shutdownComplete() || !h.clients.empty()); ++i) h.cycle();
    assert(h.manager.shutdownComplete() && h.clients.empty());
    assert(h.manager.size() == 0 && h.manager.eventCount() == 0 && h.manager.bufferedOutputBytes() == 0);
    assert(h.results.size() == 2 && h.failures.size() == 1);
    int status; assert(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD);
}
