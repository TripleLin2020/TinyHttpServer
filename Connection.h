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
                std::cout << "error message: " << err.message() << ", size = " << bytesTransferred
                          << std::endl;
                break;
            }

            RequestParser::ResultType res;
            std::tie(res, std::ignore) =
                _parser.parse(_request, _readBuffer, _readBuffer + bytesTransferred);
            if (res == RequestParser::good) {
                _response = handleRequest(_request);
                co_await asyncWrite(_socket, _response.toBuffers());
                if (!isKeepAlive()) break;
                _response = {};
                _response = {};
                _parser.reset();
            } else if (res == RequestParser::bad) {
                _response = Response(StatusType::bad_request);
                co_await asyncWrite(_socket, _response.toBuffers());
                break;
            }
        }
    }

private:
    Response handleRequest(const Request& request) {
        std::string reqPath;
        if (!decodeUrl(request.uri, reqPath)) {
            return {StatusType::bad_request};
        }

        // Request path must be absolute.
        if (reqPath.empty() || reqPath[0] != '/' || reqPath.find("..") != std::string::npos) {
            return {StatusType::bad_request};
        }

        if (reqPath.back() == '/') {
            return {StatusType::ok};
        }

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
        if (!is) {
            return {StatusType::not_found};
        }

        // Fill out the response to be sent to the client.
        Response response(StatusType::ok, MimeType::extensionToType(extension));
        char buf[512];
        while (is.read(buf, sizeof(buf)).gcount() > 0) {
            response.appendToContent(buf, is.gcount());
        }
    }

    bool decodeUrl(const std::string& in, std::string& out) {
        out.clear();
        out.reserve(in.size());
        for (std::size_t i = 0; i < in.size(); ++i) {
            if (in[i] == '%') {
                if (i + 3 <= in.size()) {
                    int value = 0;
                    std::istringstream is(in.substr(i + 1, 2));
                    if (is >> std::hex >> value) {
                        out += static_cast<char>(value);
                        i += 2;
                    }
                } else {
                    return false;
                }
            } else if (in[i] == '+') {
                out += ' ';
            } else {
                out += in[i];
            }
        }
        return true;
    }

    bool isKeepAlive() {
        for (auto &[k, v] : _request.headers) {
            if (k == "Connection" && v == "close") return false;
        }
        return true;
    }

private:
    Socket _socket;
    char _readBuffer[1024];
    RequestParser _parser;
    Request _request;
    Response _response;
    std::string _docRoot;
};

#endif  // TINY_HTTP_SERVER_CONNECTION_H
