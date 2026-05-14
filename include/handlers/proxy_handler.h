#ifndef PROXY_HANDLER_H
#define PROXY_HANDLER_H

#include <string>

void handle_api(int client_socket, const std::string& path);

#endif
