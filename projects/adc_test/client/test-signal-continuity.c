/*
command to compile:
gcc -O3 test-signal-continuity.c -o test-signal-continuity
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_ADDR "192.168.1.100"
#define TCP_PORT 1001

#define N 4194304

int main()
{
  int i, sock;
  struct sockaddr_in addr;
  int16_t *data, d, dmax;

  data = malloc(2 * N);

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return 1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(TCP_ADDR);
  addr.sin_port = htons(TCP_PORT);

  if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("connect");
    return 1;
  }

  while(1)
  {
    recv(sock, data, 2 * N, MSG_WAITALL);
    dmax = 0;
    for(i = 1; i < N; ++i)
    {
      d = abs(data[i] - data[i - 1]);
      if(dmax < d) dmax = d;
    }
    if(dmax > 10) printf("%d\n", dmax);
  }

  return 0;
}
