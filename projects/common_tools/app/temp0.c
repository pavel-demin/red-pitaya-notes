#include <stdio.h>
#include <stdlib.h>

#define DIR "/sys/bus/iio/devices/iio:device0/"

float read_value(char *name)
{
  FILE *fp;
  char buffer[64];

  if((fp = fopen(name, "r")) == NULL)
  {
    fprintf(stderr, "Cannot open %s.\n", name);
    exit(EXIT_FAILURE);
  }

  fgets(buffer, sizeof(buffer), fp);
  fclose(fp);

  return atof(buffer);
}

int main()
{
  float off, raw, scl;

  off = read_value(DIR "in_temp0_offset");
  raw = read_value(DIR "in_temp0_raw");
  scl = read_value(DIR "in_temp0_scale");

  printf("%.1f\n", (off + raw) * scl / 1000);

  return EXIT_SUCCESS;
}
