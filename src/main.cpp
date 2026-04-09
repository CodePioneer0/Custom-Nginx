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

using namespace std;

// --- PHASE 4: HEADER BUILDER (Separated from Body for sendfile) ---
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

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "Bind failed" << endl;
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        cerr << "Listen failed" << endl;
        return 1;
    }

    cout << "Phase 4 Server is listening on port " << port << "..." << endl;

    while (true) {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            cerr << "Accept failed" << endl;
            continue;
        }

        char buffer[1024] = {0};
        read(client_socket, buffer, 1024);
        
        string request(buffer);
        istringstream request_stream(request);

        string method, path, version;
        request_stream >> method >> path >> version;

        cout << "\n--- Received Request ---" << endl;
        cout << method << " " << path << " " << version << endl;

        // --- PHASE 4: STATIC FILE SERVER WITH SENDFILE ---
        
        // Defalut routing to index.html
        if (path == "/") {
            path = "/index.html";
        }
        
        // Map requested path to local `www` directory
        string local_path = "www" + path;
        
        // Attempt to open the requested file
        int file_fd = open(local_path.c_str(), O_RDONLY);
        
        if (file_fd < 0) {
            // File does not exist
            string body = "<html><body><h1>404 Not Found</h1></body></html>";
            string response = build_headers(404, "Not Found", "text/html", body.length()) + body;
            
            send(client_socket, response.c_str(), response.length(), 0);
            cout << "404 Not Found: " << local_path << endl;
        } else {
            // File exists! Get its size
            struct stat stat_buf;
            fstat(file_fd, &stat_buf);
            
            // Basic Content-Type mapping
            string content_type = "text/plain";
            if (path.find(".html") != string::npos) content_type = "text/html";
            else if (path.find(".css") != string::npos)  content_type = "text/css";
            else if (path.find(".png") != string::npos)  content_type = "image/png";
            
            // 1. Send only the headers specifying the file size
            string headers = build_headers(200, "OK", content_type, stat_buf.st_size);
            send(client_socket, headers.c_str(), headers.length(), 0);
            
            // 2. Use sendfile() for zero-copy high-performance transfer
            off_t offset = 0;
            sendfile(client_socket, file_fd, &offset, stat_buf.st_size);
            
            close(file_fd);
            cout << "200 OK: Served " << local_path << " using sendfile()" << endl;
        }

        close(client_socket);
    }

    close(server_fd);
    return 0;
}
