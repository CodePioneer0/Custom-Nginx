#include "../../include/http/response.h"

std::string build_headers(int status_code, const std::string& status_message, const std::string& content_type, size_t content_length) {
    std::string headers = "HTTP/1.1 " + std::to_string(status_code) + " " + status_message + "\r\n";
    headers += "Content-Type: " + content_type + "\r\n";
    headers += "Content-Length: " + std::to_string(content_length) + "\r\n";
    headers += "Connection: close\r\n";
    headers += "\r\n";
    return headers;
}
