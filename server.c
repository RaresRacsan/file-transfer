#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/stat.h>
#include <ctype.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384 // 16 KB chunk size

// Function to calculate the checksum (sum of all bytes)
unsigned long calculate_checksum(FILE *file) {
    unsigned long checksum = 0;
    unsigned char buffer[CHUNK_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            checksum += buffer[i];
        }
    }

    return checksum;
}

// Function to sanitize filenames (remove/ replace with '_')
void sanitize_filename(char * fileName) {
    // List of invalid characters for filenames in Windows
    const char *invalidChars = "\\/*?:\"<>|";

    for(int i = 0; fileName[i]; i++) {
        if(strchr(invalidChars, fileName[i])) {
                fileName[i] = '_';
            }
    }
}

// Function to get the custom file type based on extension
const char* get_file_type(const char *fileName) {
    const char *ext = strrchr(fileName, '.');
    if(ext == NULL) {
        return "unknown/unknown";   // Default type for unknown files
    }

    ext++;

    // Check for text-based file extensions
    if (strcmp(ext, "txt") == 0) return "text/plain";
    if (strcmp(ext, "c") == 0) return "text/c";
    if (strcmp(ext, "cpp") == 0) return "text/cpp";
    if (strcmp(ext, "py") == 0) return "text/py";
    if (strcmp(ext, "js") == 0) return "text/javascript";
    if (strcmp(ext, "html") == 0) return "text/html";
    if (strcmp(ext, "css") == 0) return "text/css";
    if (strcmp(ext, "json") == 0) return "text/json";

    // For binary types or other files, return appropriate MIME type
    if (strcmp(ext, "exe") == 0) return "application/exe";
    if (strcmp(ext, "pdf") == 0) return "application/pdf";
    
    // For any other file type, return generic binary MIME type
    return "application/octet-stream";
}


int main() {
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0) {
        printf("Winsock initialization failed. Error code: %d\n", wsaInit);
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed. Error code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 3) == SOCKET_ERROR) {
        printf("Listen failed. Error code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("Listening on port %d...\n", PORT);

    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    while ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize)) != INVALID_SOCKET) {
        printf("Client connected.\n");

        char fileName[256];
        int bytesReceived = recv(clientSocket, fileName, sizeof(fileName), 0);
        if (bytesReceived <= 0) {
            printf("Error receiving filename.\n");
            closesocket(clientSocket);
            continue;
        }
        fileName[bytesReceived] = '\0';

        // Sanitize filename to avoid illegal characters
        sanitize_filename(fileName);

        FILE *file = fopen(fileName, "rb");
        if (file == NULL) {
            printf("File not found: %s\n", fileName);
            closesocket(clientSocket);
            continue;
        }

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        unsigned long checksum = calculate_checksum(file);
        fclose(file);

        printf("Server: File '%s' checksum: %lu\n", fileName, checksum);
        printf("Server: File size: %ld bytes\n", fileSize);

        // Get the mime type
        const char* fileType = get_file_type(fileName);
        printf("File type: %s.\n", fileType);


        // Send file metadata (name, size, checksum, type)
        send(clientSocket, fileName, strlen(fileName) + 1, 0);
        send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
        send(clientSocket, (char*)&checksum, sizeof(checksum), 0);
        send(clientSocket, fileType, strlen(fileType) + 1, 0);

        file = fopen(fileName, "rb");

        // Send file content in chunks  
        char buffer[CHUNK_SIZE];
        int bytesRead;

        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            send(clientSocket, buffer, bytesRead, 0);
        }

        printf("File sent successfully.\n");
        fclose(file);
        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}