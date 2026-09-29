#ifndef SOCKET_COMPAT_H
#define SOCKET_COMPAT_H

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

typedef SOCKET socket_t;
typedef int socket_len_t;

#define INVALID_SOCK INVALID_SOCKET
#define socket_close(s) closesocket(s)

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef int socket_t;
typedef socklen_t socket_len_t;

#define INVALID_SOCK (-1)
#define socket_close(s) close(s)

#endif

#endif
