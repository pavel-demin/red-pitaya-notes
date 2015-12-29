/*
gcc laser-test-model.c -o laser-test-model -lm
*/

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

int main()
{
  int32_t j, k;
  int64_t i, a, b;
  __int128 integrator[7], comb[7], last_comb[7];

  for(i = 0; i < 7; ++i)
  {
    integrator[i] = 0;
    comb[i] = 0;
    last_comb[i] = 0;
  }

  for(i = 0; i < (int64_t)24416 * (int64_t)256 * (int64_t)1026; ++i)
  {
    if(i < (int64_t)24416 * (int64_t)256) integrator[0] = 0;
    else if(i < (int64_t)24416 * (int64_t)256 * (int64_t)1025)
    {
      a = i - (int64_t)24416 * (int64_t)256;
      a = a / 24416;
      b = a % 1024;
      integrator[0] = ((b < 512) ? b : 1023 - b) * 16;
    }
    else integrator[0] = 0;

    for(j = 1; j < 7; ++j)
    {
      integrator[j] += integrator[j-1];
    }
    if(i % 3052 == 1)
    {
      comb[0] = integrator[6];
      for(j = 1; j < 7; ++j)
      {
        comb[j] = comb[j-1] - last_comb[j-1];
      }

      for(j = 0; j < 7; ++j)
      {
        last_comb[j] = comb[j];
      }

      printf("%d\n", (int32_t)(comb[6] >> 68));
    }
  }

  return 0;
}
