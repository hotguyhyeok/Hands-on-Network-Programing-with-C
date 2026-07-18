#include "platform.h"
#include <unistd.h>

#if defined(_WIN32)
#include <conio.h>
#endif

int main(int argc, char **argv) {

// initialize Winsock
#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2,2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif


  if (argc < 3) {
    fprintf(stderr, "usage: %s hostname port\n", argv[0]);
    return 1;
  }


  // configuring a remote address for connection with command-line argments
  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM; // tcp
  struct addrinfo *peer_address;
  if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) { // 와일드카드가 아닌 서버의 주소와 포트를 명령줄인수로 제공. &peer_address(struct addrinfo) 에 자동으로 채워짐.
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  // for debugging
  printf("Remote address is: ");
  char address_buffer[BUFSIZ];
  char service_buffer[BUFSIZ];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
        address_buffer, sizeof(address_buffer), // returned hostname(e.g. example.com) or ip addr(e.g. 192.168.0.1)
        service_buffer, sizeof(service_buffer), // returned service name(e.g. http) or port number(e.g. 80)
        NI_NUMERICHOST);
  printf("%s %s\n", address_buffer, service_buffer);


  printf("Creating socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family,  // getaddrinfo()함수가 peer_address 구조체의 필드를 proper하게 채움
        peer_address->ai_socktype, peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  printf("Connecting...\n");
  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) { // connect() is similar to bind()
                                                                               // bind() associates a socket with a local address,
                                                                               // connect() associates a socket with a remote address and initiates the TCP connection.
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }
  freeaddrinfo(peer_address);


  printf("Connected.\n");
  printf("To send data, enter text followed by enter.\n");


  while(1) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET (socket_peer, &reads);
    // windows 는 소켓디스크립터만 지원하고, 일반 **파일디스크립터는 지원x.
    // non-windows systems 를 대상**으로만 stdin 을 fd_set 에 추가.
#if !defined(_WIN32) 
    FD_SET(STDIN_FILENO, &reads);
#endif
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; // 0.1 sec, windows 에선 select()가 stdin 을 감시할 수 없어서 polling을 설정. (select 는 fd_set 중 이벤트가 있을 때까지 블로킹됨.

    if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }


    if (FD_ISSET(socket_peer, &reads)) {
      char read[BUFSIZ];
      int bytes_received = recv(socket_peer, read, BUFSIZ, 0);
      if (bytes_received < 1) {
        printf("Connection closed by peer.\n");
        break;
      }
      printf("Received (%d bytes): %.*s", bytes_received, bytes_received, read); // `recv()` doesn't add null terminated. for this reason, we use the `%.*s printf()` format specifier, which prints a string of a specified length.
    }


#if defined(_WIN32)
    if(_kbhit()) { // this function to indicate whether any console input is waiting.
                   //
                   // if the user presses a non-printable key, such as arrow key, it still trigers `_kbhit()`, even though there is no character to read.
                   // and also, after the first key press, our program will block on `fgets()` until the user presses the 'Enter' key
#else
      if(FD_ISSET(STDIN_FILENO, &reads)) { // 0.1 초 간격으로 폴링을 수행하지만, 터미널 드라이버 레벨의 line discipline 때문에 0.1 초간격으로 전송하지 않고, Line Feed;LF (\n)을 기다리기 때문에 **FD_ISSET 은 사용자가 엔터키를 눌렀을 때, stdin 을 감지**
#endif
        char read[BUFSIZ];
        if (!fgets(read, BUFSIZ, stdin)) break;
        printf("Sending: %s", read);
        int bytes_sent = send(socket_peer, read, strlen(read), 0);
        printf("Sent %d bytes.\n", bytes_sent);
      }
  }


    printf("Closing socket...\n");
    CLOSESOCKET(socket_peer);

#if defined(_WIN32)
    WSACleanup();
#endif
    printf("Finished.\n");
    return 0;
}


// OUTPUT EXAMPLE
//
// ./tcp_client example.com http
// Configuring remote address...
// Remote address is: 104.20.23.154 http
// Creating socket...
// Connecting...
// Connected.
// To send data, enter text followed by enter.
// GET / HTTP/1.1
// Sending: GET / HTTP/1.1
// Sent 15 bytes.
// HOST: example.com
// Sending: HOST: example.com
// Sent 18 bytes.
// 
// Sending:
// Sent 1 bytes.
// Received (874 bytes): HTTP/1.1 200 OK
// Date: Sat, 18 Jul 2026 08:58:02 GMT
// Content-Type: text/html
// Transfer-Encoding: chunked
// Connection: keep-alive
// Server: cloudflare
// Last-Modified: Thu, 16 Jul 2026 20:05:48 GMT
// Allow: GET, HEAD
// Accept-Ranges: bytes
// Age: 10585
// cf-cache-status: HIT
// CF-RAY: a1d04820397fa0be-NRT
// 
// 22f
// <!doctype html><html lang="en"><head><title>Example Domain</title><link rel="icon" href="data:,"><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{background:#eee;width:60vw;margin:15vh auto;font-family:system-ui,sans-serif}h1{font-size:1.5em}div{opacity:0.8}a:link,a:visited{color:#348}</style></head><body><div><h1>Example Domain</h1><p>This domain is for use in documentation examples without needing permission. Avoid use in operations.</p><p><a href="https://iana.org/domains/example">Learn more</a></p></div></body></html>
// 
// 0
// 
// 
// Sending:
// Sent 1 bytes.
// Connection closed by peer.
// Closing socket...
// Finished.
