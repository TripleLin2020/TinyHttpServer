#ifndef TINY_HTTP_SERVER_CONNECTION_H
#define TINY_HTTP_SERVER_CONNECTION_H

#include <boost/asio/ip/tcp.hpp>
#include <fstream>
#include <iostream>

#include "AsioCoroutineUtil.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Lazy.h"

class Connection {
    using Socket = boost::asio::ip::tcp::socket;

public:
    Connection(Socket socket, std::string&& docRoot)
        : _socket(std::move(socket)), _docRoot(std::move(docRoot)) {}

    ~Connection() {
        boost::system::error_code ec;
        _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        _socket.close(ec);
    }

    Lazy<void> start() {
        while (true) {
            auto [err, bytesTransferred] =
                co_await asyncReadSome(_socket, boost::asio::buffer(_readBuffer));
            if (err) {
                std::cerr << "Error: " << err.message() << " , size = " << bytesTransferred
                          << std::endl;
                break;
            }

            auto [res, dummy] =
                _parser.parse(_request, _readBuffer, _readBuffer + bytesTransferred);
            if (res == RequestParser::succeed) {
                _response = handleRequest(_request);
                co_await asyncWrite(_socket, _response.toBuffers());
                if (!isKeepAlive()) break;
                _request = {};
                _response = {};
                _parser.reset();
            } else if (res == RequestParser::failed) {
                _response = Response(StatusType::bad_request);
                co_await asyncWrite(_socket, _response.toBuffers());
                break;
            }
        }
    }

private:
    Response handleRequest(const Request& request) {
        std::string reqPath = decodeUrl(request.uri);

        // Request path must be absolute.
        if (reqPath.empty() || reqPath[0] != '/' || reqPath.find("..") != std::string::npos) {
            return {StatusType::bad_request};
        }

        if (reqPath.back() == '/') return {StatusType::ok};

        // Get the file extension.
        std::size_t lastSlashPos = reqPath.find_last_of('/');
        std::size_t lastDotPos = reqPath.find_last_of('.');
        std::string extension;
        if (lastDotPos != std::string::npos && lastDotPos > lastSlashPos) {
            extension = reqPath.substr(lastDotPos + 1);
        }

        // Open the file to send back.
        std::string fullPath = _docRoot + reqPath;
        std::ifstream is(fullPath.c_str(), std::ios::in | std::ios::binary);
        if (!is) return {StatusType::not_found};

        // Fill out the response to be sent to the client.
        Response response(StatusType::ok, MimeType::extensionToType(extension));
        char buf[512];
        while (is.read(buf, sizeof(buf)).gcount() > 0) {
            response.appendToContent(buf, is.gcount());
        }
    }

    static std::string decodeUrl(const std::string& url) {
        std::string out;
        out.reserve(url.size());
        for (std::size_t i = 0; i < url.size(); ++i) {
            if (url[i] == '%') {
                if (i + 3 <= url.size()) {
                    int value = 0;
                    std::istringstream is(url.substr(i + 1, 2));
                    if (is >> std::hex >> value) {
                        out += static_cast<char>(value);
                        i += 2;
                    }
                } else {
                    return {};
                }
            } else if (url[i] == '+') {
                out += ' ';
            } else {
                out += url[i];
            }
        }
        return out;
    }

    bool isKeepAlive() {
        return std::ranges::none_of(_request.headers, [](const auto& h) {
            return h.name == "Connection" && h.value == "close";
        });
    }

private:
    Socket _socket;
    char _readBuffer[1024]{};
    RequestParser _parser;
    Request _request;
    Response _response;
    std::string _docRoot;
};

#endif  // TINY_HTTP_SERVER_CONNECTION_H
