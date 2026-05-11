#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>

using namespace std;

#define MAX_EVENTS 1024

// --- PHASE 5: Utility to make a socket non-blocking ---
int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Header Builder (From Phase 4)
string build_headers(int status_code, const string& status_message, const string& content_type, size_t content_length) {
    string headers = "HTTP/1.1 " + to_string(status_code) + " " + status_message + "\r\n";
    headers += "Content-Type: " + content_type + "\r\n";
    headers += "Content-Length: " + to_string(content_length) + "\r\n";
    headers += "Connection: close\r\n";
    headers += "\r\n"; // Crucial blank line
    return headers;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int port = 8080;

    // 1. Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        cerr << "Setsockopt failed" << endl;
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // 2. Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "Bind failed" << endl;
        return 1;
    }

    // 3. Listen
    if (listen(server_fd, SOMAXCONN) < 0) {
        cerr << "Listen failed" << endl;
        return 1;
    }

    // --- PHASE 5: EPOLL SETUP ---
    // Make server socket non-blocking
    set_non_blocking(server_fd);

    // Create epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        cerr << "Failed to create epoll instance" << endl;
        return 1;
    }

    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN; // Monitor for incoming data (new connections)
    event.data.fd = server_fd;

    // Register server_fd in epoll
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        cerr << "Failed to add server_fd to epoll" << endl;
        return 1;
    }

    cout << "Phase 5 (epoll Async) Server is listening on port " << port << "..." << endl;

    // 4. epoll Event Loop
    while (true) {
        // Wait for events to happen block until at least 1 event occurs
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < event_count; i++) {
            int active_fd = events[i].data.fd;

            if (active_fd == server_fd) {
                // New incoming connection!
                int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                if (client_socket < 0) continue;

                // Make client non-blocking
                set_non_blocking(client_socket);

                // Add new client to epoll monitoring
                event.events = EPOLLIN; // Monitor for incoming requests
                event.data.fd = client_socket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
                
                cout << "[+] New connection accepted (FD: " << client_socket << ")" << endl;
            } 
            else if (events[i].events & EPOLLIN) {
                // Existing client has sent a request!
                int client_socket = active_fd;
                char buffer[2048] = {0};
                
                int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
                if (bytes_read <= 0) {
                    // Client disconnected or error occurred
                    cout << "[-] Client disconnected (FD: " << client_socket << ")" << endl;
                    close(client_socket);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
                    continue;
                }

                string request(buffer);
                istringstream request_stream(request);

                string method, path, version;
                request_stream >> method >> path >> version;

                if (path == "/") path = "/index.html";
                string local_path = "www" + path;
                
                int file_fd = open(local_path.c_str(), O_RDONLY);
                
                if (file_fd < 0) {
                    string body = "<html><body><h1>404 Not Found</h1></body></html>";
                    string response = build_headers(404, "Not Found", "text/html", body.length()) + body;
                    send(client_socket, response.c_str(), response.length(), 0);
                } else {
                    struct stat stat_buf;
                    fstat(file_fd, &stat_buf);
                    
                    string content_type = "text/plain";
                    if (path.find(".html") != string::npos) content_type = "text/html";
                    
                    string headers = build_headers(200, "OK", content_type, stat_buf.st_size);
                    send(client_socket, headers.c_str(), headers.length(), 0);
                    
                    off_t offset = 0;
                    sendfile(client_socket, file_fd, &offset, stat_buf.st_size);
                    close(file_fd);
                }

                // Since we don't have Keep-Alive yet, close connection after response
                close(client_socket);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
                cout << "[*] Served request and closed connection (FD: " << client_socket << ")" << endl;
            }
        }
    }

    close(server_fd);
    return 0;
}
