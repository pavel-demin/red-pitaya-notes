#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>
#define INVSOC INVALID_SOCKET
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef SOCKET
#define SOCKET int
#define INVSOC (-1)
#endif
#endif

#if defined(__APPLE__) || defined(__MACH__)
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif
#endif

void usage()
{
  #if defined(_WIN32)
  fprintf(stderr, "Usage: mcpha-pulser-stop.exe addr\n");
  #else
  fprintf(stderr, "Usage: mcpha-pulser-stop addr\n");
  #endif
  fprintf(stderr, " addr - IP address of the Red Pitaya board.\n");
}

int main(int argc, char *argv[])
{
  SOCKET sock;
  struct sockaddr_in addr;
  fd_set writefds;
  struct timeval timeout;
  int result;
  uint64_t command;

  if(argc != 2)
  {
    usage();
    return EXIT_FAILURE;
  }

  #if defined(_WIN32)
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  #endif

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock == INVSOC)
  {
    fprintf(stderr, "** ERROR: could not create TCP socket\n");
    return EXIT_FAILURE;
  }

  #if defined(_WIN32)
  u_long mode = 1;
  ioctlsocket(sock, FIONBIO, &mode);
  #else
  int flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);
  #endif

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(argv[1]);
  addr.sin_port = htons(1001);

  connect(sock, (struct sockaddr *)&addr, sizeof(addr));

  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  FD_ZERO(&writefds);
  FD_SET(sock, &writefds);

  #if defined(_WIN32)
  result = select(0, NULL, &writefds, NULL, &timeout);
  #else
  result = select(sock + 1, NULL, &writefds, NULL, &timeout);
  #endif

  if(result <= 0)
  {
    fprintf(stderr, "** ERROR: could not connect to %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  command = (30ULL << 56);

  #if defined(_WIN32)
  int total = sizeof(command);
  int size;
  size = send(sock, (char *)&command, total, 0);
  #else
  ssize_t total = sizeof(command);
  ssize_t size;
  size = send(sock, &command, total, MSG_NOSIGNAL);
  #endif

  #if defined(_WIN32)
  closesocket(sock);
  #else
  close(sock);
  #endif

  if(size < total)
  {
    fprintf(stderr, "** ERROR: could not send command\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
