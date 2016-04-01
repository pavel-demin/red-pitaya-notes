#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <sys/mman.h>
#include <libconfig.h>

int main(int argc, char *argv[])
{
  FILE *fp;
  int fd, offset, length, i, j;
  time_t t;
  struct tm *gmt;
  void *cfg, *sts, *mux;
  uint64_t *fifo[8];
  uint8_t *rst;
  uint16_t *cntr;
  int32_t type = 2;
  uint64_t buffer[8][45000];
  config_t config;
  config_setting_t *setting, *element;
  char date[12];
  char name[32];
  char zeros[15] = "000000_0000.c2";
  double dialfreq;
  double corr;
  double freq[8];
  int chan[8];

  if(argc != 2)
  {
    fprintf(stderr, "Usage: write-c2-files config_file.cfg\n");
    return EXIT_FAILURE;
  }

  config_init(&config);

  if(!config_read_file(&config, argv[1]))
  {
    fprintf(stderr, "Error on line %d in configuration file.\n", config_error_line(&config));
    return EXIT_FAILURE;
  }

  if(!config_lookup_float(&config, "corr", &corr))
  {
    fprintf(stderr, "No 'corr' setting in configuration file.\n");
    return EXIT_FAILURE;
  }

  if(corr < -100.0 || corr > 100.0)
  {
    fprintf(stderr, "Wrong 'corr' setting in configuration file.\n");
    return EXIT_FAILURE;
  }

  setting = config_lookup(&config, "bands");
  if(setting == NULL)
  {
    fprintf(stderr, "No 'bands' setting in configuration file.\n");
    return EXIT_FAILURE;
  }

  length = config_setting_length(setting);

  if(length > 8)
  {
    fprintf(stderr, "More than 8 bands in configuration file.\n");
    return EXIT_FAILURE;
  }

  if(length < 8)
  {
    fprintf(stderr, "Less than 8 bands in configuration file.\n");
    return EXIT_FAILURE;
  }

  for(i = 0; i < 8; ++i)
  {
    element = config_setting_get_elem(setting, i);

    if(!config_setting_lookup_float(element, "freq", &freq[i]))
    {
      fprintf(stderr, "No 'freq' setting in element %d.\n", i);
      return EXIT_FAILURE;
    }

    if(!config_setting_lookup_int(element, "chan", &chan[i]))
    {
      fprintf(stderr, "No 'chan' setting in element %d.\n", i);
      return EXIT_FAILURE;
    }

    if(chan[i] < 1 || chan[i] > 2)
    {
      fprintf(stderr, "Wrong 'chan' setting in element %d.\n", i);
      return EXIT_FAILURE;
    }
  }

  t = time(NULL);
  if((gmt = gmtime(&t)) == NULL)
  {
    perror("gmtime");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  mux = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);

  for(i = 0; i < 8; ++i)
  {
    fifo[i] = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40003000 + i * 0x1000);
    *(uint32_t *)(cfg + 4 + i * 4) = (uint32_t)floor((1.0 + 1.0e-6 * corr) * freq[i] / 125.0 * (1<<30) + 0.5);
  }

  for(i = 0; i < 8; ++i)
  {
    *(uint32_t *)(mux + 64 + i * 4) = i * 2 + chan[i] - 1;
  }

  *(uint32_t *)mux = 2;

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
    dialfreq = freq[i] - 0.0015;
    strftime(date, 12, "%y%m%d_%H%M", gmt);
    sprintf(name, "wspr_%d_%d_%s.c2", i, (uint32_t)(dialfreq * 1.0e6), date);
    if((fp = fopen(name, "wb")) == NULL)
    {
      perror("fopen");
      return EXIT_FAILURE;
    }
    fwrite(zeros, 1, 14, fp);
    fwrite(&type, 1, 4, fp);
    fwrite(&dialfreq, 1, 8, fp);
    fwrite(buffer[i], 1, 360000, fp);
    fclose(fp);
  }

  return EXIT_SUCCESS;
}
