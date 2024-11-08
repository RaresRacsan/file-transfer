#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>  // For Windows-specific directory handling

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

// Function to send a directory's files and subdirectories recursively
int send_directory(const char *dirName, SOCKET clientSocket) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    // Construct the search path (use "*" to get all files and directories)
    char searchPath[512];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", dirName);

    // Find the first file or directory
    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Failed to open directory: %s\n", dirName);
        return 0;
    }

    // Loop through the directory contents
    do {
        // Skip "." and ".." directories
        if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
            char fullPath[512];
            snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirName, findFileData.cFileName);

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // It's a subdirectory, send its name and recursively send its contents
                send(clientSocket, "DIR", 3, 0);  // Indicate it's a directory
                send(clientSocket, findFileData.cFileName, strlen(findFileData.cFileName) + 1, 0);  // Send directory name
                send_directory(fullPath, clientSocket);  // Recursively send the subdirectory
            } else {
                // It's a file, send its name and contents
                send(clientSocket, "FILE", 4, 0);  // Indicate it's a file
                send(clientSocket, findFileData.cFileName, strlen(findFileData.cFileName) + 1, 0);  // Send file name

                // Open the file for reading
                FILE *file = fopen(fullPath, "rb");
                if (file == NULL) {
                    printf("Failed to open file: %s\n", fullPath);
                    continue;
                }

                // Send file metadata (size, checksum)
                fseek(file, 0, SEEK_END);
                long fileSize = ftell(file);
                fseek(file, 0, SEEK_SET);
                unsigned long checksum = calculate_checksum(file);

                send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
                send(clientSocket, (char*)&checksum, sizeof(checksum), 0);

                // Send file content in chunks
                unsigned char buffer[CHUNK_SIZE];
                int bytesRead;
                while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    if (send(clientSocket, buffer, bytesRead, 0) == SOCKET_ERROR) {
                        printf("Error sending file content.\n");
                        fclose(file);
                        return 0;
                    }
                }

                fclose(file);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    return 1;
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

        // Receive the command (either "file" or "directory")
        char commandType[32];
        int bytesReceived = recv(clientSocket, commandType, sizeof(commandType), 0);
        if (bytesReceived <= 0) {
            printf("Error receiving command type.\n");
            closesocket(clientSocket);
            continue;
        }

        // Receive the target file/directory name
        char targetName[256];
        bytesReceived = recv(clientSocket, targetName, sizeof(targetName), 0);
        if (bytesReceived <= 0) {
            printf("Error receiving target name.\n");
            closesocket(clientSocket);
            continue;
        }

        if (strcmp(commandType, "file") == 0) {
            // Handle single file request (same as before)
            FILE *file = fopen(targetName, "rb");
            if (file == NULL) {
                printf("File not found: %s\n", targetName);
                closesocket(clientSocket);
                continue;
            }

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);
            unsigned long checksum = calculate_checksum(file);

            send(clientSocket, targetName, strlen(targetName) + 1, 0);
            send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
            send(clientSocket, (char*)&checksum, sizeof(checksum), 0);

            unsigned char buffer[CHUNK_SIZE];
            int bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                if (send(clientSocket, buffer, bytesRead, 0) == SOCKET_ERROR) {
                    printf("Error sending file content.\n");
                    fclose(file);
                    closesocket(clientSocket);
                    return 0;
                }
            }

            printf("File sent successfully.\n");
            fclose(file);
        } else if (strcmp(commandType, "directory") == 0) {
            // Handle directory request
            if (send_directory(targetName, clientSocket)) {
                printf("All files and subdirectories in '%s' sent successfully.\n", targetName);
            } else {
                printf("Error listing files in directory '%s'.\n", targetName);
            }
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
