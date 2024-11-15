// server.c
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
void sanitize_filename(char *fileName) {
    // List of invalid characters for filenames in Windows
    const char *invalidChars = "\\/*?:\"<>|";

    for(int i = 0; fileName[i]; i++) {
        if(strchr(invalidChars, fileName[i])) {
                fileName[i] = '_';
            }
    }
}

int main() {
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0) {
        printf("Error: Winsock initialization failed. Error code: %d\n", wsaInit);
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        printf("Error: Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Error: Bind failed. Error code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 3) == SOCKET_ERROR) {
        printf("Error: Listen failed. Error code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening on port %d...\n", PORT);
    printf("Waiting for clients to connect.\n");

    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    while ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize)) != INVALID_SOCKET) {
        printf("Client connected.\n");

        // Receive the number of files
        int numFiles;
        int bytesReceived = recv(clientSocket, (char*)&numFiles, sizeof(numFiles), 0);
        if (bytesReceived <= 0) {
            printf("Error: Failed to receive number of files.\n");
            closesocket(clientSocket);
            continue;
        }

        numFiles = ntohl(numFiles); // Convert from network byte order

        printf("Client requested %d file(s).\n", numFiles);

        for(int i = 0; i < numFiles; i++) {
            char fileName[256];
            // Receive each file name
            bytesReceived = recv(clientSocket, fileName, sizeof(fileName), 0);
            if (bytesReceived <= 0) {
                printf("Error: Failed to receive filename.\n");
                break;
            }
            fileName[bytesReceived] = '\0';

            // Sanitize filename to avoid illegal characters
            sanitize_filename(fileName);

            FILE *file = fopen(fileName, "rb");
            if (file == NULL) {
                printf("Error: File %s not found.\n", fileName);
                // Send an error flag to client
                int fileExists = htonl(0); // 0 indicates file not found
                send(clientSocket, (char*)&fileExists, sizeof(fileExists), 0);
                continue;
            }

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            unsigned long checksum = calculate_checksum(file);
            fclose(file);

            printf("Server: File '%s' found.\n", fileName);
            printf("Server: File size is %ld bytes.\n", fileSize);
            printf("Server: File checksum is %lu.\n", checksum);

            // Send a success flag to client
            int fileExists = htonl(1); // 1 indicates file exists
            send(clientSocket, (char*)&fileExists, sizeof(fileExists), 0);

            // Send file metadata (name, size, checksum)
            send(clientSocket, fileName, strlen(fileName) + 1, 0);
            long networkFileSize = htonl(fileSize);
            send(clientSocket, (char*)&networkFileSize, sizeof(networkFileSize), 0);
            unsigned long networkChecksum = htonl(checksum);
            send(clientSocket, (char*)&networkChecksum, sizeof(networkChecksum), 0);

            // Ask client if they want to accept the file
            char acceptMessage[] = "Do you want to accept the file? (y/n): ";
            send(clientSocket, acceptMessage, strlen(acceptMessage) + 1, 0);

            char clientResponse;
            recv(clientSocket, &clientResponse, sizeof(clientResponse), 0);

            if (clientResponse != 'y' && clientResponse != 'Y') {
                printf("Client declined the file '%s'.\n", fileName);
                continue;
            }

            file = fopen(fileName, "rb");
            if (file == NULL) {
                printf("Error: Failed to open file '%s' for reading.\n", fileName);
                continue;
            }

            // Send file content in chunks
            char buffer[CHUNK_SIZE];
            int bytesReadFile;

            while ((bytesReadFile = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                send(clientSocket, buffer, bytesReadFile, 0);
            }

            printf("File '%s' sent successfully.\n", fileName);
            fclose(file);
        }

        printf("All requested files processed.\n");
        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
