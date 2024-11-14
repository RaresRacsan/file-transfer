#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384 // 16 KB chunk size
#define BUFFER_SIZE 1024

void sanitize_filename(char *fileName) {
    const char *invalidChars = "\\/*?:\"<>|";
    for (int i = 0; fileName[i]; i++) {
        if (strchr(invalidChars, fileName[i])) {
            fileName[i] = '_';
        }
    }
}

void request_file_transfer(SOCKET server_socket, const char *filename) {
    char buffer[BUFFER_SIZE];

    // Send file transfer request
    snprintf(buffer, BUFFER_SIZE, "REQUEST_TRANSFER %s", filename);
    send(server_socket, buffer, strlen(buffer), 0);

    // Receive server response
    int bytes_received = recv(server_socket, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received] = '\0';

    if (strncmp(buffer, "TRANSFER", 8) == 0) {
        char *file_info = buffer + 9;
        char server_filename[256];
        long fileSize;
        unsigned long serverChecksum;

        sscanf(file_info, "%s %ld %lu", server_filename, &fileSize, &serverChecksum);

        printf("Server wants to transfer file: %s\n", server_filename);
        printf("File size: %ld bytes\n", fileSize);
        printf("File checksum: %lu\n", serverChecksum);

        // Ask user to accept or decline
        printf("Do you want to accept the file transfer? (yes/no): ");
        char response[10];
        scanf("%s", response);

        if (strcmp(response, "yes") == 0) {
            send(server_socket, "ACCEPT_TRANSFER", 15, 0);

            FILE *file = fopen(server_filename, "wb");
            if (file == NULL) {
                printf("Error: Failed to create file for writing.\n");
                return;
            }

            char file_buffer[CHUNK_SIZE];
            int bytesReceived;
            unsigned long clientChecksum = 0;

            printf("Receiving file content...\n");
            while ((bytesReceived = recv(server_socket, file_buffer, sizeof(file_buffer), 0)) > 0) {
                fwrite(file_buffer, 1, bytesReceived, file);
                clientChecksum += calculate_checksum((unsigned char *)file_buffer, bytesReceived);
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
        } else {
            send(server_socket, "DECLINE_TRANSFER", 15, 0);
        }
    }
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

    request_file_transfer(clientSocket, fileName);

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}