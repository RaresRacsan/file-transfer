// server_app.c
#include <gtk/gtk.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384 // 16 KB chunk size

GtkWidget *label_status;
SOCKET serverSocket;

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

// Function to handle client connections
DWORD WINAPI client_handler(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char statusMessage[256];

    // Receive the number of files
    int numFiles;
    int bytesReceived = recv(clientSocket, (char*)&numFiles, sizeof(numFiles), 0);
    if (bytesReceived <= 0) {
        printf("Error: Failed to receive number of files.\n");
        closesocket(clientSocket);
        return 1;
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

    printf("All requested files processed for this client.\n");
    closesocket(clientSocket);

    // Update GUI status
    sprintf(statusMessage, "Client connection handled successfully.");
    gtk_label_set_text(GTK_LABEL(label_status), statusMessage);

    return 0;
}

// Function to start the server and listen for connections
void start_server() {
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0) {
        gtk_label_set_text(GTK_LABEL(label_status), "Error: Winsock initialization failed.");
        return;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        gtk_label_set_text(GTK_LABEL(label_status), "Error: Socket creation failed.");
        WSACleanup();
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        gtk_label_set_text(GTK_LABEL(label_status), "Error: Bind failed.");
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        gtk_label_set_text(GTK_LABEL(label_status), "Error: Listen failed.");
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    gtk_label_set_text(GTK_LABEL(label_status), "Server is listening...");

    // Accept clients in a separate thread to keep GUI responsive
    while (1) {
        SOCKET clientSocket;
        struct sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("Error: Accept failed.\n");
            continue;
        }

        printf("Client connected.\n");

        // Create a new thread for each client
        HANDLE hThread = CreateThread(NULL, 0, client_handler, (LPVOID)clientSocket, 0, NULL);
        if (hThread == NULL) {
            printf("Error: Failed to create thread for client.\n");
            closesocket(clientSocket);
        } else {
            CloseHandle(hThread);
        }
    }

    // Should never reach here
    closesocket(serverSocket);
    WSACleanup();
}

// Callback when Start Server button is clicked
void on_start_server_clicked(GtkWidget *widget, gpointer data) {
    // Disable the button to prevent multiple clicks
    gtk_widget_set_sensitive(widget, FALSE);
    gtk_label_set_text(GTK_LABEL(label_status), "Starting server...");

    // Start server in a separate thread to keep GUI responsive
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_server, NULL, 0, NULL);
    if (hThread == NULL) {
        gtk_label_set_text(GTK_LABEL(label_status), "Error: Failed to start server thread.");
    } else {
        CloseHandle(hThread);
    }
}

// Main function
int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Server");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    // Create layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create widgets
    label_status = gtk_label_new("Server is not running.");

    // Add widgets to layout
    gtk_box_pack_start(GTK_BOX(vbox), label_status, FALSE, FALSE, 0);

    // Create and connect "Start Server" button
    GtkWidget *button_start_server = gtk_button_new_with_label("Start Server");
    gtk_box_pack_start(GTK_BOX(vbox), button_start_server, FALSE, FALSE, 0);

    // Connect signals
    g_signal_connect(button_start_server, "clicked", G_CALLBACK(on_start_server_clicked), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Show window
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
