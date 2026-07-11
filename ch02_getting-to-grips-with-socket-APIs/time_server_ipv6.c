// C 전처리문을 통해 플랫폼 종속없이 portable 하면서, Berkeley / Winsock 소켓들을 통용.

/* 핵심요약. 
 * 전처리문은 별로 중요하지 않음. 윈도우에서도 사용할 수 있도록 전처리지시문 작성한거임.
 * unix 에선 fd 로 소켓을 관리하기 때문에 man 2 read/write, close 등 모두 사용 가능하지만,
 * 윈도우에선 winsock2.h 에서 지원하는 함수, 구조체 등을 별도로 사용.
 */

#if defined(_WIN32) // (외부조건문 시작) 만약 윈도우 환경이면 실행. (WIN32 는 windows 컴파일러가 자동으로 정의)
                    
#ifndef _WIN32_WINNT // (내부조건문 시작)_WIN32_WINNT 가 아직 정의 안됐으면
#define _WIN32_WINNT 0x0600 // Vista 이상
#endif // (내부 조건문 종료)
       
#include <winsock2.h>
#include <ws2tcpip.h>

#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET) // 소켓이 유효한지 확인
#define CLOSESOCKET(s) closesocket(s) // 소켓닫기 (Windows 전용함수)
#define GETSOCKETERRNO() (WSAGetLastErro()) // 마지막 오류코드

#else // windows platform 이 아니면 실행
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>


// Unix 에서 소켓은 file descriptor 
#define ISVALIDSOCKET(s) ((s) >= 0) // Unix 에서 유효한 소켓은 0 이상.
#define CLOSESOCKET(s) close(s) // 소켓닫기 (Unix 표준 함수)
#define SOCKET int
#define GETSOCKETERRNO() (errno) // Unix 전역 오류 변수

#endif // (외부조건문 종료)

#include <stdio.h>
#include <string.h>
#include <time.h>

#define QUEUE_LIMIT 10

int main() {
#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2,2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif


  printf("Configuring local address...\n");
  struct addrinfo hints; // addrinfo은 socket 생성에 필요한 정보(주소패밀리,소켓타입,프로토콜,실제주소 등)를 감싸는 메타구조체.
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // this telling `getaddrinfo()` that we want it to bind to the **wildcard address**

  struct addrinfo *bind_address; // getaddrinfo()함수가 hints 조건에 부합하는 연결리스트 노드를 반환

  // It's common to see programs that don't use `getaddrinfo()` here. Instead, they fill in a `struct addrinfo` structure directly. **The advantage to using `getaddrinfo()` is that it is protocol-independent**
  getaddrinfo(0, "8080", &hints, &bind_address);


  printf("Creating socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  printf("Binding socket to local address...\n");
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
  

  printf("Waiting for connection...\n");
  struct sockaddr_storage client_address;
  socklen_t client_len = sizeof(client_address);
  SOCKET socket_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len); // 'accept()' will **block your program until a new connection is made**
  if (!ISVALIDSOCKET(socket_client)) {
    fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }


  printf("Client is connected... ");
  char address_buffer[100];
  // getnameinfo – socket address structure to hostname and service name.
  // i.e. int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);
  getnameinfo((struct sockaddr*) &client_address, client_len, address_buffer,
      sizeof(address_buffer), 0, 0, NI_NUMERICHOST); // `NI_NUMERICHOST` flag to view **host name as IP address**
  printf("%s\n", address_buffer);


  printf("Reading request...\n");
  char request[1024];
  int bytes_received = recv(socket_client, request, 1024, 0); // `recv()` will block until it has something
  printf("Received %d bytes.\n", bytes_received);

  // printf("%s", request); // it wll occur segmentation fault error OR print some garbage.
  // because, `recv()` does not add null characters.
  printf("%.*s", bytes_received, request); // null 문자 찾지 않고 bytes_received 만큼만 출력


  printf("Sending response...\n");
  const char *response = 
    "HTTP/1.1 200 OK\r\n" // \r\n 은 캐리지리턴+개행. HTTP 프로토콜 표준 줄바꿈.
    "Connection: close\r\n"
    "Content-Type: text/plain\r\n\r\n" // \r\n\r\n 2번은 HTTP에서 헤더 끝 본문 시작을 의미.
    "Local time is: ";
  int bytes_sent = send(socket_client, response, strlen(response), 0);
  printf("Sent %d bytes.\n", bytes_sent);


  time_t timer;
  time(&timer);
  char *time_msg = ctime(&timer);
  bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
  printf("Sent %d bytes.\n", bytes_sent);


  printf("Closing connection...\n");
  CLOSESOCKET(socket_client);


  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);


#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");


  return 0;
}

// OUTPUT EXAMPLE
//
// Configuring local address...
// Creating socket...
// Binding socket to local address...
// Listening...
// Waiting for connection...
// Client is connected... ::1
// Reading request...
// Received 472 bytes.
// GET / HTTP/1.1
// Host: [::1]:8080
// User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:152.0) Gecko/20100101 Firefox/152.0
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
// Accept-Language: en-US,en;q=0.9,ko-KR;q=0.8
// Accept-Encoding: gzip, deflate, br, zstd
// Sec-GPC: 1
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// Sec-Fetch-Dest: document
// Sec-Fetch-Mode: navigate
// Sec-Fetch-Site: none
// Sec-Fetch-User: ?1
// Priority: u=0, i
// 
// Sending response...
// Sent 79 bytes.
// Sent 25 bytes.
// Closing connection...
// Closing listening socket...
// Finished.
