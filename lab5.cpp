#include <iostream>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <thread>
#include <vector>

#define PORT 8080
#define BUFFER_SIZE 1024

std::unordered_map<std::string, std::string> pages = {
    {"/", "index.html"}
};

std::string loadFile(const std::string& filePath) {
    FILE *file = fopen(filePath.c_str(), "r");
    if (!file) {
        return "";
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<char> buffer(fileSize);
    fread(buffer.data(), 1, fileSize, file);
    fclose(file);

    return std::string(buffer.begin(), buffer.end());
}

void sendResponse(int clientSocket, const std::string &response) {
    send(clientSocket, response.c_str(), response.size(), 0);
}

void handleRequest(int clientSocket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        std::cerr << "Read failed\n";
        close(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer);

    size_t methodEnd = request.find(' ');
    if (methodEnd == std::string::npos) {
        close(clientSocket);
        return;
    }
    std::string method = request.substr(0, methodEnd);

    size_t pathEnd = request.find(' ', methodEnd + 1);
    if (pathEnd == std::string::npos) {
        close(clientSocket);
        return;
    }
    std::string path = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);

    std::cout << "Received request: " << method << " " << path << std::endl;

    if (method == "GET") {
        std::string content;
        std::string statusLine;

        auto it = pages.find(path);
        if (it != pages.end()) {
            content = loadFile(it->second);
            if (content.empty()) {
                statusLine = "HTTP/1.1 500 Internal Server Error\r\n";
                content = "<html><body><h1>500 Internal Server Error</h1></body></html>";
                std::cerr << "Error: Unable to load file for path " << path << std::endl;
            } else {
                statusLine = "HTTP/1.1 200 OK\r\n";
            }
        } else {
            content = loadFile(path.substr(1));
            if (content.empty()) {
                statusLine = "HTTP/1.1 404 Not Found\r\n";
                content = "<html><body><h1>404 Not Found</h1></body></html>";
                std::cout << "Path not found: " << path << std::endl;
            } else {
                statusLine = "HTTP/1.1 200 OK\r\n";
            }
        }

        std::string httpResponse = statusLine +
                                   "Content-Length: " + std::to_string(content.size()) + "\r\n" +
                                   "Content-Type: text/html\r\n" +
                                   "\r\n" + content;

        sendResponse(clientSocket, httpResponse);
    } else {
        std::cerr << "Error: Unsupported method " << method << std::endl;
    }

    close(clientSocket);
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Bind failed\n";
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Listen failed\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);

        if (clientSocket == -1) {
            std::cerr << "Accept failed\n";
            continue;
        }

        std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

        std::thread(handleRequest, clientSocket).detach();
    }

    close(serverSocket);
    return 0;
}
