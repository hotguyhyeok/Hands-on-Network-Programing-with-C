#if defined(_WIN32) // on windows
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif 
#include <winsock2.h>
#include <ws2tcpip.h>

#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET) 
#define CLOSESOCKET(s) closesocket(s) 
#define GETSOCKETERRNO() (WSAGetLastErro()) 


#else // Unix-like platform
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define ISVALIDSOCKET(s) ((s) >= 0) 
#define CLOSESOCKET(s) close(s) 
#define SOCKET int
#define GETSOCKETERRNO() (errno) 
#endif 

#include <stdio.h>
#include <string.h>
