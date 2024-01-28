#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

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

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

void usage()
{
  #if defined(_WIN32)
  fprintf(stderr, "Usage: pha-start.exe addr time rate pol1 pol2 min1 max1 min2 max2 file\n");
  #else
  fprintf(stderr, "Usage: pha-start addr time rate pol1 pol2 min1 max1 min2 max2 file\n");
  #endif
  fprintf(stderr, " addr - IP address of the Red Pitaya board,\n");
  fprintf(stderr, " time - duration of the acquisition in seconds (from 1 to 99999999),\n");
  fprintf(stderr, " rate - decimation rate (from 1 to 8192),\n");
  fprintf(stderr, " pol1 - IN1 pulse polarity (0 for positive, 1 for negative),\n");
  fprintf(stderr, " pol2 - IN2 pulse polarity (0 for positive, 1 for negative),\n");
  fprintf(stderr, " min1 - IN1 min threshold (from 0 to 16380),\n");
  fprintf(stderr, " max1 - IN1 max threshold (from 0 to 16380),\n");
  fprintf(stderr, " min2 - IN2 min threshold (from 0 to 16380),\n");
  fprintf(stderr, " max2 - IN2 max threshold (from 0 to 16380),\n");
  fprintf(stderr, " file - output file.\n");
}

int main(int argc, char *argv[])
{
  FILE *fp;
  SOCKET sock;
  struct sockaddr_in addr;
  fd_set readfds, writefds;
  struct timeval timeout;
  int result;
  char *end;
  long value;
  int32_t rate, pol1, pol2, min1, max1, min2, max2;
  time_t start, stop;
  uint64_t command[14];
  char buffer[2048];

  if(argc != 11)
  {
    usage();
    return EXIT_FAILURE;
  }

  errno = 0;
  value = strtol(argv[2], &end, 10);
  if(errno != 0 || end == argv[2] || value < 1 || value > 99999999)
  {
    usage();
    return EXIT_FAILURE;
  }
  stop = value;

  errno = 0;
  value = strtol(argv[3], &end, 10);
  if(errno != 0 || end == argv[3] || value < 1 || value > 8192)
  {
    usage();
    return EXIT_FAILURE;
  }
  rate = value;

  value = strtol(argv[4], &end, 10);
  if(errno != 0 || end == argv[4] || value < 0 || value > 1)
  {
    usage();
    return EXIT_FAILURE;
  }
  pol1 = value;

  value = strtol(argv[5], &end, 10);
  if(errno != 0 || end == argv[5] || value < 0 || value > 1)
  {
    usage();
    return EXIT_FAILURE;
  }
  pol2 = value;

  value = strtol(argv[6], &end, 10);
  if(errno != 0 || end == argv[6] || value < 0 || value > 16380)
  {
    usage();
    return EXIT_FAILURE;
  }
  min1 = value;

  value = strtol(argv[7], &end, 10);
  if(errno != 0 || end == argv[7] || value < 0 || value > 16380)
  {
    usage();
    return EXIT_FAILURE;
  }
  max1 = value;

  value = strtol(argv[8], &end, 10);
  if(errno != 0 || end == argv[8] || value < 0 || value > 16380)
  {
    usage();
    return EXIT_FAILURE;
  }
  min2 = value;

  value = strtol(argv[9], &end, 10);
  if(errno != 0 || end == argv[9] || value < 0 || value > 16380)
  {
    usage();
    return EXIT_FAILURE;
  }
  max2 = value;

  if((fp = fopen(argv[10], "wb")) == NULL)
  {
    fprintf(stderr, "** ERROR: could not open %s\n", argv[6]);
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
  addr.sin_port = htons(1002);

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

  command[0] = (0ULL << 60);
  command[1] = (1ULL << 60);
  command[2] = (2ULL << 60) + rate;
  command[3] = (3ULL << 60) + (0ULL << 56) + pol1;
  command[4] = (3ULL << 60) + (1ULL << 56) + pol2;
  command[5] = (4ULL << 60) + (0ULL << 56) + 100;
  command[6] = (4ULL << 60) + (1ULL << 56) + 100;
  command[7] = (5ULL << 60) + (0ULL << 56) + min1;
  command[8] = (5ULL << 60) + (1ULL << 56) + min2;
  command[9] = (6ULL << 60) + (0ULL << 56) + max1;
  command[10] = (6ULL << 60) + (1ULL << 56) + max2;
  command[11] = (7ULL << 60) + (0ULL << 56) + 125000000ULL * stop;
  command[12] = (7ULL << 60) + (1ULL << 56) + 125000000ULL * stop;
  command[13] = (8ULL << 60);

  #if defined(_WIN32)
  int total = sizeof(command);
  int size;
  size = send(sock, (char *)command, total, 0);
  #else
  ssize_t total = sizeof(command);
  ssize_t size;
  size = send(sock, command, total, MSG_NOSIGNAL);
  #endif

  if(size < total)
  {
    fprintf(stderr, "** ERROR: could not send command\n");
    return EXIT_FAILURE;
  }

  start = time(NULL) + 1;

  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  signal(SIGINT, signal_handler);

  while(!interrupted && time(NULL) - start < stop)
  {
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    #if defined(_WIN32)
    result = select(0, &readfds, NULL, NULL, &timeout);
    #else
    result = select(sock + 1, &readfds, NULL, NULL, &timeout);
    #endif

    if(result < 0) break;

    if(FD_ISSET(sock, &readfds))
    {
      size = recv(sock, buffer, 2048, 0);
      if(size <= 0) break;
      if(fwrite(buffer, 1, size, fp) < size) break;
    }
  }

  fclose(fp);

  #if defined(_WIN32)
  closesocket(sock);
  #else
  close(sock);
  #endif

  return EXIT_SUCCESS;
}
