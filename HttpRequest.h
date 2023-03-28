#ifndef TINY_HTTP_SERVER_HTTP_REQUEST_H
#define TINY_HTTP_SERVER_HTTP_REQUEST_H

#include <string>
#include <tuple>
#include <vector>

struct Header {
    std::string name;
    std::string value;
};

struct Request {
    std::string method;
    std::string uri;
    int httpVersionMajor;
    int httpVersionMinor;
    std::vector<Header> headers;
};

class RequestParser {
public:
    RequestParser() : _state(method_start) {}

    /// Reset to initial parser state
    void reset() { _state = method_start; }

    /// Result of parse
    enum ResultType { succeed, failed, indeterminate };

    /// Parse some data.
    /// 'result_type' is 'succeed' when a complete request has been parsed.
    /// It's 'failed' when the data is invalid.
    /// It's 'indeterminate' when more data is required.
    /// 'InputIterator' indicates how much of the input has been consumed.
    template <typename InputIterator>
    std::tuple<ResultType, InputIterator> parse(Request& req, InputIterator begin,
                                                InputIterator end) {
        while (begin != end) {
            ResultType res = consume(req, *begin++);
            if (res == succeed || res == failed) return std::make_tuple(res, begin);
        }
        return std::make_tuple(indeterminate, begin);
    }

private:
    // Handle 'input'
    ResultType consume(Request& req, char input) {
        switch (_state) {
            case method_start:
                if (!isChar(input) || isCtl(input) || isTspecial(input)) {
                    return failed;
                } else {
                    _state = method;
                    req.method.push_back(input);
                    return indeterminate;
                }
            case method:
                if (input == ' ') {
                    _state = uri;
                    return indeterminate;
                } else if (!isChar(input) || isCtl(input) || isTspecial(input)) {
                    return failed;
                } else {
                    req.method.push_back(input);
                    return indeterminate;
                }
            case uri:
                if (input == ' ') {
                    _state = http_version_h;
                    return indeterminate;
                } else if (isCtl(input)) {
                    return failed;
                } else {
                    req.uri.push_back(input);
                    return indeterminate;
                }
            case http_version_h:
                if (input == 'H') {
                    _state = http_version_t_1;
                    return indeterminate;
                } else {
                    return failed;
                }
            case http_version_t_1:
                if (input == 'T') {
                    _state = http_version_t_2;
                    return indeterminate;
                } else {
                    return failed;
                }
            case http_version_t_2:
                if (input == 'T') {
                    _state = http_version_p;
                    return indeterminate;
                } else {
                    return failed;
                }
            case http_version_p:
                if (input == 'P') {
                    _state = http_version_slash;
                    return indeterminate;
                } else {
                    return failed;
                }
            case http_version_slash:
                if (input == '/') {
                    req.httpVersionMajor = 0;
                    req.httpVersionMinor = 0;
                    _state = http_version_major_start;
                    return indeterminate;
                } else {
                    return failed;
                }
            case http_version_major_start:
                if (isDigit(input)) {
                    req.httpVersionMajor = req.httpVersionMinor * 10 + input - '0';
                    _state = http_version_major;
                    return indeterminate;
                } else {
                    return failed;
                }
            case http_version_major:
                if (input == '.') {
                    _state = http_version_minor_start;
                    return indeterminate;
                } else if (isDigit(input)) {
                    req.httpVersionMajor = req.httpVersionMajor * 10 + input - '0';
                    return indeterminate;
                } else {
                    return failed;
                }
            case http_version_minor_start:
                if (isDigit(input)) {
                    req.httpVersionMinor = req.httpVersionMinor * 10 + input - '0';
                    _state = http_version_minor;
                    return indeterminate;
                } else {
                    return failed;
                }
            case http_version_minor:
                if (input == '\r') {
                    _state = expecting_newline_1;
                    return indeterminate;
                } else if (isDigit(input)) {
                    req.httpVersionMinor = req.httpVersionMinor * 10 + input - '0';
                    return indeterminate;
                } else {
                    return failed;
                }
            case expecting_newline_1:
                if (input == '\n') {
                    _state = header_line_start;
                    return indeterminate;
                } else {
                    return failed;
                }
            case header_line_start:
                if (input == '\r') {
                    _state = expecting_newline_3;
                    return indeterminate;
                } else if (!req.headers.empty() && (input == ' ' || input == '\t')) {
                    _state = header_lws;
                    return indeterminate;
                } else if (!isChar(input) || isCtl(input) || isTspecial(input)) {
                    return failed;
                } else {
                    req.headers.push_back(Header());
                    req.headers.back().name.push_back(input);
                    _state = header_name;
                    return indeterminate;
                }
            case header_lws:
                if (input == '\r') {
                    _state = expecting_newline_2;
                    return indeterminate;
                } else if (input == ' ' || input == '\t') {
                    return indeterminate;
                } else if (isCtl(input)) {
                    return failed;
                } else {
                    _state = header_value;
                    req.headers.back().value.push_back(input);
                    return indeterminate;
                }
            case header_name:
                if (input == ':') {
                    _state = space_before_header_value;
                    return indeterminate;
                } else if (!isChar(input) || isCtl(input) || isTspecial(input)) {
                    return failed;
                } else {
                    req.headers.back().name.push_back(input);
                    return indeterminate;
                }
            case space_before_header_value:
                if (input == ' ') {
                    _state = header_value;
                    return indeterminate;
                } else {
                    return failed;
                }
            case header_value:
                if (input == '\r') {
                    _state = expecting_newline_2;
                    return indeterminate;
                } else if (isCtl(input)) {
                    return failed;
                } else {
                    req.headers.back().value.push_back(input);
                    return indeterminate;
                }
            case expecting_newline_2:
                if (input == '\n') {
                    _state = header_line_start;
                    return indeterminate;
                } else {
                    return failed;
                }
            case expecting_newline_3:
                return (input == '\n') ? succeed : failed;
            default:
                return failed;
        }
    }

    /// Check if a byte is an HTTP character
    static bool isChar(int c) { return c >= 0 && c <= 127; }

    /// Check if a byte is an HTTP control character
    static bool isCtl(int c) { return (c >= 0 && c <= 31) || (c == 127); }

    /// Check if a byte is defined as an HTTP tspecial character
    static bool isTspecial(int c) {
        switch (c) {
            case '(':
            case ')':
            case '<':
            case '>':
            case '@':
            case ',':
            case ';':
            case ':':
            case '\\':
            case '"':
            case '/':
            case '[':
            case ']':
            case '?':
            case '=':
            case '{':
            case '}':
            case ' ':
            case '\t':
                return true;
            default:
                return false;
        }
    }

    /// Check if a byte is a digit
    static bool isDigit(int c) { return c >= '0' && c <= '9'; }

private:
    enum state {
        method_start,
        method,
        uri,
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        expecting_newline_1,
        header_line_start,
        header_lws,
        header_name,
        space_before_header_value,
        header_value,
        expecting_newline_2,
        expecting_newline_3
    } _state;
};

#endif  // TINY_HTTP_SERVER_HTTP_REQUEST_H
