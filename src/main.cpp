#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>

using namespace std;

// --- PHASE 3: HTTP RESPONSE BUILDER ---
string build_response(int status_code, const string& status_message, const string& content_type, const string& body) {
    string response = "HTTP/1.1 " + to_string(status_code) + " " + status_message + "\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    response += "Content-Length: " + to_string(body.length()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n"; // Blank line between headers and body
    response += body;
    return response;
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

    cout << "Basic TCP Server is listening on port " << port << "..." << endl;

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

        // --- PHASE 3: DYNAMIC RESPONSE based on path ---
        string response;
        if (path == "/") {
            string body = "<html><body><h1>Welcome to Mini Nginx!</h1><p>This is a proper 200 OK response.</p></body></html>";
            response = build_response(200, "OK", "text/html", body);
        } else {
            string body = "<html><body><h1>404 Not Found</h1><p>The requested page does not exist.</p></body></html>";
            response = build_response(404, "Not Found", "text/html", body);
        }

        send(client_socket, response.c_str(), response.length(), 0);
        cout << "Response sent for path: " << path << endl;

        close(client_socket);
    }

    close(server_fd);
    return 0;
}
