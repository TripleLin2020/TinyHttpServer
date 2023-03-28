#include <iostream>
#include <string>
#include <thread>

#include "AsioCoroutineUtil.h"
#include "Lazy.h"
#include "SyncAwait.h"

using namespace boost;
using asio::ip::tcp;

Lazy<void> start(asio::io_context& ioCtx, std::string host, std::string port) {
    tcp::socket socket(ioCtx);
    if (auto ec = co_await asyncConnect(ioCtx, socket, host, port); ec) {
        std::cerr << "Connect error: " << ec.message() << std::endl;
        throw std::system_error(ec);
    }
    std::cout << "Connect to " << host << ":" << port << " successfully.\n";

    std::stringstream requestStream;
    requestStream << "GET "
                  << "/"
                  << " HTTP/1.1\r\n";
    requestStream << "Host: "
                  << "127.0.0.1"
                  << "\r\n";
    requestStream << "Accept: */*\r\n";
    requestStream << "Connection: close\r\n\r\n";

    co_await asyncWrite(socket, asio::buffer(requestStream.str(), requestStream.str().size()));

    asio::streambuf responseBuf;
    co_await asyncReadUntil(socket, responseBuf, "\r\n");

    std::istream responseStream(&responseBuf);
    std::string httpVersion;
    responseStream >> httpVersion;
    unsigned int statusCode;
    responseStream >> statusCode;
    std::string statusMessage;
    std::getline(responseStream, statusMessage);
    if (!responseStream || httpVersion.substr(0, 5) != "HTTP/") {
        std::cout << "Invalid response" << std::endl;
        co_return;
    }
    if (statusCode != 200) {
        std::cout << "Response returned with status code " << statusCode << std::endl;
        co_return;
    }

    co_await asyncReadUntil(socket, responseBuf, "\r\n\r\n");

    std::string header;
    while (std::getline(responseStream, header) && header != "\r") std::cout << header << "\n";
    std::cout << "\n";

    if (responseBuf.size() > 0) std::cout << &responseBuf << '\n';

    while (true) {
        auto [err, bytesTransferred] = co_await asyncRead(socket, responseBuf);
        if (err != system::error_code(asio::error::eof)) throw std::system_error(err);
        if (bytesTransferred <= 0) break;
        std::cout << &responseBuf << '\n';
    }

    system::error_code ignoreEc;
    socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignoreEc);
    socket.close(ignoreEc);
}

int main() {
    try {
        asio::io_context ioCtx;
        std::thread t([&ioCtx] {
            asio::io_context::work work(ioCtx);
            ioCtx.run();
        });
        syncAwait(start(ioCtx, "127.0.0.1", "2333"));
        ioCtx.stop();
        t.join();
        std::cout << "Finished ok, client quit." << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
