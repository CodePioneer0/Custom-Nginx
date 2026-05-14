#include "../../include/handlers/proxy_handler.h"
#include "../../include/http/response.h"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <atomic>

using namespace std;

struct Backend {
    string ip;
    int port;
};

vector<Backend> backends = {
    {"127.0.0.1", 9001},
    {"127.0.0.1", 9002}
};

atomic<int> current_backend_idx{0};

void handle_api(int client_socket, const string& path) {
    int idx = current_backend_idx.fetch_add(1) % backends.size();
    Backend backend = backends[idx];
    
    cout << "[Proxy] Forwarding request for " << path << " to " << backend.ip << ":" << backend.port << endl;

    int proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_fd < 0) return;

    struct sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(backend.port);
    inet_pton(AF_INET, backend.ip.c_str(), &backend_addr.sin_addr);

    if (connect(proxy_fd, (struct sockaddr*)&backend_addr, sizeof(backend_addr)) < 0) {
        cout << "[Proxy] Backend " << backend.port << " is down!" << endl;
        string body = "502 Bad Gateway - Backend Offline";
        string resp = build_headers(502, "Bad Gateway", "text/plain", body.length()) + body;
        send(client_socket, resp.c_str(), resp.length(), 0);
        close(proxy_fd);
        return;
    }

    string proxy_request = "GET " + path + " HTTP/1.1\r\n";
    proxy_request += "Host: " + backend.ip + "\r\n";
    proxy_request += "Connection: close\r\n\r\n";
    send(proxy_fd, proxy_request.c_str(), proxy_request.length(), 0);

    char buffer[4096];
    int bytes_read;
    while ((bytes_read = read(proxy_fd, buffer, sizeof(buffer))) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    close(proxy_fd);
    cout << "[Proxy] Successfully served from backend " << backend.port << endl;
}
