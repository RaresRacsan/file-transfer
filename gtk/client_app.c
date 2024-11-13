#include <gtk/gtk.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define CHUNK_SIZE 16384

GtkWidget *entry_server_ip;
GtkWidget *entry_file_name;
GtkWidget *label_status;

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

void request_file (const char *serverIp, const char *fileName) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        gtk_label_set_text(GTK_LABEL(label_status), "Connection failed.");
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Send filename request
    send(clientSocket, fileName, strlen(fileName), 0);

    // Receive file size
    long fileSize;
    recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);

    // Receive file data
    FILE *file = fopen(fileName, "wb");
    char buffer[CHUNK_SIZE];
    int bytesReceived;
    while ((bytesReceived = recv(clientSocket, buffer, CHUNK_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytesReceived, file);
    }
    fclose(file);

    gtk_label_set_text(GTK_LABEL(label_status), "File received successfully.");
    closesocket(clientSocket);
    WSACleanup();
}

// Callback when Request button is clicked
void on_request_button_clicked (GtkWidget *widget, gpointer data) {
    const char *serverIp = gtk_entry_get_text(GTK_ENTRY(entry_server_ip));
    const char *fileName = gtk_entry_get_text(GTK_ENTRY(entry_file_name));

    if(strlen(serverIp) == 0 || strlen(fileName) == 0) {
        gtk_label_set_text(GTK_LABEL(label_status), "Enter server IP and file name.");
        return;
    } 

    gtk_label_set_text(GTK_LABEL(label_status), "Requesting file...");
    request_file(serverIp, fileName);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    apply_css();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *label_ip = gtk_label_new("Server IP:");
    entry_server_ip = gtk_entry_new();
    GtkWidget *label_file = gtk_label_new("File Name:");
    entry_file_name = gtk_entry_new();
    label_status = gtk_label_new("");

    gtk_box_pack_start(GTK_BOX(vbox), label_ip, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_server_ip, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label_file, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_file_name, FALSE, FALSE, 0);

    GtkWidget *button_request = gtk_button_new_with_label("Request File");
    gtk_box_pack_start(GTK_BOX(vbox), button_request, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label_status, FALSE, FALSE, 0);

    g_signal_connect(button_request, "clicked", G_CALLBACK(on_request_button_clicked), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}