#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DATA_PORT 1002
#define BCAST_ADDR "239.0.0.39"

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main(int argc, char *argv[])
{
  int file, sock, i;
  int pos, limit, start;
  void *sts, *ram;
  char *name = "/dev/mem";
  struct sockaddr_in addr;

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 1)
  {
    perror("socket");
    return 1;
  }

  /* set up destination address */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(BCAST_ADDR);
  addr.sin_port = htons(DATA_PORT);

  if((file = open(name, O_RDWR)) < 1)
  {
    perror("open");
    return 1;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x40001000);
  ram = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x1E000000);

  signal(SIGINT, signal_handler);

  limit = 128;
  while(!interrupted)
  {
    /* read ram writer position */
    pos = *((uint32_t *)(sts + 0));

    /* send 1024 bytes if ready, otherwise sleep 0.5 ms */
    if((limit > 0 && pos > limit) || (limit == 0 && pos < 384))
    {
      start = limit > 0 ? limit*8 - 1024 : 3072;
      sendto(sock, ram + start, 1024, 0, (struct sockaddr *)&addr, sizeof(addr));
      limit += 128;
      if(limit == 512) limit = 0;
    }
    else
    {
      usleep(500);
    }
  }

  munmap(ram, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));

  return 0;
}
