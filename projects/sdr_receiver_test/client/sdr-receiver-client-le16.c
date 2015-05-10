/*
command to compile:
gcc sdr-receiver-client-le16.c -o sdr-receiver-client-le16 -lm
*/

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_ADDR "192.168.1.4"
#define TCP_PORT 1001

int main(int argc, char**argv)
{
  int sock;
  struct sockaddr_in addr;
  int32_t bufferIn[256];
  int16_t bufferOut[256];
  int32_t freq, corr, rate;
  int i;

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

  rate = 1;
  rate |= 1<<31;
  send(sock, (char *)&rate, 4, 0);

  freq = 5000000;
  corr = 25; // ppm
  freq = (int)floor(freq*(1.0 + corr*1.0e-6) + 0.5);
  send(sock, (char *)&freq, 4, 0);

  while(1)
  {
    recv(sock, bufferIn, 1024, MSG_WAITALL);
    for(i = 0; i < 256; ++i)
    {
      bufferOut[i] = (int16_t)(bufferIn[i] >> 10);
    }

    fwrite(bufferOut, 1, 512, stdout);
    fflush(stdout);
  }
}

