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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <file_name>\n", argv[0]);
        return 1;
    }

    const char *serverIp = argv[1];
    const char *fileName = argv[2];

    // Initialize Winsock
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0) {
        printf("Winsock initialization failed. Error code: %d\n", wsaInit);
        return 1;
    }

    // Create client socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Setup server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Connection failed. Error code: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    printf("Connected to server.\n");

    // Send filename to server
    send(clientSocket, fileName, strlen(fileName), 0);
    printf("Requested file: %s\n", fileName);

    // Receive filename, file size, and checksum from the server
    char receivedFileName[256];
    long fileSize;
    unsigned long serverChecksum;
    
    recv(clientSocket, receivedFileName, sizeof(receivedFileName), 0);
    recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
    recv(clientSocket, (char*)&serverChecksum, sizeof(serverChecksum), 0);

    // Open file to write
    FILE *file = fopen(receivedFileName, "wb");
    if (file == NULL) {
        printf("Failed to create file for writing.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    char buffer[1024];
    int bytesReceived;
    long bytesReceivedTotal = 0;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesReceived, file);
        bytesReceivedTotal += bytesReceived;
    }

    printf("File received: %ld bytes\n", bytesReceivedTotal);

    // Check if file size matches
    if (bytesReceivedTotal != fileSize) {
        printf("File corruption detected: sizes do not match!\n");
    } else {
        // Check file integrity by checksum
        unsigned long receivedChecksum = calculate_checksum(file);
        if (receivedChecksum != serverChecksum) {
            printf("File corruption detected: checksums do not match!\n");
        } else {
            printf("File integrity verified.\n");
        }
    }

    fclose(file);
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
