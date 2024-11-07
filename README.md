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

3. **Install Git** (optional):
   - Download and install from [Git's website](https://git-scm.com/).
   - Use Git to clone this repository for quick setup.

### Verify GCC Installation

To ensure GCC is installed correctly, open a terminal or command prompt and type:
```bash
gcc --version
```
You should see version information if GCC is installed properly.

## Getting Started
1. **Clone this repository**
2. **Open the project in your preferred IDE or text editor**
   - You can use Visual Studio Code, Code::Blocks, Dev-C++, Clion, or any other IDE text editor that supports C programming.
   - Open the project folder in your chosen editor to manage and edit files easily.

## Compilation
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

## Running the Application
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
   Example:
```bash
./client.exe 127.0.0.1 testfile.txt
```
The client will connect to the server, request the specified file, and save it locally with the same filename.

## Troubleshooting
1. **Connection refused (10061)**: Ensure the server is running and listening on the specified IP and PORT.
2. **Invalid IP and PORT**: Verify both the client and server are using the correct IP and PORT.
3. **Firewall or Antivirus**: Temporarily disable firewall or anitivirus software if they block connections.

## Notes
   - Ensure the server and client are on the same network, or adjust router and firewall settings to allow communication.
   - This application is designed for basic file transfers and currently supports single-client connections.

## Future Enhancements
1. **Multiple File Transfers:**
   - Support transferring multiple files at once or in a batch. This could involve sending an array of file names and then transferring each file one after another.
2. **Directory Transfers:**
   - Allow transferring entire directories, which could involve recursively sending all files and folders, along with metadata.
3. **Compression:**
   - To reduce transfer time, you could compress the files before sending and decompress them on the receiving side.
4. **Encryption:**
   - Implement encryption (like SSL/TLS) for secure file transfers, especially if you plan to send sensitive data.
5. **User interface**
6. **Error handling and other problems:**
   - file overwritting, special file types, file integrity check.

## License
This project is open-source and available under the MIT License.
