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

#define TCP_PORT 1001

volatile int32_t *gen;
volatile uint16_t *size;

uint32_t rate = 1000;
uint32_t dist = 0;
int64_t hist[4096];

int enable_thread = 0;
int active_thread = 0;

void *pulser_handler(void *arg);

static inline int lower_bound(int64_t *array, int size, int value)
{
  int i = 0, j = size, k;
  while(i < j)
  {
    k = i + (j - i) / 2;
    if(value > array[k]) i = k + 1;
    else j = k;
  }
  return i;
}

int main(int argc, char *argv[])
{
  int i, fd, sock_server, sock_client;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  volatile uint32_t *slcr, *axi_hp0;
  volatile void *sts, *cfg;
  void *hst[2], *ram, *buf;
  volatile uint8_t *rst[4];
  volatile uint32_t *trg;
  struct sockaddr_in addr;
  int yes = 1;
  uint32_t start, pre, tot;
  uint64_t command, data;
  uint8_t code, chan;
  uint16_t fall, rise, f, r, s;
  uint32_t spectrum[4096];
  int64_t value, total, y[3];
  int keep_pulsing = 0;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  slcr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8000000);
  axi_hp0 = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8008000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  trg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  hst[0] = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  hst[1] = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);
  gen = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40030000);
  ram = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);
  buf = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

  rst[0] = cfg + 0;
  rst[1] = cfg + 1;
  rst[2] = cfg + 2;
  rst[3] = cfg + 3;

  size = (uint16_t *)(sts + 54);

  /* set HP0 bus width to 64 bits */
  slcr[2] = 0xDF0D;
  slcr[144] = 0;
  axi_hp0[0] &= ~1;
  axi_hp0[5] &= ~1;

  /* set sample rate */
  *(uint16_t *)(cfg + 4) = 125;

  /* set trigger channel */
  trg[16] = 0;
  trg[0] = 2;

  /* reset timers and histograms */
  *rst[0] &= ~3;
  *rst[0] |= 3;
  *rst[1] &= ~3;
  *rst[1] |= 3;

  /* reset oscilloscope */
  *rst[2] &= ~3;
  *rst[2] |= 3;

  /* reset generator */
  *rst[3] &= ~128;
  *rst[3] |= 128;

  fall = 50;
  rise = 50;

  *(uint16_t *)(cfg + 84) = 6932;
  *(uint16_t *)(cfg + 86) = 65528;
  *(uint16_t *)(cfg + 88) = 58655;

  *(int16_t *)(cfg + 90) = -8192;
  *(int16_t *)(cfg + 92) = 8191;

  /* reset spectrum */
  memset(spectrum, 0, 16384);

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
      code = (uint8_t)(command >> 56) & 0xff;
      chan = (uint8_t)(command >> 52) & 0xf;
      data = (uint64_t)(command & 0xfffffffffffffULL);
      if(code == 0)
      {
        /* reset timer */
        if(chan == 0)
        {
          *rst[0] &= ~2;
          *rst[0] |= 2;
        }
        else if(chan == 1)
        {
          *rst[1] &= ~2;
          *rst[1] |= 2;
        }
      }
      else if(code == 1)
      {
        /* reset histogram*/
        if(chan == 0)
        {
          *rst[0] &= ~1;
          *rst[0] |= 1;
        }
        else if(chan == 1)
        {
          *rst[1] &= ~1;
          *rst[1] |= 1;
        }
      }
      else if(code == 2)
      {
        /* reset oscilloscope */
        *rst[2] &= ~3;
        *rst[2] |= 3;
      }
      else if(code == 3)
      {
        /* reset generator */
        *rst[3] &= ~128;
        *rst[3] |= 128;
      }
      else if(code == 4)
      {
        /* set sample rate */
        *(uint16_t *)(cfg + 4) = data;
      }
      else if(code == 5)
      {
        /* set negator mode (0 for disabled, 1 for enabled) */
        if(chan == 0)
        {
          if(data == 0)
          {
            *rst[0] &= ~16;
          }
          else if(data == 1)
          {
            *rst[0] |= 16;
          }
        }
        else if(chan == 1)
        {
          if(data == 0)
          {
            *rst[1] &= ~16;
          }
          else if(data == 1)
          {
            *rst[1] |= 16;
          }
        }
      }
      else if(code == 6)
      {
        /* set baseline mode (0 for none, 1 for auto) */
        if(chan == 0)
        {
          if(data == 0)
          {
            *rst[0] &= ~4;
          }
          else if(data == 1)
          {
            *rst[0] |= 4;
          }
        }
        else if(chan == 1)
        {
          if(data == 0)
          {
            *rst[1] &= ~4;
          }
          else if(data == 1)
          {
            *rst[1] |= 4;
          }
        }
      }
      else if(code == 7)
      {
        /* set baseline level */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 16) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 32) = data;
        }
      }
      else if(code == 8)
      {
        /* set pha delay */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 18) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 34) = data;
        }
      }
      else if(code == 9)
      {
        /* set pha min threshold */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 20) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 36) = data;
        }
      }
      else if(code == 10)
      {
        /* set pha max threshold */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 22) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 38) = data;
        }
      }
      else if(code == 11)
      {
        /* set timer */
        if(chan == 0)
        {
          *(uint64_t *)(cfg + 8) = data;
        }
        else if(chan == 1)
        {
          *(uint64_t *)(cfg + 24) = data;
        }
      }
      else if(code == 12)
      {
        /* set timer mode (0 for stop, 1 for running) */
        if(chan == 0)
        {
          if(data == 0)
          {
            *rst[0] &= ~8;
          }
          else if(data == 1)
          {
            *rst[0] |= 8;
          }
        }
        else if(chan == 1)
        {
          if(data == 0)
          {
            *rst[1] &= ~8;
          }
          else if(data == 1)
          {
            *rst[1] |= 8;
          }
        }
      }
      else if(code == 13)
      {
        /* read timer */
        if(chan == 0)
        {
          data = *(uint64_t *)(sts + 12);
          if(send(sock_client, &data, 8, MSG_NOSIGNAL) < 0) break;
        }
        else if(chan == 1)
        {
          data = *(uint64_t *)(sts + 20);
          if(send(sock_client, &data, 8, MSG_NOSIGNAL) < 0) break;
        }
      }
      else if(code == 14)
      {
        /* read histogram */
        if(chan == 0)
        {
          memcpy(buf, hst[0], 65536);
          if(send(sock_client, buf, 65536, MSG_NOSIGNAL) < 0) break;
        }
        else if(chan == 1)
        {
          memcpy(buf, hst[1], 65536);
          if(send(sock_client, buf, 65536, MSG_NOSIGNAL) < 0) break;
        }
      }
      else if(code == 15)
      {
        /* set trigger source (0 for channel 1, 1 for channel 2) */
        if(chan == 0)
        {
          trg[16] = 0;
          trg[0] = 2;
        }
        else if(chan == 1)
        {
          trg[16] = 1;
          trg[0] = 2;
        }
      }
      else if(code == 16)
      {
        /* set trigger slope (0 for rising, 1 for falling) */
        if(data == 0)
        {
          *rst[2] &= ~4;
        }
        else if(data == 1)
        {
          *rst[2] |= 4;
        }
      }
      else if(code == 17)
      {
        /* set trigger mode (0 for normal, 1 for auto) */
        if(data == 0)
        {
          *rst[2] &= ~8;
        }
        else if(data == 1)
        {
          *rst[2] |= 8;
        }
      }
      else if(code == 18)
      {
        /* set trigger level */
        *(uint16_t *)(cfg + 80) = data;
      }
      else if(code == 19)
      {
        /* set number of samples before trigger */
        *(uint32_t *)(cfg + 72) = data - 1;
      }
      else if(code == 20)
      {
        /* set total number of samples */
        *(uint32_t *)(cfg + 76) = data - 1;
      }
      else if(code == 21)
      {
        /* start oscilloscope */
        *rst[2] |= 16;
        *rst[2] &= ~16;
      }
      else if(code == 22)
      {
        /* read oscilloscope status */
        *(uint32_t *)buf = *(uint32_t *)(sts + 44) & 1;
        if(send(sock_client, buf, 4, MSG_NOSIGNAL) < 0) break;
      }
      else if(code == 23)
      {
        /* read oscilloscope data */
        pre = *(uint32_t *)(cfg + 72) + 1;
        tot = *(uint32_t *)(cfg + 76) + 1;
        start = *(uint32_t *)(sts + 44) >> 1;
        start = (start - pre) & 0x007FFFFF;
        if(start + tot <= 0x007FFFFF)
        {
          memcpy(buf, ram + start * 4, tot * 4);
          if(send(sock_client, buf, tot * 4, MSG_NOSIGNAL) < 0) break;
        }
        else
        {
          memcpy(buf, ram + start * 4, (0x007FFFFF - start) * 4);
          if(send(sock_client, buf, (0x007FFFFF - start) * 4, MSG_NOSIGNAL) < 0) break;
          memcpy(buf, ram, (start + tot - 0x007FFFFF) * 4);
          if(send(sock_client, buf, (start + tot - 0x007FFFFF) * 4, MSG_NOSIGNAL) < 0) break;
        }
      }
      else if(code == 24)
      {
        /* set fall time */
        if(data < 0 || data > 100) continue;
        fall = data;
      }
      else if(code == 25)
      {
        /* set rise time */
        if(data < 0 || data > 100) continue;
        rise = data;
      }
      else if(code == 26)
      {
        /* set lower limit */
        *(int16_t *)(cfg + 90) = data;
      }
      else if(code == 27)
      {
        /* set upper limit */
        *(int16_t *)(cfg + 92) = data;
      }
      else if(code == 28)
      {
        /* set rate */
        rate = data;
      }
      else if(code == 29)
      {
        /* set probability distribution */
        dist = data;
      }
      else if(code == 30)
      {
        /* reset spectrum */
        memset(spectrum, 0, 16384);
      }
      else if(code == 31)
      {
        /* set spectrum bin */
        spectrum[(data >> 32) & 0xfff] = data & 0xffffffff;
      }
      else if(code == 32)
      {
        keep_pulsing = data;
        /* stop pulser */
        enable_thread = 0;
        while(active_thread) usleep(1000);
        /* reset generator */
        *rst[3] &= ~128;
        *rst[3] |= 128;
        /* start pulser */
        total = 0;
        for(i = 0; i < 4096; ++i)
        {
          total += spectrum[i];
        }

        if(total < 1) continue;

        value = 0;
        for(i = 0; i < 4096; ++i)
        {
          value += spectrum[i];
          hist[i] = value * RAND_MAX / total - 1;
        }

        f = (uint16_t)floor(expf(-logf(2.0) / 125.0 / fall) * 65536.0 + 0.5);
        r = (uint16_t)floor(expf(-logf(2.0) / 125.0 / rise * 1.0e3) * 65536.0 + 0.5);

        y[0] = 4095 << 9;
        y[1] = 0;
        y[2] = 0;
        while(y[2] <= y[1])
        {
          y[2] = y[1];
          y[1] = y[0] + y[1] * r / 65536;
          y[0] = y[0] * f / 65536;
        }
        s = (uint16_t)(4095 * 65535 / (y[2] >> 9));

        *(uint16_t *)(cfg + 84) = s;
        *(uint16_t *)(cfg + 86) = f;
        *(uint16_t *)(cfg + 88) = r;

        enable_thread = 1;
        active_thread = 1;
        if(pthread_create(&thread, &attr, pulser_handler, NULL) < 0)
        {
          perror("pthread_create");
          return EXIT_FAILURE;
        }
        pthread_detach(thread);
      }
      else if(code == 33)
      {
        /* stop pulser */
        enable_thread = 0;
        while(active_thread) usleep(1000);
        /* reset generator */
        *rst[3] &= ~128;
        *rst[3] |= 128;
      }
    }

    close(sock_client);

    if(keep_pulsing) continue;

    /* stop pulser */
    enable_thread = 0;
    while(active_thread) usleep(1000);
    /* reset generator */
    *rst[3] &= ~128;
    *rst[3] |= 128;
  }

  close(sock_server);

  return EXIT_SUCCESS;
}

void *pulser_handler(void *arg)
{
  int32_t amplitude, interval;

  *gen = 0;
  *gen = 100000;

  while(enable_thread)
  {
    while(*size > 6000) usleep(1000);

    amplitude = lower_bound(hist, 4096, rand() % RAND_MAX);

    if(dist == 0)
    {
      interval = (int32_t)floor(125.0e6 / rate + 0.5);
    }
    else
    {
      interval = (int32_t)floor(-logf(1.0 - rand() / (RAND_MAX + 1.0)) * 125.0e6 / rate + 0.5);
    }

    *gen = amplitude;
    *gen = interval;
  }

  active_thread = 0;

  return NULL;
}
