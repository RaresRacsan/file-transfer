// client_app.c
#include <gtk/gtk.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080   // Server port
#define CHUNK_SIZE 16384

GtkWidget *entry_server_ip;
GtkWidget *entry_file_names;
GtkWidget *label_status;

// Function to apply custom styles to widgets
void apply_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window {"
        "   background-color: #1e1e1e;"
        "}"
        "label {"
        "   color: #ffffff;"
        "   font-size: 12px;"
        "   margin: 10px;"
        "}"
        "entry {"
        "   background: #2d2d2d;"
        "   color: #ffffff;"
        "   border: 1px solid #3d3d3d;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "   margin: 5px;"
        "}"
        "entry:focus {"
        "   border-color: #0066cc;"
        "}"
        "button {"
        "   background: #28a745;"
        "   color: #ffffff;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "   margin: 10px;"
        "}"
        "button:hover {"
        "   background: #218838;"
        "}", -1, NULL);

        GdkScreen *screen = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

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
        scanf(" %c", &response);

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

// Function to request files from server
void request_files(const char *serverIp, char **fileNames, int numFiles) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Create socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        gtk_label_set_text(GTK_LABEL(label_status), "Connection failed.");
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Send the number of files
    int networkNumFiles = htonl(numFiles);
    send(clientSocket, (char*)&networkNumFiles, sizeof(networkNumFiles), 0);

    // Send each file name
    for(int i = 0; i < numFiles; i++) {
        send(clientSocket, fileNames[i], strlen(fileNames[i]) + 1, 0);
        printf("Requested file: '%s'...\n", fileNames[i]);
    }

    // Receive and process each file
    for(int i = 0; i < numFiles; i++) {
        // Receive file existence flag
        int fileExists;
        int bytesReceived = recv(clientSocket, (char*)&fileExists, sizeof(fileExists), 0);
        if (bytesReceived <= 0) {
            printf("Error: Failed to receive file existence flag.\n");
            break;
        }
        fileExists = ntohl(fileExists);

        if(fileExists == 0) {
            gtk_label_set_text(GTK_LABEL(label_status), "One or more files not found on server.");
            continue;
        }

        // Receive file metadata
        char receivedFileName[256];
        long fileSize;
        unsigned long serverChecksum;

        recv(clientSocket, receivedFileName, sizeof(receivedFileName), 0);
        recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
        recv(clientSocket, (char*)&serverChecksum, sizeof(serverChecksum), 0);

        // Convert file size from network byte order
        fileSize = ntohl(fileSize);
        serverChecksum = ntohl(serverChecksum);

        printf("\nReceived file metadata:\n");
        printf("File: %s\n", receivedFileName);
        printf("Size: %ld bytes\n", fileSize);
        printf("Checksum: %lu\n", serverChecksum);

        // Check if file already exists and handle renaming
        if(file_exists(receivedFileName)) {
            printf("Proceeding with renamed or overwritten file: %s.\n", receivedFileName);
        }

        // Receive the prompt from the server
        char acceptMessage[256];
        recv(clientSocket, acceptMessage, sizeof(acceptMessage), 0);
        printf("%s", acceptMessage);

        // Send the client's response
        char response;
        scanf(" %c", &response);
        send(clientSocket, &response, sizeof(response), 0);

        if (response != 'y' && response != 'Y') {
            printf("File transfer declined.\n");
            continue;
        }

        FILE *file = fopen(receivedFileName, "wb");
        if (file == NULL) {
            printf("Error: Failed to create file for writing.\n");
            continue;
        }

        char buffer[CHUNK_SIZE];
        int bytesReceivedFile;
        unsigned long clientChecksum = 0;

        printf("Receiving file content...\n");
        while ((bytesReceivedFile = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
            fwrite(buffer, 1, bytesReceivedFile, file);
            clientChecksum += calculate_checksum((unsigned char*)buffer, bytesReceivedFile);
            if(clientChecksum >= serverChecksum) { // Simple check to prevent over-reading
                break;
            }
        }

        printf("File received.\n");

        if (_filelength(_fileno(file)) != fileSize) {
            printf("Error: File corruption detected (size mismatch).\n");
        } else {
            if (clientChecksum == serverChecksum) {
                printf("Success: File checksum matches. File is valid.\n");
            } else {
                printf("Error: File checksum does not match. File may be corrupted.\n");
            }
        }

        fclose(file);
    }

    gtk_label_set_text(GTK_LABEL(label_status), "All requested files processed.");
    closesocket(clientSocket);
    WSACleanup();
}

// Callback when Request button is clicked
void on_request_button_clicked (GtkWidget *widget, gpointer data) {
    const char *serverIp = gtk_entry_get_text(GTK_ENTRY(entry_server_ip));
    const char *fileNamesStr = gtk_entry_get_text(GTK_ENTRY(entry_file_names));

    // Parse file names separated by commas
    char *fileNamesCopy = strdup(fileNamesStr);
    int numFiles = 0;
    char *token = strtok(fileNamesCopy, ",");
    char *fileNames[100]; // Maximum 100 files

    while(token != NULL && numFiles < 100) {
        // Trim leading and trailing whitespace
        while(isspace(*token)) token++;
        char *end = token + strlen(token) - 1;
        while(end > token && isspace(*end)) end--;
        *(end+1) = '\0';

        fileNames[numFiles++] = token;
        token = strtok(NULL, ",");
    }

    free(fileNamesCopy);

    // Validating input
    if(strlen(serverIp) == 0 || numFiles == 0) {
        gtk_label_set_text(GTK_LABEL(label_status), "Enter server IP and at least one file name.");
        return;
    } 

    gtk_label_set_text(GTK_LABEL(label_status), "Requesting files...");
    request_files(serverIp, fileNames, numFiles);
}

// Main function
int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Apply custom styles
    apply_css();

    // Create main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 250);

    // Create layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create widgets
    GtkWidget *label_ip = gtk_label_new("Server IP:");
    entry_server_ip = gtk_entry_new();
    GtkWidget *label_files = gtk_label_new("File Names (comma-separated):");
    entry_file_names = gtk_entry_new();
    label_status = gtk_label_new("");

    // Add widgets to layout
    gtk_box_pack_start(GTK_BOX(vbox), label_ip, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_server_ip, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label_files, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_file_names, FALSE, FALSE, 0);

    // Create and connect "Request" button
    GtkWidget *button_request = gtk_button_new_with_label("Request Files");
    gtk_box_pack_start(GTK_BOX(vbox), button_request, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label_status, FALSE, FALSE, 0);

    // Connect signals
    g_signal_connect(button_request, "clicked", G_CALLBACK(on_request_button_clicked), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Show window
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
