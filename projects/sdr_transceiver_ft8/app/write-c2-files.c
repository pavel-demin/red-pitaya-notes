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
  volatile void *cfg, *sts;
  volatile uint64_t *fifo;
  volatile uint8_t *rst, *sel;
  volatile uint16_t *cntr;
  uint64_t *buffer;
  config_t config;
  config_setting_t *setting, *element;
  char date[12];
  char name[64];
  double dialfreq;
  double corr;
  double freq[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  int chan[8] = {1, 1, 1, 1, 1, 1, 1, 1};
  uint8_t value = 0;

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

  if(length < 1)
  {
    fprintf(stderr, "Less than 1 band in configuration file.\n");
    return EXIT_FAILURE;
  }

  for(i = 0; i < length; ++i)
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

    value |= (chan[i] - 1) << i;
  }

  t = time(NULL);
  if((gmt = gmtime(&t)) == NULL)
  {
    fprintf(stderr, "Cannot convert time.\n");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    fprintf(stderr, "Cannot open /dev/mem.\n");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  fifo = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);

  for(i = 0; i < 8; ++i)
  {
    *(uint32_t *)(cfg + 8 + i * 4) = (uint32_t)floor((1.0 + 1.0e-6 * corr) * freq[i] / 125.0 * (1<<30) + 0.5);
  }

  rst = (uint8_t *)(cfg + 0);
  sel = (uint8_t *)(cfg + 4);
  cntr = (uint16_t *)(sts + 0);

  *sel = value;

  *rst &= ~1;
  *rst |= 1;

  offset = 0;
  buffer = malloc(240000 * 8 * 8);
  memset(buffer, 0, 240000 * 8 * 8);

  while(offset < 236000)
  {
    while(*cntr < 500) usleep(10000);

    for(i = 0; i < 250; ++i)
    {
      for(j = 0; j < 8; ++j)
      {
        buffer[j * 240000 + offset + i] = *fifo;
      }
    }

    offset += 250;
  }

  for(i = 0; i < length; ++i)
  {
    dialfreq = freq[i] * 1.0e6;
    strftime(date, 12, "%y%m%d_%H%M", gmt);
    sprintf(name, "ft8_%d_%d_%d_%s.c2", i, (uint32_t)dialfreq, chan[i], date);
    if((fp = fopen(name, "wb")) == NULL)
    {
      fprintf(stderr, "Cannot open output file %s.\n", name);
      return EXIT_FAILURE;
    }
    fwrite(&dialfreq, 1, 8, fp);
    fwrite(&buffer[i * 240000], 1, 240000 * 8, fp);
    fclose(fp);
  }

  return EXIT_SUCCESS;
}
