#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <sys/mman.h>

int main()
{
  FILE *fp;
  int fd, offset, i, j;
  time_t t;
  struct tm *lt;
  void *cfg, *sts;
  uint64_t *fifo[8];
  uint8_t *rst;
  uint16_t *cntr;
  int32_t type = 2;
  uint64_t buffer[8][45000];
  char date[12];
  char name[32];
  char zeros[15] = "000000_0000.c2";
  double freq[8] = {
//     0.137500,
//     0.475100,
//     1.838100,
     3.594100,
     5.288700,
     7.040100,
    10.140200,
    14.097100,
    18.106100,
    21.096100,
    24.926100,
//    28.126100,
//    50.294500,
  };

  t = time(NULL);
  lt = localtime(&t);
  if((lt = localtime(&t)) == NULL)
  {
    perror("localtime");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);

  for(i = 0; i < 8; ++i)
  {
    fifo[i] = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000 + i * 0x1000);
    *(uint32_t *)(cfg + 4 + i * 4) = (uint32_t)floor(freq[i] / 125.0 * (1<<30) + 0.5);
  }

  rst = (uint8_t *)(cfg + 0);
  cntr = (uint16_t *)(sts + 12);

  *rst |= 1;
  *rst &= ~1;


  offset = 0;
  memset(buffer, 0, 45000 * 8 * 8);

  while(offset < 42000)
  {
    while(*cntr < 500) usleep(300000);

    for(i = 0; i < 250; ++i)
    {
      for(j = 0; j < 8; ++j)
      {
        buffer[j][offset + i] = *fifo[j];
      }
    }

    offset += 250;
  }

  for(i = 0; i < 8; ++i)
  {
    strftime(date, 12, "%y%m%d_%H%M", lt);
    sprintf(name, "wspr_%d_%s.c2", (uint32_t)(freq[i] * 1.0e6), date);
    if((fp = fopen(name, "wb")) == NULL)
    {
      perror("fopen");
      return EXIT_FAILURE;
    }
    fwrite(zeros, 1, 14, fp);
    fwrite(&type, 1, 4, fp);
    fwrite(&freq[i], 1, 8, fp);
    fwrite(buffer[i], 1, 360000, fp);
    fclose(fp);
  }

  return EXIT_SUCCESS;
}
