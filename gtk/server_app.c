#include <gtk/gtk.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384

GtkWidget *label_status;
GtkWidget *label_file_requested;

SOCKET serverSocket;
SOCKET clientSocket;

void apply_css(void) {
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
        "button {"
        "   background: #2d2d2d;"
        "   color: #ffffff;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "   margin: 5px;"
        "}"
        "button:hover {"
        "   background: #3d3d3d;"
        "}"
        "button#accept {"
        "   background: #28a745;"
        "}"
        "button#accept:hover {"
        "   background: #218838;"
        "}"
        "button#decline {"
        "   background: #dc3545;"
        "}"
        "button#decline:hover {"
        "   background: #c82333;"
        "}", -1, NULL);

        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);
}

// Function to send the file if the request is accepted
void send_file(const char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if(file == NULL) {
        gtk_label_set_text(GTK_LABEL(label_status), "File not found.");
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Send the file size
    send(clientSocket, (char*) &fileSize, sizeof(fileSize), 0);

    // Send file data in chunks
    char buffer[CHUNK_SIZE];
    int bytesRead;
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        send(clientSocket, buffer, bytesRead, 0);
    }

    fclose(file);
    gtk_label_set_text(GTK_LABEL(label_status), "File sent successfully.");
}

// Callback when Accept button is clicked
void on_accept_button_clicked(GtkWidget *widget, gpointer data) {
    const char *fileName = gtk_label_get_text(GTK_LABEL(label_file_requested));
    send_file(fileName);
    closesocket(clientSocket);
}

// Callback when Decline button is clicked
void on_decline_button_clicked(GtkWidget *widget, gpointer data) {
    gtk_label_set_text(GTK_LABEL(label_status), "File request declined.");
    closesocket(clientSocket);
}

void start_server() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (const struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 3);

    gtk_label_set_text(GTK_LABEL(label_status), "Server listening for requests...");

    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

    char fileName[256];
    recv(clientSocket, fileName, sizeof(fileName), 0);
    gtk_label_set_text(GTK_LABEL(label_file_requested), fileName);
    gtk_label_set_text(GTK_LABEL(label_status), "File request received.");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    apply_css();

    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Server application");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    // Create a vertical box layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Status and requested file labels
    label_status = gtk_label_new("Starting server...");
    label_file_requested = gtk_label_new("No file requested.");
    gtk_box_pack_start(GTK_BOX(vbox), label_status, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label_file_requested, FALSE, FALSE, 0);

    // Accept and decline buttons
    GtkWidget *button_accept = gtk_button_new_with_label("Accept");
    GtkWidget *button_decline = gtk_button_new_with_label("Decline");
    gtk_box_pack_start(GTK_BOX(vbox), button_accept, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button_decline, FALSE, FALSE, 0);

    // Connect button signals to their callbacks
    g_signal_connect(button_accept, "clicked", G_CALLBACK(on_accept_button_clicked), NULL);
    g_signal_connect(button_decline, "clicked", G_CALLBACK(on_decline_button_clicked), NULL);

    // Show all widgets
    gtk_widget_show_all(window);

     // Start the server in a separate thread to listen for requests
    g_idle_add((GSourceFunc)start_server, NULL);

    // Signal to close the window
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_main();

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}