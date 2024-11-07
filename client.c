#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>   // Windows socket library
#include <ws2tcpip.h>   // Additional networking functions

#pragma comment(lib, "ws2_32.lib"); // Link with Winsock library

int main(int argc, char *argv[]) {
    // Initialize Winsock
    if(argc != 3) {
        printf("Usage: %s <server_ip> <filename>\n", argv[0]);
        return 1;
    }

    const char *serverIp = argv[1];
    const char *fileName = argv[2];

    WSADATA wsaData;

    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(wsaInit != 0) {
        printf("Winsock initialization failed. Error code: %d\n", wsaInit);
        return 1;
    }
    printf("Winsock initialized.\n");

    // Create and connect the client socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);  // Same port as server
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    // Convert IP address from text to binary
    if(serverAddr.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid address/ Address not supported.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if(connect(clientSocket, (struct sockaddr*)& serverAddr, sizeof(serverAddr)) < 0) {
        printf("Connection failed. Error code: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    printf("Connected to server.\n");

    // Send filename to server
    if(send(clientSocket, fileName, strlen(fileName), 0) == SOCKET_ERROR) {
        printf("Failed to send filename. Error code: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    printf("Requested file: %s\n", fileName);

    // Receive file data and save to disk
    // Open a file to save the incoming data
    FILE *file = fopen(fileName, "wb");
    if(file == NULL) {
        printf("Failed to open file for writing.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    char buffer[1024];
    int bytesReceived;

    // Receive data in chunks and write to file
    while((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesReceived, file);
    }

    if(bytesReceived < 0) {
        printf("Failed to receive file data. Error Code: %d\n", WSAGetLastError());
    }
    else {
        printf("File reveived and saved successfully.\n");
    }

    fclose(file);
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}