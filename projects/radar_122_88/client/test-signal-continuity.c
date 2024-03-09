/*
command to compile:
gcc -O3 test-signal-continuity.c -lm -o test-signal-continuity
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

static inline void to24(int32_t *in, uint8_t *out)
{
  int32_t v = *in;
  out[2] = v >> 16;
  out[1] = v >> 8;
  out[0] = v;
}

static inline void to32(uint8_t *in, int32_t *out)
{
  int32_t v;
  v = in[2] << 16 | in[1] << 8 | in[0];
  if(v & 0x00800000) v |= 0xff000000;
  *out = v;
}

int main()
{
  int i, sock, size;
  struct sockaddr_in addr;
  uint8_t *data;
  int32_t d, dmax, v[4];
  uint32_t command[2];

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

  size = 4194304;

  for(i = 0; i < size; ++i)
  {
    v[0] = floor(5.79e6 * cos(2.0 * M_PI * i / size) + 0.5);
    v[1] = floor(5.79e6 * sin(2.0 * M_PI * i / size) + 0.5);
    v[2] = 0;
    v[3] = 0;
    to24(v + 0, data + i * 16 + 0);
    to24(v + 1, data + i * 16 + 3);
    to24(v + 2, data + i * 16 + 6);
    to24(v + 3, data + i * 16 + 9);
    data[i * 16 + 15] = (i >= 0 && i < 8) ? 1 : 0;
  }

  command[0] = 10000000;
  command[1] = (1 << 28) + (16 * size);
  send(sock, command, 8, MSG_NOSIGNAL);
  send(sock, data, 16 * size, MSG_NOSIGNAL);

  command[0] = (2 << 28);
  send(sock, command, 4, MSG_NOSIGNAL);

  while(1)
  {
    recv(sock, data, 16 * N, MSG_WAITALL);
    dmax = 0;
    for(i = 1; i < N; ++i)
    {
      to32(data + 12 + (i - 1) * 16, v);
      to32(data + 12 + i * 16, v + 1);
      d = abs(v[1] - v[0]);
      if(dmax < d && d != size - 1) dmax = d;
    }
    if(dmax > 1) printf("%d\n", dmax);
  }

  return 0;
}
