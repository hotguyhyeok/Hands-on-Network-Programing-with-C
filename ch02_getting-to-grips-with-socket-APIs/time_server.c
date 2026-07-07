// C 전처리문을 통해 플랫폼 종속없이 portable 하면서, Berkeley / Winsock 소켓들을 통용.

/* 핵심요약. 
 * 전처리문은 별로 중요하지 않음. 윈도우에서도 사용할 수 있도록 전처리지시문 작성한거임.
 * unix 에선 fd 로 소켓을 관리하기 때문에 man 2 read/write, close 등 모두 사용 가능하지만,
 * 윈도우에선 winsock 함수가 지원하는 함수, 구조체 등을 별도로 사용.
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
#define ISVALIDSOCKET(s) ((s) >= INVALID_SOCKET) // Unix 에서 유효한 소켓은 0 이상.
#define CLOSESOCKET(s) close(s) // 소켓닫기 (Unix 표준 함수)
#define GETSOCKETERRNO() (errno) // Unix 전역 오류 변수

#endif // (외부조건문 종료)

#include <stdio.h>
#include <string.h>
#include <time.h>
