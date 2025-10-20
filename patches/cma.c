#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/dma-map-ops.h>

#define CMA_ALLOC _IOWR('Z', 0, u32)

static unsigned long cma_size = 0;
static struct page *cma_page = NULL;
static struct page **cma_pages = NULL;

static void cma_free(void)
{
  if(cma_pages)
  {
    kfree(cma_pages);
    cma_pages = NULL;
  }

  if(cma_page)
  {
    dma_release_from_contiguous(NULL, cma_page, cma_size);
    cma_page = NULL;
  }
}

static long cma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  int i;
  long rc;
  u32 buffer;

  if(cmd != CMA_ALLOC) return -ENOTTY;

  rc = copy_from_user(&buffer, (void __user *)arg, sizeof(buffer));
  if(rc) return rc;

  cma_free();

  cma_size = PAGE_ALIGN(buffer) >> PAGE_SHIFT;

  cma_pages = kmalloc_array(cma_size, sizeof(struct page *), GFP_KERNEL);

  if(!cma_pages) return -ENOMEM;

  cma_page = dma_alloc_from_contiguous(NULL, cma_size, 0, false);

  if(!cma_page)
  {
    cma_free();
    return -ENOMEM;
  }

  for(i = 0; i < cma_size; ++i) cma_pages[i] = &cma_page[i];

  buffer = page_to_phys(cma_page);
  return copy_to_user((void __user *)arg, &buffer, sizeof(buffer));
}

static int cma_mmap(struct file *file, struct vm_area_struct *vma)
{
  if(!cma_pages) return -ENXIO;
  vm_flags_set(vma, VM_MIXEDMAP);
  return vm_map_pages(vma, cma_pages, cma_size);
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
  return misc_register(&cma_device);
}

static void __exit cma_exit(void)
{
  cma_free();
  misc_deregister(&cma_device);
}

module_init(cma_init);
module_exit(cma_exit);
MODULE_LICENSE("MIT");
