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

        int nr_of_files;
        recv(clientSocket, (char*)&nr_of_files, sizeof(nr_of_files), 0);

        for (int i = 0; i < nr_of_files; i++) {
            char fileName[256];
            int bytesReceived = recv(clientSocket, fileName, sizeof(fileName), 0);
            if (bytesReceived <= 0) {
                printf("Error: Failed to receive filename.\n");
                closesocket(clientSocket);
                continue;
            }
            fileName[bytesReceived] = '\0';

            // Sanitize filename to avoid illegal characters
            sanitize_filename(fileName);

            FILE *file = fopen(fileName, "rb");
            if (file == NULL) {
                printf("Error: File %s not found.\n", fileName);
                closesocket(clientSocket);
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

            // Send file metadata (name, size, checksum)
            send(clientSocket, fileName, strlen(fileName) + 1, 0);
            send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
            send(clientSocket, (char*)&checksum, sizeof(checksum), 0);

            // Ask client if they want to accept the file
            char acceptMessage[] = "Do you want to accept the file? (y/n): ";
            send(clientSocket, acceptMessage, sizeof(acceptMessage), 0);

            char clientResponse;
            recv(clientSocket, &clientResponse, sizeof(clientResponse), 0);

            if (clientResponse != 'y' && clientResponse != 'Y') {
                printf("Client declined the file.\n");
                continue;
            }

            file = fopen(fileName, "rb");

            // Send file content in chunks
            char buffer[CHUNK_SIZE];
            int bytesRead;

            while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                send(clientSocket, buffer, bytesRead, 0);
            }

            printf("File '%s' sent successfully.\n", fileName);
            fclose(file);
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}