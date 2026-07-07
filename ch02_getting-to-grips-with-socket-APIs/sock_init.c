/* sock_init.c */

#if defined (_WIN32) // compiling on Windows

#ifndef _WIN32_WINNT // windows 버전을 나타내는 매크로(숫자상수), windows 헤더파일내부에서 다뤄짐.
#define _WIN32_WINNT 0x0600 // windows vista 이상.
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#else // compiling on other platforms
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#endif

#include <stdio.h>

int main() {
#if defined(_WIN32) 
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }

#endif // Bekeley sockets don't need initialize. 
  printf("Ready to use socket API.\n");

#if defined(_WIN32)
      WSACleanup();
#endif
      return 0;

}
