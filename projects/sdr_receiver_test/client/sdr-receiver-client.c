/*
command to compile:
gcc sdr-receiver-client.c -o sdr-receiver-client -lm -lws2_32
*/

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <math.h>
#include <stdio.h>
#include <stdint.h>

#define TCP_ADDR "192.168.1.4"
#define TCP_PORT 1001

int main(int argc, char**argv)
{
  SOCKET sock;
  struct sockaddr_in addr;
  char buffer[1024*1024];
  int32_t freq, corr, rate;
  int i;

  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);

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

  recv(sock, buffer, 1024*1024, MSG_WAITALL);
  recv(sock, buffer, 1024*1024, MSG_WAITALL);

  for(i = 0; i < 128*1024; ++i)
  {
    // printf("%d %d\n", *(int32_t *)(buffer + 8*i + 0), *(int32_t *)(buffer + 8*i + 4));
    printf("%d\n", *(int32_t *)(buffer + 8*i + 0));
  }
}

