#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_PORT 1002

int sock_thread = -1;

volatile void *sts, *cfg;
volatile uint32_t *pha;

void *read_handler(void *arg);

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  volatile uint8_t *rst;
  struct sockaddr_in addr;
  int yes = 1;
  uint64_t command, data;
  uint8_t code, chan;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  pha = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);

  rst = cfg + 3;

  /* set sample rate */
  *(uint16_t *)(cfg + 6) = 4;

  /* reset timers and phas */
  *rst &= ~3;
  *rst |= 3;

  if((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, (void *)&yes , sizeof(yes));

  /* setup listening address */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(TCP_PORT);

  if(bind(sock_server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sock_server, 1024);

  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority = 99;
  pthread_attr_setschedparam(&attr, &param);

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
      code = (uint8_t)(command >> 60) & 0xf;
      chan = (uint8_t)(command >> 56) & 0xf;
      data = (uint64_t)(command & 0xffffffffffffffULL);

      if(code == 0)
      {
        /* reset pha */
        *rst &= ~1;
        *rst |= 1;
      }
      else if(code == 1)
      {
        /* reset timer */
        *rst &= ~2;
        *rst |= 2;
      }
      else if(code == 2)
      {
        /* set sample rate */
        if(data < 4)
        {
          *rst &= ~8;
          *(uint16_t *)(cfg + 6) = 4;
        }
        else
        {
          *rst |= 8;
          *(uint16_t *)(cfg + 6) = data;
        }
      }
      else if(code == 3)
      {
        /* set negator mode (0 for disabled, 1 for enabled) */
        if(chan == 0)
        {
          if(data == 0)
          {
            *rst &= ~16;
          }
          else if(data == 1)
          {
            *rst |= 16;
          }
        }
        else if(chan == 1)
        {
          if(data == 0)
          {
            *rst &= ~32;
          }
          else if(data == 1)
          {
            *rst |= 32;
          }
        }
      }
      else if(code == 4)
      {
        /* set pha delay */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 50) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 66) = data;
        }
      }
      else if(code == 5)
      {
        /* set pha min threshold */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 52) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 68) = data;
        }
      }
      else if(code == 6)
      {
        /* set pha max threshold */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 54) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 70) = data;
        }
      }
      else if(code == 7)
      {
        /* set timer */
        if(chan == 0)
        {
          *(uint64_t *)(cfg + 40) = data;
        }
        else if(chan == 1)
        {
          *(uint64_t *)(cfg + 56) = data;
        }
      }
      else if(code == 8)
      {
        /* start */
        sock_thread = sock_client;
        if(pthread_create(&thread, &attr, read_handler, NULL) < 0)
        {
          perror("pthread_create");
          return EXIT_FAILURE;
        }
        pthread_detach(thread);
        /* reset fifo */
        *rst &= ~64;
        *rst |= 64;
        /* start timer */
        *rst |= 4;
      }

    }
    /* stop timer */
    *rst &= ~4;

    sock_thread = -1;

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}

void *read_handler(void *arg)
{
  int i, size;
  uint32_t buffer[4096];

  while(1)
  {
    if(sock_thread < 0) break;

    size = *(uint16_t *)(sts + 40);

    if(size < 4)
    {
      usleep(1000);
      continue;
    }

    if(size > 4096) size = 4096;

    for(i = 0; i < size; ++i)
    {
      buffer[i] = *pha;
    }

    if(send(sock_thread, buffer, 4 * size, MSG_NOSIGNAL) < 0) break;
  }

  return NULL;
}
