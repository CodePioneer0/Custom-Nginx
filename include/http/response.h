#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>

std::string build_headers(int status_code, const std::string& status_message, const std::string& content_type, size_t content_length);

#endif
