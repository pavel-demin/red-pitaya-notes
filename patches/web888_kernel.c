#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define DEVICE_CNT    	1    
#define DEVICE_NAME     "web888_sdr"

#define NUM_GPIOS 5
#define BUFF_SIZE 10
#define GPIO_SWITCH_MODE 0
#define GPIO_EXT_CLOCK   1
#define GPIO_PE4312_CLK  2
#define GPIO_PE4312_DATA 3
#define GPIO_PE4312_LE   4

static char vbuf[BUFF_SIZE];

struct web888_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int gpio[NUM_GPIOS];
};

struct web888_dev web888_sdr; 

static int web888_init(struct device_node *nd)
{
    int ret;
    int i = 0;
    for(i = 0;i < NUM_GPIOS; i++)
    {
        web888_sdr.gpio[i] = of_get_named_gpio(nd, "web888-gpio", i);
        if(!gpio_is_valid(web888_sdr.gpio[i])) 
        {
            printk(KERN_ERR "WEB888 driver: failed to get gpio\n");
            return -EINVAL;
        }
        ret = gpio_request(web888_sdr.gpio[i], "web888_ps_gpio_pin");
        if (ret) {
            printk(KERN_ERR "WEB888 driver: failed to request gpio\n");
            return ret;
        }
        gpio_direction_output(web888_sdr.gpio[i],0);
    }

    for(i = 0;i< BUFF_SIZE;i++)
    {
        vbuf[i] = '0';
    }
    return 0;
}

static int sdr_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static void PE4312_driver(uint8_t att_val)
{
    uint8_t loop_cnt = 0;
    gpio_set_value(web888_sdr.gpio[GPIO_PE4312_CLK],0);
    gpio_set_value(web888_sdr.gpio[GPIO_PE4312_LE],0);
    for(loop_cnt = 0;loop_cnt < 6; loop_cnt ++ )
    {
        if((att_val>> (5 - loop_cnt)) & 0x01 == 0x01)
        {
            gpio_set_value(web888_sdr.gpio[GPIO_PE4312_DATA],1);
        }
        else
        {
            gpio_set_value(web888_sdr.gpio[GPIO_PE4312_DATA],0);
        }
        msleep(1);
        gpio_set_value(web888_sdr.gpio[GPIO_PE4312_CLK],1);
        msleep(1);
        gpio_set_value(web888_sdr.gpio[GPIO_PE4312_CLK],0);
    }
    msleep(1);
    gpio_set_value(web888_sdr.gpio[GPIO_PE4312_LE],1);
}

static ssize_t sdr_write(struct file *filp, const char __user *buf, size_t count, loff_t *offt)
{
    
    int ret;
    unsigned char att_data;
    unsigned char clk_sel;
    unsigned char mode_sel;
    unsigned long p = *offt;
    int tmp = count ;
    if (p > BUFF_SIZE)
        return 0;
    if (tmp > BUFF_SIZE - p)
        tmp = BUFF_SIZE - p;
    ret = copy_from_user(vbuf, buf, tmp);
    if(ret < 0) {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    att_data = (vbuf[0] - '0') < 0 ? 0 : vbuf[0] - '0';
    clk_sel  = (vbuf[1] - '0') < 0 ? 0 : vbuf[1] - '0';
    mode_sel = (vbuf[2] - '0') < 0 ? 0 : vbuf[2] - '0';

    gpio_set_value(web888_sdr.gpio[GPIO_SWITCH_MODE], (mode_sel & 0x01));
    gpio_set_value(web888_sdr.gpio[GPIO_EXT_CLOCK], (clk_sel & 0x01));
    PE4312_driver(att_data & 0x3f);
    
    *offt += tmp;
    return tmp;
}

static ssize_t sdr_read(struct file *filp, char __user * buf, size_t count, loff_t *offt)
{
    unsigned long p = *offt;
    int ret;
    int tmp = count ;
    if (p >= BUFF_SIZE)
        return 0;
    if (tmp > BUFF_SIZE - p)
        tmp = BUFF_SIZE - p;
    ret = copy_to_user(buf, vbuf+p, tmp);
    *offt +=tmp;
    return tmp;
}

static struct file_operations sdr_fops = {
    .open = sdr_open,
    .write = sdr_write,
    .read  = sdr_read,
};

static int driver_probe(struct platform_device *pdev)
{   
    int ret;
    printk("web888 driver and device was matched!\r\n");
    
    ret = web888_init(pdev->dev.of_node);
    if(ret < 0)
        return ret;
        
    ret = alloc_chrdev_region(&web888_sdr.devid, 0, DEVICE_CNT, DEVICE_NAME);
    if(ret < 0) {
        pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n", DEVICE_NAME, ret);
        goto free_gpio;
    }
    
    cdev_init(&web888_sdr.cdev, &sdr_fops);
    
    ret = cdev_add(&web888_sdr.cdev, web888_sdr.devid, DEVICE_CNT);
    if(ret < 0)
        goto del_unregister;
    
    web888_sdr.class = class_create(DEVICE_NAME);
    if (IS_ERR(web888_sdr.class)) {
        goto del_cdev;
    }
 
    web888_sdr.device = device_create(web888_sdr.class, NULL, web888_sdr.devid, NULL, DEVICE_NAME);
    if (IS_ERR(web888_sdr.device)) {
        goto destroy_class;
    }
    

    return 0;
destroy_class:
    class_destroy(web888_sdr.class);
del_cdev:
    cdev_del(&web888_sdr.cdev);
del_unregister:
    unregister_chrdev_region(web888_sdr.devid, DEVICE_CNT);
free_gpio:
    for (int i = 0; i < NUM_GPIOS; i++) {
        gpio_free(web888_sdr.gpio[i]);
    }
    return -EIO;
}

static int driver_remove(struct platform_device* pdev) {
    for (int i = 0; i < NUM_GPIOS; i++) {
        gpio_free(web888_sdr.gpio[i]);
    }
    cdev_del(&web888_sdr.cdev);
    unregister_chrdev_region(web888_sdr.devid, DEVICE_CNT);
    device_destroy(web888_sdr.class, web888_sdr.devid);
    class_destroy(web888_sdr.class);
    return 0;
}

static const struct of_device_id driver_of_match[] = {
    {
        .compatible = "web888,redpitaya",
    },
    {},
};

MODULE_DEVICE_TABLE(of, driver_of_match);

static struct platform_driver driver_driver = {
    .driver = {
        .name = DEVICE_NAME,
        .of_match_table = driver_of_match,
    },
    .probe = driver_probe,
    .remove = driver_remove,
};

module_platform_driver(driver_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JerryTech");
MODULE_DESCRIPTION("Driver for web888 to support redpitaya");


