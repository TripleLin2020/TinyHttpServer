#ifndef TINY_HTTP_SERVER_HTTP_RESPONSE_H
#define TINY_HTTP_SERVER_HTTP_RESPONSE_H

#include <boost/asio.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class StatusType {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
};

#define asio boost::asio

namespace StatusLine {
constexpr std::string_view ok = "HTTP/1.1 200 OK\r\n";
constexpr std::string_view created = "HTTP/1.1 201 Created\r\n";
constexpr std::string_view accepted = "HTTP/1.1 202 Accepted\r\n";
constexpr std::string_view no_content = "HTTP/1.1 204 No Content\r\n";
constexpr std::string_view multiple_choices = "HTTP/1.1 300 Multiple Choices\r\n";
constexpr std::string_view moved_permanently = "HTTP/1.1 301 Moved Permanently\r\n";
constexpr std::string_view moved_temporarily = "HTTP/1.1 302 Moved Temporarily\r\n";
constexpr std::string_view not_modified = "HTTP/1.1 304 Not Modified\r\n";
constexpr std::string_view bad_request = "HTTP/1.1 400 Bad Request\r\n";
constexpr std::string_view unauthorized = "HTTP/1.1 401 Unauthorized\r\n";
constexpr std::string_view forbidden = "HTTP/1.1 403 Forbidden\r\n";
constexpr std::string_view not_found = "HTTP/1.1 404 Not Found\r\n";
constexpr std::string_view internal_server_error = "HTTP/1.1 500 Internal Server Error\r\n";
constexpr std::string_view not_implemented = "HTTP/1.1 501 Not Implemented\r\n";
constexpr std::string_view bad_gateway = "HTTP/1.1 502 Bad Gateway\r\n";
constexpr std::string_view service_unavailable = "HTTP/1.1 503 Service Unavailable\r\n";

asio::const_buffer statusToBuffer(StatusType status) {
    switch (status) {
#define CASE(x)         \
    case StatusType::x: \
        return asio::buffer(x)
        CASE(ok);
        CASE(accepted);
        CASE(no_content);
        CASE(multiple_choices);
        CASE(moved_permanently);
        CASE(moved_temporarily);
        CASE(not_modified);
        CASE(bad_request);
        CASE(unauthorized);
        CASE(forbidden);
        CASE(not_found);
        CASE(internal_server_error);
        CASE(not_implemented);
        CASE(bad_gateway);
        CASE(service_unavailable);
#undef CASE
        default:
            return asio::buffer(internal_server_error);
    }
}
}  // namespace StatusLine

namespace MimeType {
const std::unordered_map<std::string_view, std::string_view> map = {{"gif", "image/gif"},
                                                                    {"htm", "text/html"},
                                                                    {"html", "text/html"},
                                                                    {"jpg", "image/jpeg"},
                                                                    {"png", "image/png"}};

std::string_view extensionToType(std::string_view extension) {
    if (auto it = map.find(extension); it != map.end()) {
        return it->second;
    }
    return "text/plain";
}

}  // namespace MimeType

namespace MiscString {
constexpr std::string_view nameValueSeparator = ": ";
constexpr std::string_view crlf = "\r\n";
}  // namespace MiscString

