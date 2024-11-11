# File Transfer Application in C (TCP)

A simple file transfer application built with C using TCP sockets. This application includes a **server** that listens for incoming client connections and serves files, and a **client** that requests a file from the server and saves it locally.

## Requirements

To run this application, you will need:
1. **MinGW or GCC compiler** (to compile C code on Windows).
2. **Visual Studio Code** (optional, for editing and managing the code).
3. **Git** (optional, for cloning this repository and version control).

### Downloads

1. **Download MinGW**:
   - Go to [MinGW's SourceForge page](https://sourceforge.net/projects/mingw/files/latest/download).
   - Install **MinGW** and ensure that `gcc` is included in the installation.
   - Add MinGW's `bin` folder (e.g., `C:\MinGW\bin`) to your **PATH** environment variable.

2. **Install Visual Studio Code** (optional):
   - Download and install from [Visual Studio Code's website](https://code.visualstudio.com/).
   - You can open the project folder in VS Code to easily manage and compile files.

3. **GTK 3** (for graphical user interface).

### Verify GCC Installation

To ensure GCC is installed correctly, open a terminal or command prompt and type:
```bash
gcc --version
```
You should see version information if GCC is installed properly.

## Downloads
1. **Download MinGW**:
   - go to MinGW's SourceForge page.
   - Install MinGW and ensure that gcc is included in the installation.
   - Add MinGW's bin folder (e.g., C:\MinGW\bin) to your PATH environment variable.
2. **Install GTK 3**:
   - Download GTK 3 for Windows from GTK's official website.
   - Install GTK and ensure the required DLLs are placed in your system’s PATH or alongside the compiled executables.
   Alternatively, if you're using MSYS2:
   - Open the MSYS2 terminal and run:
   ```bash
   pacman -S mingw-w64-x86_64-gtk3
   ```

## Getting Started
1. **Clone this repository**
2. **Open the project in your preferred IDE or text editor**
   - You can use Visual Studio Code, Code::Blocks, Dev-C++, Clion, or any other IDE text editor that supports C programming.
   - Open the project folder in your chosen editor to manage and edit files easily.


## Compilation - without GTK
Compile the server and client applications as follows:
1. **Compile the server**
```bash
gcc server.c -o server.exe -lws2_32
```
2. **Compile the client**
```bash
gcc client.c -o client.exe -lws2_32
```
This will produce server.exe and client.exe.

## Running the Application - without GTK
### Step 1: Start the Server
1. Open a terminal or command prompt in the project directory.
2. Run the server application:
```bash
./server.exe
```
### Step 2: Run the Client
1. In a **separate terminal** or on a **different machine**, navigate to the project directory.
2. Run the client, specifying the server's IP address and the filename you want to download:
```bash
./client.exe <server_ip> <filename>
```
   - Replace <server_ip> with the server's IP address (e.g., 127.0.0.1 if running locally).
   - Replace <filename> with the name of the file you want to download from the server's directory.
```bash
./client.exe 127.0.0.1 testfile.txt
```
The client will connect to the server, request the specified file, and save it locally with the same filename.

## Compile Server and client - with GTK
1. Set up pkg-config to find GTK:
   - If you are using MSYS2 or another package manager like vcpkg, ensure the pkg-config path is configured properly.
   In MSYS2, you can run the following command:
   ```bash
   export PKG_CONFIG_PATH="/mingw64/lib/pkgconfig"
   ```
2. Update c_cpp_properties.json in Visual Studio Code: To ensure Visual Studio Code recognizes GTK and other libraries, update the c_cpp_properties.json file to include the path to GTK and other necessary libraries:
   Example:
   - Press Ctrl + Shift + P and search for C/C++: Edit Configurations (UI).
   - In the Include path section, add paths like:
   ```bash
    "C:/msys64/mingw64/include/gtk-3.0",
    "C:/msys64/mingw64/include/glib-2.0",
    "C:/msys64/mingw64/lib/glib-2.0/include",
    "C:/msys64/mingw64/include/pango-1.0",
    "C:/msys64/mingw64/include/cairo",
    "C:/msys64/mingw64/include/gdk-pixbuf-2.0",
    "C:/msys64/mingw64/include/atk-1.0",
    "C:/msys64/mingw64/include/harfbuzz"
   ```
3. Compile the server and client:
   - Compile the server application:
   ```bash
   gcc server_app.c -o server_app.exe `pkg-config --cflags --libs gtk+-3.0` -lws2_32
   ```
   - Compile the client application:
   ```bash
   gcc client_app.c -o client_app.exe `pkg-config --cflags --libs gtk+-3.0` -lws2_32
   ```
   This will produce server_app.exe and client_app.exe in your project folder.

## Running the application - with GTK
### Step 1: Start the Server
1. Open a terminal or command prompt in the project directory.
2. Run the server application:
   ```bash
   ./server_app.exe
   ```
### Step 2: Run the Client
1. In a separate terminal or on a different machine, navigate to the project directory.
2. Run the client, specifying the server's IP address and the filename you want to download:
   ```bash
   ./client_app.exe
   ```

## Troubleshooting
1. **Connection refused (10061)**: Ensure the server is running and listening on the specified IP and PORT.
2. **Invalid IP and PORT**: Verify both the client and server are using the correct IP and PORT.
3. **Firewall or Antivirus**: Temporarily disable firewall or anitivirus software if they block connections.

## Notes
   - Ensure the server and client are on the same network, or adjust router and firewall settings to allow communication.
   - This application is designed for basic file transfers and currently supports single-client connections.

## Future Enhancements - Tracker
1. **Multiple File Transfers:**
   - Support transferring multiple files at once or in a batch. This could involve sending an array of file names and then transferring each file one after another.
2. **Directory Transfers:**
   - Allow transferring entire directories, which could involve recursively sending all files and folders, along with metadata.
3. **Compression:**
   - To reduce transfer time, you could compress the files before sending and decompress them on the receiving side.
4. **Encryption:**
   - Implement encryption (like SSL/TLS) for secure file transfers, especially if you plan to send sensitive data.
5. **User interface**
   - for the application with GTK
   - for the application without GTK ✔
7. **Error handling and other problems:** ✔
   - file overwritting, special file types, file integrity check. ✔

## License
This project is open-source and available under the MIT License.
