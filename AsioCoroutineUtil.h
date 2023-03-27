#ifndef TINY_HTTP_SERVER_ASIO_COROUTINE_UTIL_H
#define TINY_HTTP_SERVER_ASIO_COROUTINE_UTIL_H

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "Executor.h"
#include "Lazy.h"

#define asio boost::asio
#define tcp asio::ip::tcp

class AsioExecutor : public Executor {
public:
    AsioExecutor(asio::io_context& ioContext) : ioContext_(ioContext) {}

    virtual bool schedule(Func func) override {
        asio::post(ioContext_, std::move(func));
        return true;
    }

private:
    asio::io_context& ioContext_;
};

class AcceptorAwaiter {
public:
    AcceptorAwaiter(tcp::acceptor& acceptor, tcp::socket& socket)
        : _acceptor(acceptor), _socket(socket) {}

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        _acceptor.async_accept(_socket, [this, handle](auto ec) {
            _ec = ec;
            handle.resume();
        });
    }

    std::error_code await_resume() noexcept { return _ec; }

    AcceptorAwaiter coAwait(Executor* executor) noexcept { return *this; }

private:
    tcp::acceptor& _acceptor;
    tcp::socket& _socket;
    std::error_code _ec{};
};

inline Lazy<std::error_code> asyncAccept(tcp::acceptor& acceptor, tcp::socket& socket) noexcept {
    co_return co_await AcceptorAwaiter(acceptor, socket);
}

template <typename Socket, typename AsioBuffer>
struct ReadAwaiter {
public:
    ReadAwaiter(Socket& socket, AsioBuffer& buffer) : _socket(socket), _buffer(buffer) {}
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        asio::async_read(_socket, _buffer, [this, handle](auto ec, auto size) mutable {
            _ec = ec;
            _size = size;
            handle.resume();
        });
    }
    auto await_resume() { return std::make_pair(_ec, _size); }
    auto coAwait(Executor* executor) noexcept { return std::move(*this); }

private:
    Socket& _socket;
    AsioBuffer& _buffer;
    std::error_code _ec{};
    size_t _size{0};
};

template <typename Socket, typename AsioBuffer>
inline Lazy<std::pair<std::error_code, size_t>> asyncRead(Socket& socket,
                                                          AsioBuffer& buffer) noexcept {
    co_return co_await ReadAwaiter(socket, buffer);
}

template <typename Socket, typename AsioBuffer>
class ReadUntilAwaiter {
public:
    ReadUntilAwaiter(Socket& socket, AsioBuffer& buffer, std::string_view delim)
        : _socket(socket), _buffer(buffer), _delim(delim) {}

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        asio::async_read_until(_socket, _buffer, _delim, [this, handle](auto ec, auto size) {
            _ec = ec;
            _size = size;
            handle.resume();
        });
    }
    auto await_resume() { return std::make_pair(_ec, _size); }

    auto coAwait(Executor* executor) noexcept { return std::move(*this); }

private:
    Socket& _socket;
    AsioBuffer& _buffer;
    std::string_view _delim;
    std::error_code _ec{};
    std::size_t _size{0};
};

template <typename Socket, typename AsioBuffer>
inline Lazy<std::pair<std::error_code, std::size_t>> asyncReadUntil(
    Socket& socket, AsioBuffer& buffer, std::string_view delim) noexcept {
    co_return co_await ReadUntilAwaiter(socket, buffer, delim);
}

template <typename Socket, typename AsioBuffer>
class ReadSomeAwaiter {
public:
    ReadSomeAwaiter(Socket& socket, AsioBuffer&& buffer) : _socket(socket), _buffer(buffer) {}

    bool await_ready() { return false; }
    auto await_suspend(std::coroutine_handle<> handle) {
        _socket.async_read_some(std::move(_buffer), [this, handle](auto ec, auto size) {
            _ec = ec;
            _size = size;
            handle.resume();
        });
    }
    auto await_resume() { return std::make_pair(_ec, _size); }

    auto coAwait(Executor* executor) noexcept { return std::move(*this); }

private:
    Socket& _socket;
    AsioBuffer _buffer;
    std::error_code _ec{};
    size_t _size{0};
};

template <typename Socket, typename AsioBuffer>
inline Lazy<std::pair<std::error_code, size_t>> asyncReadSome(Socket& socket,
                                                              AsioBuffer&& buffer) noexcept {
    co_return co_await ReadSomeAwaiter(socket, std::forward<AsioBuffer>(buffer));
}

template <typename Socket, typename AsioBuffer>
class WriteAwaiter {
public:
    WriteAwaiter(Socket& socket, AsioBuffer&& buffer)
        : _socket(socket), _buffer(std::move(buffer)) {}

    bool await_ready() { return false; }
    auto await_suspend(std::coroutine_handle<> handle) {
        asio::async_write(_socket, std::move(_buffer), [this, handle](auto ec, auto size) {
            _ec = ec;
            _size = size;
            handle.resume();
        });
    }
    auto await_resume() { return std::make_pair(_ec, _size); }

private:
    Socket& _socket;
    AsioBuffer _buffer;
    std::error_code _ec{};
    size_t _size{0};
};

template <typename Socket, typename AsioBuffer>
inline Lazy<std::pair<std::error_code, std::size_t>> asyncWrite(Socket& socket,
                                                                AsioBuffer&& buffer) noexcept {
    co_return co_await WriteAwaiter(socket, std::forward<AsioBuffer>(buffer));
}

class ConnectAwaiter {
public:
    ConnectAwaiter(asio::io_context& ioContext, tcp::socket& socket, const std::string& host,
                   const std::string& port)
        : _ioContext(ioContext), _socket(socket), _host(host), _port(port) {}

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        tcp::resolver resolver(_ioContext);
        auto endpoints = resolver.resolve(_host, _port);
        asio::async_connect(_socket, endpoints,
                            [this, handle](std::error_code ec, const tcp::endpoint&) {
                                _ec = ec;
                                handle.resume();
                            });
    }
    auto await_resume() noexcept { return _ec; }
    auto coAwait(Executor* executor) noexcept { return std::move(*this); }

private:
    asio::io_context& _ioContext;
    tcp::socket& _socket;
    std::string _host;
    std::string _port;
    std::error_code _ec{};
};

inline Lazy<std::error_code> asyncConnect(asio::io_context& ioCtx, tcp::socket& socket,
                                          const std::string& host,
                                          const std::string& port) noexcept {
    co_return co_await ConnectAwaiter(ioCtx, socket, host, port);
}

#undef tcp
#undef asio

#endif  // TINY_HTTP_SERVER_ASIO_COROUTINE_UTIL_H
