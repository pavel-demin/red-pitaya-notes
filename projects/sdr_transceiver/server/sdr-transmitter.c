#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main(int argc, char *argv[])
{
  int fd, sockServer, sockClient;
  int position, limit, offset;
  void *cfg, *sts, *ram;
  char *end, *name = "/dev/mem";
  char buf[4096];
  struct sockaddr_in addr;
  uint16_t port;
  int yes = 1;
  long number;

  errno = 0;
  number = (argc == 2) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 0 || number > 1)
  {
    printf("Usage: sdr-transmitter 0|1\n");
    return EXIT_FAILURE;
  }

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  switch(number)
  {
    case 0:
      port = 1002;
      cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
      sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
      ram = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40004000);
      break;
    case 1:
      port = 1004;
      cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40006000);
      sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40007000);
      ram = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x4000A000);
      break;
  }

  if((sockServer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, (void *)&yes , sizeof(yes));

  /* setup listening address */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if(bind(sockServer, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sockServer, 1024);

  while(!interrupted)
  {
    /* set PTT pin to low */
    *((uint32_t *)(cfg + 0)) = 0;

    memset(ram, 0, 8192);

    if((sockClient = accept(sockServer, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    /* set PTT pin to high */
    *((uint32_t *)(cfg + 0)) = 1;

    limit = 512;

    while(!interrupted)
    {
      /* read ram reader position */
      position = *((uint16_t *)(sts + 2));

      /* receive 4096 bytes if ready, otherwise sleep 0.5 ms */
      if((limit > 0 && position > limit) || (limit == 0 && position < 512))
      {
        offset = limit > 0 ? 0 : 4096;
        limit = limit > 0 ? 0 : 512;
        if(recv(sockClient, buf, 4096, MSG_WAITALL) <= 0) break;
        memcpy(ram + offset, buf, 4096);
      }
      else
      {
        usleep(500);
      }
    }

    signal(SIGINT, SIG_DFL);
    close(sockClient);
  }

  close(sockServer);

  return EXIT_SUCCESS;
}
