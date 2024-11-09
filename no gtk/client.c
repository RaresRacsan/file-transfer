#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384 // 16 KB chunk size

// Function to calculate the checksum (sum of all bytes)
unsigned long calculate_checksum(unsigned char *data, size_t dataSize) {
    unsigned long checksum = 0;
    for (size_t i = 0; i < dataSize; i++) {
        checksum += data[i];
    }
    return checksum;
}

// Function to sanitize filenames (remove/ replace with '_')
void sanitize_filename (char *fileName) {
    // List of invalid characters for filenames in Windows
    const char *invalidChars = "\\/*?:\"<>|";

    for(int i = 0; fileName[i]; i++) {
        if(strchr(invalidChars, fileName[i])) {
                fileName[i] = '_';
            }
    }
}

// Function to check if a file exists and prompt user for action (overwrite or rename)
int file_exists (char *fileName) {
    if(_access(fileName, 0) != -1) {    // Check if file exists
        printf("Warning: File '%s' already exists.\n", fileName);

        printf("Do you want to overwrite it? (y/n): ");
        char response;
        scanf("%c", &response);

        if(response != 'y' && response != 'Y') {
            char newFileName[256];
            printf("Enter a new filename: ");
            scanf("%s", newFileName);
            strcpy(fileName, newFileName);
        }
        return 1;   // File exists
    }
    return 0;       // File doesn't exist
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
        printf("Error: Winsock initialization failed. Error code: %d\n", wsaInit);
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Error: Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error: Connection failed. Error code: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    printf("Connected to server at %s...\n", serverIp);

    // Send filename to server
    send(clientSocket, fileName, strlen(fileName), 0);
    printf("Requested file: '%s'...\n", fileName);

    // Receive the file metadata
    char receivedFileName[256];
    long fileSize;
    unsigned long serverChecksum;
    
    recv(clientSocket, receivedFileName, sizeof(receivedFileName), 0);
    recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
    recv(clientSocket, (char*)&serverChecksum, sizeof(serverChecksum), 0);

    //sanitize_filename(receivedFileName);

    printf("Received file metadata:\n");
    printf("File: %s\n", receivedFileName);
    printf("Size: %ld bytes\n", fileSize);
    printf("Checksum: %lu\n", serverChecksum);

    // Check if file already exists and handle renaming
    if(file_exists(receivedFileName)) {
        printf("Proceeding with renamed or overwritten file: %s.\n", receivedFileName);
    }
    
    FILE *file = fopen(receivedFileName, "wb");
    if (file == NULL) {
        printf("Error: Failed to create file for writing.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    char buffer[CHUNK_SIZE];
    int bytesReceived;
    unsigned long clientChecksum = 0;

    printf("Receiving file content...\n");
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesReceived, file);
        clientChecksum += calculate_checksum(buffer, bytesReceived);
    }

    printf("File received.\n");

    if (ftell(file) != fileSize) {
        printf("Error: File corruption detected (size mismatch).\n");
    } else {
        if (clientChecksum == serverChecksum) {
            printf("Success: File checksum matches. File is valid.\n");
        } else {
            printf("Error: File checksum does not match. File may be corrupted.\n");
        }
    }

    fclose(file);
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}