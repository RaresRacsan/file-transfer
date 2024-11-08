#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>  // For directory handling

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384 // 16 KB chunk size

// Function to calculate checksum of received data
unsigned long calculate_checksum(unsigned char *data, size_t dataSize) {
    unsigned long checksum = 0;
    for (size_t i = 0; i < dataSize; i++) {
        checksum += data[i];
    }
    return checksum;
}

// Function to create directories recursively
int create_directories(const char *path) {
    char dir[512];
    strcpy(dir, path);

    // Create directory and handle errors
    if (_mkdir(dir) == -1 && errno != EEXIST) {
        printf("Error creating directory: %s\n", dir);
        return 0;
    }
    return 1;
}

// Function to handle receiving and saving files
int receive_file(SOCKET clientSocket) {
    char fileName[256];
    long fileSize;
    unsigned long serverChecksum;

    // Receive file name, size, and checksum
    int bytesReceived = recv(clientSocket, fileName, sizeof(fileName), 0);
    if (bytesReceived <= 0) {
        printf("Error receiving file name.\n");
        return 0;
    }

    bytesReceived = recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
    if (bytesReceived <= 0) {
        printf("Error receiving file size.\n");
        return 0;
    }

    bytesReceived = recv(clientSocket, (char*)&serverChecksum, sizeof(serverChecksum), 0);
    if (bytesReceived <= 0) {
        printf("Error receiving file checksum.\n");
        return 0;
    }

    FILE *file = fopen(fileName, "wb");
    if (file == NULL) {
        printf("Error opening file to save: %s\n", fileName);
        return 0;
    }

    unsigned char buffer[CHUNK_SIZE];
    size_t totalBytesReceived = 0;

    // Receive file data in chunks
    while (totalBytesReceived < fileSize) {
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            printf("Error receiving file content.\n");
            fclose(file);
            return 0;
        }
        fwrite(buffer, 1, bytesRead, file);
        totalBytesReceived += bytesRead;
    }

    // Verify checksum
    fseek(file, 0, SEEK_SET);
    unsigned char *fileData = (unsigned char*)malloc(fileSize);
    fread(fileData, 1, fileSize, file);
    unsigned long checksum = calculate_checksum(fileData, fileSize);
    if (checksum != serverChecksum) {
        printf("Checksum mismatch! Received file might be corrupted.\n");
    } else {
        printf("File received and checksum validated.\n");
    }

    free(fileData);
    fclose(file);
    return 1;
}

// Main client logic to connect to the server and request files or directories
int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Winsock initialization failed.\n");
        return 1;
    }

    // Create client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set up server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Assuming server is running locally

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connection failed. Error code: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Client interaction loop (request files or directories)
    char command[32];
    char targetName[256];

    printf("Enter command (file/directory): ");
    scanf("%s", command);
    printf("Enter file or directory name: ");
    scanf("%s", targetName);

    // Send command type to server (either "file" or "directory")
    if (send(clientSocket, command, strlen(command) + 1, 0) == SOCKET_ERROR) {
        printf("Error sending command.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Send target name (file or directory)
    if (send(clientSocket, targetName, strlen(targetName) + 1, 0) == SOCKET_ERROR) {
        printf("Error sending target name.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Handle receiving directory or file
    if (strcmp(command, "file") == 0) {
        if (!receive_file(clientSocket)) {
            printf("Error receiving file.\n");
        }
    } else if (strcmp(command, "directory") == 0) {
        printf("Directory request sent, waiting for server.\n");
        while (1) {
            char response[256];
            int bytesReceived = recv(clientSocket, response, sizeof(response), 0);
            if (bytesReceived <= 0) {
                break;
            }

            if (strcmp(response, "DIR") == 0) {
                // Directory handling logic (create the directory on the client)
                char dirName[256];
                recv(clientSocket, dirName, sizeof(dirName), 0);
                create_directories(dirName);
                printf("Directory created: %s\n", dirName);
            } else if (strcmp(response, "FILE") == 0) {
                // File handling logic (receive and save the file)
                if (!receive_file(clientSocket)) {
                    printf("Error receiving file.\n");
                }
            }
        }
    }

    // Cleanup and close
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
