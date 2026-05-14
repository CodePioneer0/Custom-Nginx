#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <fcntl.h>
#include <sys/epoll.h>

#include "../include/handlers/static_handler.h"
#include "../include/handlers/proxy_handler.h"
#include "../include/thread_pool.h"

using namespace std;

#define MAX_EVENTS 1024
#define NUM_WORKER_THREADS 4

int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void route_request(int client_socket, const string& path) {
    if (path.find("/api") == 0) {
        handle_api(client_socket, path);
    } else {
        handle_static(client_socket, path);
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int port = 8080;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) return 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) return 1;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) return 1;
    if (listen(server_fd, SOMAXCONN) < 0) return 1;

    set_non_blocking(server_fd);
    int epoll_fd = epoll_create1(0);
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    // --- PHASE 6: INITIALIZE THREAD POOL ---
    ThreadPool pool(NUM_WORKER_THREADS); 
    
    cout << "🚀 Phase 8 SERVER LIVE: epoll async I/O + Thread Pool (" << NUM_WORKER_THREADS << " workers) + Load Balancer listening on port " << port << "..." << endl;

    while (true) {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < event_count; i++) {
            int active_fd = events[i].data.fd;

            if (active_fd == server_fd) {
                int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                if (client_socket < 0) continue;

                set_non_blocking(client_socket);
                
                // Use EPOLLONESHOT to ensure only ONE thread processes this socket at a time
                event.events = EPOLLIN | EPOLLONESHOT;
                event.data.fd = client_socket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
            } 
            else if (events[i].events & EPOLLIN) {
                int client_socket = active_fd;

                // --- PHASE 6 & 8 INTEGRATION ---
                // Instead of processing in the main event loop block, we push the work to the thread pool!
                
                pool.enqueue([client_socket]() {
                    char buffer[2048] = {0};
                    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
                    
                    if (bytes_read > 0) {
                        string request(buffer);
                        istringstream request_stream(request);
                        string method, path, version;
                        request_stream >> method >> path >> version;

                        if (!path.empty()) {
                            // Dispatch to Phase 7/8 Router
                            route_request(client_socket, path);
                        }
                    }

                    // Close the connection (automatically removes it from epoll kernel object)
                    close(client_socket);
                });
            }
        }
    }
    close(server_fd);
    return 0;
}
