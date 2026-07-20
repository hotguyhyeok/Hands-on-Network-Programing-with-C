#include "platform.h"
#include <ctype.h>
#include <sys/socket.h>

#define QUEUE_LIMIT 10

int main() {
#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif

  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, "8080", &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
        bind_address->ai_protocol); // ai_protocol 은 domain(ai_family), type(ai_socktype)의 조합으로 결정되는 int 상수. IPPROTO_TCP, IPPROTO_UDP, IPPROTO_ICMP 등이 있음.
                                    // 이곳에 0으로 하드코딩해서 넘기기도 하는데, 이때, 0은 커널이 알아서 domain and type 조합으로 프로토콜을 선택함
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  printf("Bindind socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }
  freeaddrinfo(bind_address);

  printf("Listening...\n");
  if (listen(socket_listen, QUEUE_LIMIT) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  fd_set master;
  FD_ZERO(&master);
  FD_SET(socket_listen, &master);
  SOCKET max_socket = socket_listen; // for now, we add only listening socket to the set. Because it's the only socket, it must also be the largest.


  printf("Waiting for connections...\n");

  while(1) {
    fd_set reads;
    reads = master;
    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }


    SOCKET i;
    for (i=0; i <= max_socket; ++i) { // select()는 이벤트가 발생했는지 여부를 플래그로 표시할 뿐, 이벤트 발생지점을 찾는건 사용자 몫. 현재 코드는 O(n) 시간복잡도.
                                      // select()는 커널 내부에서도 전체를 스캔함.
                                      // 개선하기 위해선 epoll 을 사용(on unix-like)하거나 소켓들을 관리하는 별도의 배열(또는 구조체)을 선언할 수 있음.
      if (FD_ISSET(i, &reads)) {


        if (i == socket_listen) {
          struct sockaddr_storage client_address;
          socklen_t client_len = sizeof(client_address);
          SOCKET socket_client = accept(socket_listen, 
                (struct sockaddr*) &client_address, &client_len);
          if (!ISVALIDSOCKET(socket_client)) {
            fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
          }
          FD_SET(socket_client, &master);
          if (socket_client > max_socket) max_socket = socket_client;

          char address_buffer[BUFSIZ];
          getnameinfo((struct sockaddr*) &client_address, client_len, // 변환할 소켓 주소 구조체와 크기
              address_buffer, sizeof(address_buffer), // 호스트 이름을 저장할 버퍼와 크기(NULL, 0 로 생략 가능)
              0, 0, // 서비스(포트)를 저장할 버퍼와 크기(NULL, 0 로 생략 가능)
              NI_NUMERICHOST); // 숫자 IP문자열로 반환
          printf("New connection from %s\n", address_buffer);


      } else { // if the socket `i` isn't `socket_listen`, then it is instead a request for an established connection.
        char read[BUFSIZ];
        int bytes_received = recv(i, read, BUFSIZ, 0);
        if (bytes_received < 1) {
          FD_CLR(i, &master);
          CLOSESOCKET(i);
          continue;
        }

        for (int j=0; j < bytes_received; ++j)
          read[j] = toupper(read[j]);
        send(i, read, bytes_received, 0);
        }

      } // if FD_ISSET
    } // for i to max_socket
  } // while(1)


  // 아래는 실제 실행되지 않음. 저자는 추후라도 while문에 abort 기능추가를 대비해서 실행되지 않아도 코드에 포함시키는 것이 좋다고 함.
  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");

  return 0;
}


// SERVER'S OUTPUT EXAMPLE
//
// Configuring local address...
// Creating socket...
// Bindind socket to local address...
// Listening...
// Waiting for connections...
// New connection from 127.0.0.1
// New connection from 127.0.0.1
// ^C


// CLIENT'S OUTPUT EXAMPLE
//
// Configuring remote address...
// Remote address is: 127.0.0.1 http-alt
// Creating socket...
// Connecting...
// Connected.
// To send data, enter text followed by enter.
// Hello World!
// Sending: Hello World!
// Sent 13 bytes.
// Received (13 bytes): HELLO WORLD!
// Connection closed by peer.
// Closing socket...
// Finished.
