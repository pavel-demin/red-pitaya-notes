/*
command to compile:
gcc adc-test-client.c -o adc-test-client -lws2_32
*/

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define TCP_ADDR "192.168.1.100"
#define TCP_PORT 1001

int main(int argc, char**argv)
{
  SOCKET sock;
  struct sockaddr_in addr;
  char buffer[256*4096];

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

  while(1)
  {
    recv(sock, buffer, 256*4096, MSG_WAITALL);
  }
}

