#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>

#define CMA_ALLOC _IOWR('Z', 0, u32)

static struct device *dma_device = NULL;
static size_t dma_size = 0;
static void *cpu_addr = NULL;
static dma_addr_t dma_addr;

static void cma_free(void)
{
  if(!cpu_addr) return;
  dma_free_coherent(dma_device, dma_size, cpu_addr, dma_addr);
  cpu_addr = NULL;
}

static long cma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  long rc;
  u32 buffer;

  if(cmd != CMA_ALLOC) return -ENOTTY;

  rc = copy_from_user(&buffer, (void __user *)arg, sizeof(buffer));
  if(rc) return rc;

  cma_free();

  dma_size = buffer;
  cpu_addr = dma_alloc_coherent(dma_device, dma_size, &dma_addr, GFP_KERNEL);

  if(IS_ERR_OR_NULL(cpu_addr))
  {
    rc = PTR_ERR(cpu_addr);
    if(rc == 0) rc = -ENOMEM;
    cpu_addr = NULL;
    return rc;
  }

  buffer = dma_addr;
  return copy_to_user((void __user *)arg, &buffer, sizeof(buffer));
}

static int cma_mmap(struct file *file, struct vm_area_struct *vma)
{
  return dma_mmap_coherent(dma_device, vma, cpu_addr, dma_addr, dma_size);
}

static int cma_release(struct inode *inode, struct file *file)
{
  cma_free();
  return 0;
}

static struct file_operations cma_fops =
{
  .unlocked_ioctl = cma_ioctl,
  .mmap = cma_mmap,
  .release = cma_release
};

struct miscdevice cma_device =
{
  .minor = MISC_DYNAMIC_MINOR,
  .name = "cma",
  .fops = &cma_fops
};

static int __init cma_init(void)
{
  int rc;

  rc = misc_register(&cma_device);
  if(rc) return rc;

  dma_device = cma_device.this_device;

  dma_device->dma_ops = &arm_coherent_dma_ops;
  dma_device->coherent_dma_mask = DMA_BIT_MASK(32);

  return 0;
}

static void __exit cma_exit(void)
{
  cma_free();
  misc_deregister(&cma_device);
}

module_init(cma_init);
module_exit(cma_exit);
MODULE_LICENSE("MIT");
