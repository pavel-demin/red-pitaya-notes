/*
command to compile:
gcc -O3 test-signal-continuity.c -o test-signal-continuity
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_ADDR "192.168.1.100"
#define TCP_PORT 1001

#define N 524288

int main()
{
  int i, sock;
  struct sockaddr_in addr;
  float *data, d, dmax;
  uint32_t command[3];

  data = malloc(16 * N);

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

  command[0] = 16;
  command[1] = (1 << 28) + 2000000;
  command[2] = (2 << 28) + 2000000;
  send(sock, command, sizeof(command), MSG_NOSIGNAL);

  while(1)
  {
    recv(sock, data, 16 * N, MSG_WAITALL);
    dmax = 0;
    for(i = 4; i < 4 * N; i += 4)
    {
      d = fabs(data[i] - data[i - 4]);
      if(dmax < d) dmax = d;
    }
    if(dmax > 5.0e-3) printf("%f\n", dmax);
  }

  return 0;
}
