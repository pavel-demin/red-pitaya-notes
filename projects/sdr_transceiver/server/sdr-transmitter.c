#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void *cfg, *sts, *ram;
int sockThread[2] = {0, 0};

void *ctrl_handler(void *arg);
void *data_handler(void *arg);

int main(int argc, char *argv[])
{
  int fd, sockServer, sockClient;
  pthread_t thread;
  void *(*handler[2])(void *) = {ctrl_handler, data_handler};
  char *end, *name = "/dev/mem";
  struct sockaddr_in addr;
  uint16_t port;
  uint32_t command;
  int yes = 1;
  long number;

  errno = 0;
  number = (argc == 2) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 0 || number > 1)
  {
    printf("Usage: sdr-controller 0|1\n");
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

  /* set PTT pin to low */
  *((uint32_t *)(cfg + 0)) = 0;
  /* set default tx phase increment */
  *((uint32_t *)(cfg + 12)) = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default tx sample rate */
  *((uint32_t *)(cfg + 16)) = 625;

  memset(ram, 0, 8192);

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

  while(1)
  {
    if((sockClient = accept(sockServer, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }
    if(recv(sockClient, (char *)&command, 4, MSG_WAITALL) < 0) continue;
    if(command > 1 || sockThread[command])
    {
      close(sockClient);
      continue;
    }

    sockThread[command] = sockClient;

    if(pthread_create(&thread, NULL, handler[command], NULL) < 0)
    {
      perror("pthread_create");
      return EXIT_FAILURE;
    }
    pthread_detach(thread);
  }

  close(sockServer);

  return EXIT_SUCCESS;
}

void *ctrl_handler(void *arg)
{
  int sockClient = sockThread[0];
  uint32_t command, freq;
  uint32_t freqMin = 50000;
  uint32_t freqMax = 60000000;

  /* set PTT pin to low */
  *((uint32_t *)(cfg + 0)) = 0;
  /* set default tx phase increment */
  *((uint32_t *)(cfg + 12)) = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default tx sample rate */
  *((uint32_t *)(cfg + 16)) = 625;

  memset(ram, 0, 8192);

  while(1)
  {
    if(recv(sockClient, (char *)&command, 4, MSG_WAITALL) < 0) break;
    switch(command >> 28)
    {
      case 0:
        /* set tx phase increment */
        freq = command & 0xfffffff;
        if(freq < freqMin || freq > freqMax) continue;
        *((uint32_t *)(cfg + 12)) = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
        break;
      case 1:
        /* set tx sample rate */
        switch(command & 7)
        {
          case 0:
            freqMin = 10000;
            *((uint32_t *)(cfg + 16)) = 3125;
            break;
          case 1:
            freqMin = 25000;
            *((uint32_t *)(cfg + 16)) = 1250;
            break;
          case 2:
            freqMin = 50000;
            *((uint32_t *)(cfg + 16)) = 625;
            break;
          case 3:
            freqMin = 125000;
            *((uint32_t *)(cfg + 16)) = 250;
            break;
          case 4:
            freqMin = 250000;
            *((uint32_t *)(cfg + 16)) = 125;
            break;
        }
        break;
      case 2:
        /* set PTT pin to high */
        *((uint32_t *)(cfg + 0)) = 1;
        break;
      case 3:
        /* set PTT pin to low */
        *((uint32_t *)(cfg + 0)) = 0;
        break;
    }
  }

  /* set PTT pin to low */
  *((uint32_t *)(cfg + 0)) = 0;
  /* set default tx phase increment */
  *((uint32_t *)(cfg + 12)) = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default tx sample rate */
  *((uint32_t *)(cfg + 16)) = 625;

  memset(ram, 0, 8192);

  close(sockClient);
  sockThread[0] = 0;

  return NULL;
}

void *data_handler(void *arg)
{
  int sockClient = sockThread[1];
  int position, limit, offset;
  int size = 0;
  char buf[4096];

  limit = 512;

  while(1)
  {
    /* read ram reader position */
    position = *((uint16_t *)(sts + 2));

    /* receive 4096 bytes if ready, otherwise sleep 0.5 ms */
    if((limit > 0 && position > limit) || (limit == 0 && position < 512))
    {
      offset = limit > 0 ? 0 : 4096;
      limit = limit > 0 ? 0 : 512;
      if(ioctl(sockClient, FIONREAD, &size) < 0) break;
      if(size >= 4096)
      {
        if(recv(sockClient, buf, 4096, MSG_WAITALL) <= 0) break;
        if(*((uint32_t *)(cfg + 0)))
        {
          memcpy(ram + offset, buf, 4096);
        }
        else
        {
          memset(ram + offset, 0, 4096);
        }
      }
      else
      {
        memset(ram + offset, 0, 4096);
      }
    }
    else
    {
      usleep(500);
    }
  }

  close(sockClient);
  sockThread[1] = 0;

  return NULL;
}