namespace response_content {
constexpr std::string_view response_ok =
    "<html>"
    "<head><title>Hello</title></head>"
    "<body><h1>Hello async_simple</h1></body>"
    "</html>";
constexpr std::string_view response_created =
    "<html>"
    "<head><title>Created</title></head>"
    "<body><h1>201 Created</h1></body>"
    "</html>";
constexpr std::string_view response_accepted =
    "<html>"
    "<head><title>Accepted</title></head>"
    "<body><h1>202 Accepted</h1></body>"
    "</html>";
constexpr std::string_view response_no_content =
    "<html>"
    "<head><title>No Content</title></head>"
    "<body><h1>204 Content</h1></body>"
    "</html>";
constexpr std::string_view response_multiple_choices =
    "<html>"
    "<head><title>Multiple Choices</title></head>"
    "<body><h1>300 Multiple Choices</h1></body>"
    "</html>";
constexpr std::string_view response_moved_permanently =
    "<html>"
    "<head><title>Moved Permanently</title></head>"
    "<body><h1>301 Moved Permanently</h1></body>"
    "</html>";
constexpr std::string_view response_moved_temporarily =
    "<html>"
    "<head><title>Moved Temporarily</title></head>"
    "<body><h1>302 Moved Temporarily</h1></body>"
    "</html>";
constexpr std::string_view response_not_modified =
    "<html>"
    "<head><title>Not Modified</title></head>"
    "<body><h1>304 Not Modified</h1></body>"
    "</html>";
constexpr std::string_view response_bad_request =
    "<html>"
    "<head><title>Bad Request</title></head>"
    "<body><h1>400 Bad Request</h1></body>"
    "</html>";
constexpr std::string_view response_unauthorized =
    "<html>"
    "<head><title>Unauthorized</title></head>"
    "<body><h1>401 Unauthorized</h1></body>"
    "</html>";
constexpr std::string_view response_forbidden =
    "<html>"
    "<head><title>Forbidden</title></head>"
    "<body><h1>403 Forbidden</h1></body>"
    "</html>";
constexpr std::string_view response_not_found =
    "<html>"
    "<head><title>Not Found</title></head>"
    "<body><h1>404 Not Found</h1></body>"
    "</html>";
constexpr std::string_view response_internal_server_error =
    "<html>"
    "<head><title>Internal Server Error</title></head>"
    "<body><h1>500 Internal Server Error</h1></body>"
    "</html>";
constexpr std::string_view response_not_implemented =
    "<html>"
    "<head><title>Not Implemented</title></head>"
    "<body><h1>501 Not Implemented</h1></body>"
    "</html>";
constexpr std::string_view response_bad_gateway =
    "<html>"
    "<head><title>Bad Gateway</title></head>"
    "<body><h1>502 Bad Gateway</h1></body>"
    "</html>";
constexpr std::string_view response_service_unavailable =
    "<html>"
    "<head><title>Service Unavailable</title></head>"
    "<body><h1>503 Service Unavailable</h1></body>"
    "</html>";

std::string_view to_string(StatusType status) {
    switch (status) {
        case StatusType::ok:
            return response_ok;
        case StatusType::created:
            return response_created;
        case StatusType::accepted:
            return response_accepted;
        case StatusType::no_content:
            return response_no_content;
        case StatusType::multiple_choices:
            return response_multiple_choices;
        case StatusType::moved_permanently:
            return response_moved_permanently;
        case StatusType::moved_temporarily:
            return response_moved_temporarily;
        case StatusType::not_modified:
            return response_not_modified;
        case StatusType::bad_request:
            return response_bad_request;
        case StatusType::unauthorized:
            return response_unauthorized;
        case StatusType::forbidden:
            return response_forbidden;
        case StatusType::not_found:
            return response_not_found;
        case StatusType::internal_server_error:
            return response_internal_server_error;
        case StatusType::not_implemented:
            return response_not_implemented;
        case StatusType::bad_gateway:
            return response_bad_gateway;
        case StatusType::service_unavailable:
            return response_service_unavailable;
        default:
            return response_internal_server_error;
    }
}
}  // namespace response_content

class Response {
public:
    Response() = default;

    Response(StatusType status, std::string_view contentType = "text/html")
        : _status(status), _content(response_content::to_string(status)) {
        _headers.resize(2);
        _headers[0].name = "Content-Length";
        _headers[0].value = std::to_string(_content.size());
        _headers[1].name = "Content-Type";
        _headers[1].value = contentType;
    }

    std::vector<asio::const_buffer> toBuffers() {
        std::vector<asio::const_buffer> buffers;
        buffers.push_back(StatusLine::statusToBuffer(_status));
        for (const auto& h : _headers) {
            buffers.push_back(asio::buffer(h.name));
            buffers.push_back(asio::buffer(MiscString::nameValueSeparator));
            buffers.push_back(asio::buffer(h.value));
            buffers.push_back(asio::buffer(MiscString::crlf));
        }
        buffers.push_back(asio::buffer(MiscString::crlf));
        buffers.push_back(asio::buffer(_content));
        return buffers;
    }

    void appendToContent(const char* buf, std::size_t len) {
        _content.append(buf, len);
        _headers[0].value = std::to_string(_content.size());
    }


private:
    StatusType _status;
    std::vector<Header> _headers;
    std::string _content;
};

#undef asio

#endif  // TINY_HTTP_SERVER_HTTP_RESPONSE_H