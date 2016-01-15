#include <xil_io.h>

#define RAM_ADDR 0x1e000000

struct resource_table
{
  u32 ver;
  u32 num;
  u32 reserved[2];
  u32 offset[1];
} __packed;

enum fw_resource_type
{
  RSC_CARVEOUT = 0,
  RSC_DEVMEM = 1,
  RSC_TRACE = 2,
  RSC_VDEV = 3,
  RSC_MMU = 4,
  RSC_LAST = 5,
};

struct fw_rsc_carveout
{
  u32 type;
  u32 da;
  u32 pa;
  u32 len;
  u32 flags;
  u32 reserved;
  u8 name[32];
} __packed;

__attribute__ ((section (".rtable")))
const struct rproc_resource
{
  struct resource_table base;
  struct fw_rsc_carveout code_cout;
} ti_ipc_remoteproc_ResourceTable = {
  .base = { .ver = 1, .num = 1, .reserved = { 0, 0 },
    .offset = { offsetof(struct rproc_resource, code_cout) },
  },
  .code_cout = {
    .type = RSC_CARVEOUT, .da = RAM_ADDR, .pa = RAM_ADDR, .len = 1<<19,
    .flags = 0, .reserved = 0, .name = "APP_CPU1",
  },
};

int main()
{
  volatile int i;
  while(1)
  {
    Xil_Out32(0x40000000, 7);
    for(i = 0; i < 10000000; ++i);
    Xil_Out32(0x40000000, 0);
    for(i = 0; i < 10000000; ++i);
  }
  return 0;
}
