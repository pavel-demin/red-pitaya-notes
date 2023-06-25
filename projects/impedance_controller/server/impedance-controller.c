#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

volatile uint16_t *cntr;
volatile float *data;

int sock_thread = -1;

void *read_handler(void *arg);

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  volatile void *cfg, *sts;
  volatile uint32_t *freq;
  volatile uint16_t *rate;
  volatile int16_t *level;
  volatile uint8_t *rst, *gpio;
  struct sockaddr_in addr;
  uint32_t command;
  int32_t value;
  int yes = 1;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);

  cntr = (uint16_t *)(sts + 0);

  rst = (uint8_t *)(cfg + 0);
  gpio = (uint8_t *)(cfg + 1);
  rate = (uint16_t *)(cfg + 2);
  freq = (uint32_t *)(cfg + 4);
  level = (int16_t *)(cfg + 8);

  *rate = 4;
  *freq = (uint32_t)floor(100000 / 125.0e6 * (1<<30) + 0.5);

  *level = 0;

  *rst &= ~1;
  *gpio = 0;

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
  addr.sin_port = htons(1001);

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
      if(recv(sock_client, (char *)&command, 4, MSG_WAITALL) <= 0) break;
      value = command & 0xfffffff;
      value |= (value & 0x8000000) ? 0xf0000000 : 0;
      switch(command >> 28)
      {
        case 0:
          /* set sample rate */
          if(value < 4 || value > 8192) continue;
          *rate = value;
          break;
        case 1:
          /* set frequency */
          if(value < 0 || value > 62500000) continue;
          *freq = (uint32_t)floor(value / 125.0e6 * (1<<30) + 0.5);
          break;
        case 2:
          /* set level */
          if(value < -32766 || value > 32766) continue;
          *level = value;
          break;
        case 3:
          /* set gpio */
          if(value < 0 || value > 255) continue;
          *gpio = value;
          break;
        case 4:
          /* start */
          if(sock_thread >= 0) continue;
          /* reset */
          *rst &= ~1;
          sock_thread = sock_client;
          if(pthread_create(&thread, &attr, read_handler, NULL) < 0)
          {
            perror("pthread_create");
            return EXIT_FAILURE;
          }
          pthread_detach(thread);
          *rst |= 1;
          break;
        case 5:
          /* stop */
          sock_thread = -1;
          *rst &= ~1;
          break;
      }
    }

    sock_thread = -1;
    *rst &= ~1;
    *gpio = 0;
    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}

void *read_handler(void *arg)
{
  int i;
  float buffer[16];

  while(1)
  {
    if(sock_thread < 0) break;

    if(*cntr < 16)
    {
      usleep(1000);
      continue;
    }

    for(i = 0; i < 16; ++i)
    {
      buffer[i] = *data;
    }

    if(send(sock_thread, buffer, 64, MSG_NOSIGNAL) < 0) break;
  }

  return NULL;
}
