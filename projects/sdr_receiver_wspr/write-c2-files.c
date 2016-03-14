#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>

int main()
{
  FILE *fp;
  int fd, offset, i, j;
  void *cfg, *sts;
  uint64_t *fifo[8];
  uint8_t *rst;
  uint16_t *cntr;
  int32_t type = 2;
  uint64_t buffer[8][45000];
  char name[15];
  double freq[8] = {
//    137500,
//    475100,
//    1838100,
    3594100,
    5288700,
    7040100,
    10140200,
    14097100,
    18106100,
    21096100,
    24926100,
//    28126100,
//    50294500,
  };

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
    *(uint32_t *)(cfg + 4 + i * 4) = (uint32_t)floor(freq[i] / 125.0e6 * (1<<30) + 0.5);
  }

  rst = (uint8_t *)(cfg + 0);
  cntr = (uint16_t *)(sts + 12);

  *rst |= 1;
  *rst &= ~1;

  offset = 0;

  while(offset < 45000)
  {
    while(*cntr < 500) sleep(1);

    for(i = 0; i < 500; ++i)
    {
      for(j = 0; j < 8; ++j)
      {
        buffer[j][offset + i] = *fifo[j];
      }
    }

    offset += 500;
  }

  for(i = 0; i < 8; ++i)
  {
    sprintf(name, "000000_000%d.c2", i);
    if((fp = fopen(name, "wb")) == NULL)
    {
      perror("fopen");
      return EXIT_FAILURE;
    }
    fwrite(name, 1, 14, fp);
    fwrite(&type, 1, 4, fp);
    fwrite(&freq[i], 1, 8, fp);
    fwrite(buffer[i], 1, 360000, fp);
    fclose(fp);
  }

  return EXIT_SUCCESS;
}
