#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
  int fd, sock_server, sock_client;
  struct sockaddr_in sock_addr;
  int yes = 1;
  uint64_t command;
  uint32_t code, size, addr;
  void *buffer;
  volatile void *hub;

  buffer = malloc(16777216);

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  hub = mmap(NULL, 32768*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);

  if((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, (void *)&yes , sizeof(yes));

  /* setup listening address */
  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sock_addr.sin_port = htons(1001);

  if(bind(sock_server, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sock_server, 1024);

  while(1)
  {
    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    while(1)
    {
      if(recv(sock_client, &command, 8, MSG_WAITALL) <= 0) break;
      code = command >> 52 & 0xf;
      size = command >> 28 & 0xffffff;
      addr = command & 0xfffffff;
      switch(code)
      {
        case 0:
          memcpy(buffer, hub + addr, size);
          send(sock_client, buffer, size, MSG_NOSIGNAL);
          break;
        case 1:
          recv(sock_client, buffer, size, MSG_WAITALL);
          memcpy(hub + addr, buffer, size);
          break;
        case 2:
          recv(sock_client, buffer, size, MSG_WAITALL);
          fd = open("/dev/xdevcfg", O_WRONLY);
          write(fd, buffer, size);
          close(fd);
          break;
      }
    }

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}
