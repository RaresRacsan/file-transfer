#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>   // Windows socket library
#include <ws2tcpip.h>   // Additional networking functions

#pragma comment(lib, "ws2_32.lib")  // Link with Winsock library

int main() {
    // Initializing Winsock
    WSADATA wsaData;

    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(wsaInit != 0) {
        printf("Winsock initialization failed. Error code: %d\n", wsaInit);
        return 1;
    }

    printf("Winsock initialized!\n");

    // Create and bind the Server Socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in  serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);  // Port number 8080
    serverAddr.sin_addr.s_addr = INADDR_ANY;    // Listen to any IP address

    if(bind(serverSocket, (const struct sockaddr*)& serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed. Error code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if(listen(serverSocket, 3) == SOCKET_ERROR) {
        printf("Listen failed. Error code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    printf("Listening on port 8080...\n");

    // Accept client connections and handle file requests
    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    while((clientSocket = accept(serverSocket, (struct sockaddr*)& clientAddr, &clientAddrSize)) != INVALID_SOCKET){
        printf("Client connected.\n");

        // Receive filename from the client
        char fileName[256];
        int bytesReceived = recv(clientSocket, fileName, sizeof(fileName), 0);

        if(bytesReceived > 0) {
            fileName[bytesReceived] = '\0';     // Null-terminate the filename
            printf("CLient requeste file: %s\n", fileName);

            // Open the requeste file
            FILE *file = fopen(fileName, "rb");
            if(file == NULL) {
                printf("File is not found: %s\n", fileName);
                closesocket(clientSocket);
                continue;
            }

            // Read file and send to client
            char buffer[1024];
            int bytesRead;

            while((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                send(clientSocket, buffer, bytesRead, 0);
            }

            printf("File sent succesfully.\n");
            fclose(file);
        }
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
}