#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

namespace accel {

class TcpConnection {
public:
    explicit TcpConnection(int fd) : fd_(fd) { set_nodelay(); }

    TcpConnection(const TcpConnection&)            = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    TcpConnection(TcpConnection&& o) noexcept : fd_(o.fd_.exchange(-1)) {}

    TcpConnection& operator=(TcpConnection&& o) noexcept {
        if (this != &o) {
            close_fd();
            fd_.store(o.fd_.exchange(-1));
        }
        return *this;
    }

    ~TcpConnection() { close_fd(); }

    int  fd()    const { return fd_.load(); }
    bool valid() const { return fd_.load() >= 0; }

    void close_fd() {
        int fd = fd_.exchange(-1);
        if (fd >= 0) ::close(fd);
    }

    // Returns false on EOF or error
    bool send_all(const std::string& data) const {
        const char* ptr       = data.data();
        size_t      remaining = data.size();
        while (remaining > 0) {
            ssize_t n = ::send(fd_.load(), ptr, remaining, MSG_NOSIGNAL);
            if (n <= 0) return false;
            ptr       += n;
            remaining -= static_cast<size_t>(n);
        }
        return true;
    }

    // Reads until newline; returns false on EOF or error
    bool read_line(std::string& line) const {
        line.clear();
        char c;
        while (true) {
            ssize_t n = ::recv(fd_.load(), &c, 1, 0);
            if (n <= 0) return false;
            if (c == '\n') return true;
            line += c;
        }
    }

private:
    std::atomic<int> fd_{-1};

    void set_nodelay() {
        int fd = fd_.load();
        if (fd < 0) return;
        int flag = 1;
        ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    }
};

class TcpListener {
public:
    explicit TcpListener(uint16_t port, int backlog = 10) {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0)
            throw std::runtime_error(std::string("socket: ") + strerror(errno));

        int opt = 1;
        ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(port);

        if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
            throw std::runtime_error(std::string("bind port ") +
                                     std::to_string(port) + ": " + strerror(errno));

        if (::listen(fd_, backlog) < 0)
            throw std::runtime_error(std::string("listen: ") + strerror(errno));
    }

    ~TcpListener() {
        if (fd_ >= 0) ::close(fd_);
    }

    TcpListener(const TcpListener&)            = delete;
    TcpListener& operator=(const TcpListener&) = delete;

    TcpConnection accept() const {
        sockaddr_in peer{};
        socklen_t   len = sizeof(peer);
        int cfd = ::accept(fd_, reinterpret_cast<sockaddr*>(&peer), &len);
        if (cfd < 0)
            throw std::runtime_error(std::string("accept: ") + strerror(errno));
        return TcpConnection{cfd};
    }

    static std::string peer_ip(int cfd) {
        sockaddr_in peer{};
        socklen_t   len = sizeof(peer);
        if (::getpeername(cfd, reinterpret_cast<sockaddr*>(&peer), &len) < 0)
            return "unknown";
        char buf[INET_ADDRSTRLEN];
        ::inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf));
        return buf;
    }

private:
    int fd_ = -1;
};

inline TcpConnection connect_to(const std::string& host, uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error(std::string("socket: ") + strerror(errno));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        ::close(fd);
        throw std::runtime_error("Invalid address: " + host);
    }

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        throw std::runtime_error(std::string("connect ") + host + ":" +
                                 std::to_string(port) + ": " + strerror(errno));
    }
    return TcpConnection{fd};
}

} // namespace accel
