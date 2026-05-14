#include "../../include/handlers/static_handler.h"
#include "../../include/http/response.h"
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>

using namespace std;

void handle_static(int client_socket, string path) {
    if (path == "/" || path == "/static" || path == "/static/") {
        path = "/index.html";
    } else if (path.find("/static/") == 0) {
        path = path.substr(7);
    }

    string local_path = "www" + path;
    int file_fd = open(local_path.c_str(), O_RDONLY);
    
    if (file_fd < 0) {
        string body = "<html><body><h1>404 Not Found</h1></body></html>";
        string response = build_headers(404, "Not Found", "text/html", body.length()) + body;
        send(client_socket, response.c_str(), response.length(), 0);
        cout << "404 Not Found: " << local_path << endl;
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
        cout << "200 OK: Served Static File " << local_path << endl;
    }
}
