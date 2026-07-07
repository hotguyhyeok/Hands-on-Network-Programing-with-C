/* time_console.c */

#include <stdio.h>
#include <time.h>

int main() {
  time_t timer;
  time(&timer);

  printf("Local time is: %s\n", ctime(&timer)); // typedef time_t 는 long
                                                // time()은 시스템 현재시간 time_t 반환
                                                // ctime()은 type_t를 문자열로 반환

  return 0;
}
