#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>

using namespace std;

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int port = 8080;

    // 1. Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    // Attach socket to the port (prevents "Address already in use" errors)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        cerr << "Setsockopt failed" << endl;
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // 2. Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "Bind failed" << endl;
        return 1;
    }

    // 3. Listen for incoming connections
    if (listen(server_fd, 10) < 0) {
        cerr << "Listen failed" << endl;
        return 1;
    }

    cout << "Basic TCP Server is listening on port " << port << "..." << endl;

    // 4. Accept multiple clients (blocking loop)
    while (true) {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            cerr << "Accept failed" << endl;
            continue;
        }

        // Read the incoming request
        char buffer[1024] = {0};
        read(client_socket, buffer, 1024);
        cout << "\n--- Received Connection ---\n";

        // --- PHASE 2: HTTP PARSING ---
        string request(buffer);
        istringstream request_stream(request);

        string method, path, version;
        // The first line of an HTTP request is: METHOD PATH HTTP_VERSION
        request_stream >> method >> path >> version;

        cout << "Parsed Request Line:" << endl;
        cout << "  Method:  " << method << endl;
        cout << "  Path:    " << path << endl;
        cout << "  Version: " << version << endl;

        cout << "Parsed Headers:" << endl;
        string header_line;
        // Skip to the end of the first line
        getline(request_stream, header_line);

        // Read headers until we hit an empty line (which separates headers from body)
        while (getline(request_stream, header_line) && header_line != "\r") {
            if (header_line.empty()) break;
            cout << "  " << header_line << endl;
        }
        cout << "---------------------------\n";

        // 5. Send response
        string body = "Hello from server";
        string response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: " + to_string(body.length()) + "\r\n"
                          "\r\n" + body;

        send(client_socket, response.c_str(), response.length(), 0);
        cout << "Response sent to client." << endl;

        // Close connection
        close(client_socket);
    }

    close(server_fd);
    return 0;
}
