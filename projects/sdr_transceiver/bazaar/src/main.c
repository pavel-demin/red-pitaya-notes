/*
command to compile:
gcc -shared -Wall -fPIC -Os -s main.c -o controllerhf.so
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define APP_NAME "/opt/redpitaya/www/apps/sdr_transceiver/sdr-transceiver"

typedef struct rp_app_params_s
{
  char *name;
  float value;
  int fpga_update;
  int read_only;
  float min_val;
  float max_val;
}
rp_app_params_t;

const char *rp_app_desc()
{
  return "Red Pitaya SDR transceiver.\n";
}

int rp_app_init()
{
  fprintf(stderr, "Starting SDR transceiver server.\n");
  if(fork() == 0) execl(APP_NAME, APP_NAME, "1", NULL);
  if(fork() == 0) execl(APP_NAME, APP_NAME, "2", NULL);
  return 0;
}

int rp_app_exit(void)
{
  fprintf(stderr, "Stopping SDR transceiver server.\n");
  system("killall sdr-transceiver");
  return 0;
}

int rp_set_params(rp_app_params_t *p, int len)
{
  return 0;
}

int rp_get_params(rp_app_params_t **p)
{
  return 0;
}

int rp_get_signals(float ***s, int *sig_num, int *sig_len)
{
  return 0;
}
