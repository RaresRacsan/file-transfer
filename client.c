#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384 // 16 KB chunk size

unsigned long calculate_checksum(unsigned char *data, size_t dataSize) {
    unsigned long checksum = 0;
    for (size_t i = 0; i < dataSize; i++) {
        checksum += data[i];
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

    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0) {
        printf("Winsock initialization failed. Error code: %d\n", wsaInit);
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

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

    send(clientSocket, fileName, strlen(fileName), 0);
    printf("Requested file: %s\n", fileName);

    char receivedFileName[256];
    long fileSize;
    unsigned long serverChecksum;
    
    recv(clientSocket, receivedFileName, sizeof(receivedFileName), 0);
    recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
    recv(clientSocket, (char*)&serverChecksum, sizeof(serverChecksum), 0);

    FILE *file = fopen(receivedFileName, "wb");
    if (file == NULL) {
        printf("Failed to create file for writing.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    char buffer[CHUNK_SIZE];
    int bytesReceived;
    unsigned long clientChecksum = 0;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesReceived, file);
        clientChecksum += calculate_checksum(buffer, bytesReceived);
    }

    if (ftell(file) != fileSize) {
        printf("File corruption detected (size mismatch).\n");
    } else {
        if (clientChecksum == serverChecksum) {
            printf("File checksum matches. File is valid.\n");
        } else {
            printf("File checksum does not match. File may be corrupted.\n");
        }
    }

    fclose(file);
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}