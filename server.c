#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384 // 16 KB chunk size

unsigned long calculate_checksum(FILE *file) {
    unsigned long checksum = 0;
    unsigned char buffer[CHUNK_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            checksum += buffer[i];
        }
    }
    fseek(file, 0, SEEK_SET); // Reset file pointer to start for later use
    return checksum;
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
        printf("Server: File '%s' checksum: %lu\n", fileName, checksum);
        printf("Server: File size: %ld bytes\n", fileSize);

        send(clientSocket, fileName, strlen(fileName) + 1, 0);
        send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
        send(clientSocket, (char*)&checksum, sizeof(checksum), 0);

        char buffer[CHUNK_SIZE];
        int bytesRead;

        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            int bytesSent = 0;
            while (bytesSent < bytesRead) {
                int result = send(clientSocket, buffer + bytesSent, bytesRead - bytesSent, 0);
                if (result == SOCKET_ERROR) {
                    printf("Error sending file data. Code: %d\n", WSAGetLastError());
                    fclose(file);
                    closesocket(clientSocket);
                    WSACleanup();
                    return 1;
                }
                bytesSent += result;
            }
        }

        printf("File sent successfully.\n");
        fclose(file);
        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
