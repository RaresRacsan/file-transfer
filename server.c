#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

// Function to calculate the checksum (sum of all bytes)
unsigned long calculate_checksum(FILE *file) {
    unsigned long checksum = 0;
    unsigned char buffer[1024];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            checksum += buffer[i];
        }
    }

    return checksum;
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0) {
        printf("Winsock initialization failed. Error code: %d\n", wsaInit);
        return 1;
    }

    printf("Winsock initialized!\n");

    // Create server socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Bind server socket
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

    // Accept client connections
    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    while ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize)) != INVALID_SOCKET) {
        printf("Client connected.\n");

        // Receive filename from client
        char fileName[256];
        int bytesReceived = recv(clientSocket, fileName, sizeof(fileName), 0);
        if (bytesReceived <= 0) {
            printf("Error receiving filename.\n");
            closesocket(clientSocket);
            continue;
        }
        fileName[bytesReceived] = '\0';

        printf("Client requested file: %s\n", fileName);

        // Open file and get size and checksum
        FILE *file = fopen(fileName, "rb");
        if (file == NULL) {
            printf("File not found: %s\n", fileName);
            closesocket(clientSocket);
            continue;
        }

        // Calculate file size
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Calculate checksum (sum of all bytes)
        unsigned long checksum = calculate_checksum(file);
        fclose(file);

        printf("Server: File '%s' checksum: %lu\n", fileName, checksum);
        printf("Server: File size: %ld bytes\n", fileSize);

        // Send filename, file size, and checksum to the client
        send(clientSocket, fileName, strlen(fileName) + 1, 0); // Include null terminator
        send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
        send(clientSocket, (char*)&checksum, sizeof(checksum), 0);

        // Send file in chunks
        file = fopen(fileName, "rb");
        char buffer[1024];
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
