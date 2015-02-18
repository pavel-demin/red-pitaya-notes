#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
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
  int fd, i;
  int freq = 600000;
  int pos, limit, start;
  void *cfg, *sts, *ram;
  char *name = "/dev/mem";
  struct sockaddr_in addr;

  if((fd = open(name, O_RDWR)) < 1)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  ram = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);

  if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 1)
  {
    perror("socket");
    return 1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(BCAST_ADDR);
  addr.sin_port = htons(DATA_PORT);

  signal(SIGINT, signal_handler);

  *((uint32_t *)(cfg + 0)) &= ~15;

  *((uint32_t *)(cfg + 4)) = roundf(freq/125.0e6*(1<<22)-1);
  *((uint32_t *)(cfg + 8)) = 127;

  *((uint32_t *)(cfg + 0)) |= 15;

  limit = 128;
  while(!interrupted)
  {
    /* read ram writer position */
    pos = *((uint32_t *)(sts + 0));

    /* send 1024 bytes if ready, otherwise sleep 0.5 ms */
    if((limit > 0 && pos > limit) || (limit == 0 && pos < 384))
    {
      start = limit > 0 ? limit*8 - 1024 : 3072;
      sendto(fd, ram + start, 1024, 0, (struct sockaddr *) &addr, sizeof(addr));
      limit += 128;
      if(limit == 512) limit = 0;
    }
    else
    {
      usleep(500);
    }
  }

  *((uint32_t *)(cfg + 0)) &= ~15;

  munmap(ram, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));
  munmap(cfg, sysconf(_SC_PAGESIZE));

  return 0;
}
