/*
 * Copyright (c) 2014, Mihai Cristea, REDANS SRL
 * email at: mihai _AT_ redans -DOT- eu
 * GPLv3 License applies
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include "hwcrypt.h"
 
/* Module information */
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
 
/* Device variables */
static struct class* hwcrypt_class = NULL;
static struct device* hwcrypt_device = NULL;
static int hwcrypt_major;

/* Flag used with the one_shot mode */
static bool message_read;
/* A mutex will ensure that only one process accesses our device */
static DEFINE_MUTEX(hwcrypt_device_mutex);

static unsigned char hwcrypt_msg[HWCRYPT_MSG_MAX];

/* Module parameters that can be provided on insmod */
static bool debug = false; /* print extra debug info */
module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "enable debug info (default: false)");
static bool one_shot = true; /* only read a single message after open() */
module_param(one_shot, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(one_shot, "disable the readout of multiple messages at once (default: true)");

static int hwcrypt_device_open(struct inode* inode, struct file* filp)
{

 /* Our sample device does not allow write access */
 if ( ((filp->f_flags & O_ACCMODE) == O_WRONLY)
   || ((filp->f_flags & O_ACCMODE) == O_RDWR) ) {
  info("write access is prohibited\n");
  return -EACCES;
 }
 
 /* Ensure that only one process has access to our device at any one time */
 if (!mutex_trylock(&hwcrypt_device_mutex)) {
  info("another process is accessing the device\n");
  return -EBUSY;
 }

 message_read = false;
 return 0;
}
 
static int hwcrypt_device_close(struct inode* inode, struct file* filp)
{
 mutex_unlock(&hwcrypt_device_mutex);
 return 0;
}
 
static ssize_t hwcrypt_device_read(struct file* filp, char __user *buffer, size_t length, loff_t* offset)
{
 
 /* The default from 'cat' is to issue multiple reads until the FIFO is depleted one_shot avoids that */
 if (one_shot && message_read) return 0;
  
 memcpy (buffer, hwcrypt_msg, HWCRYPT_MSG_MAX);
 printk("hwcrypt_device_read: %s\n", hwcrypt_msg);
 message_read = true;
 
 return 0;
}
 
/* The file_operation scructure tells the kernel which device operations are handled.
 * For a list of available file operations, see http://lwn.net/images/pdf/LDD3/ch03.pdf */
static struct file_operations fops = {
 .read = hwcrypt_device_read,
 .open = hwcrypt_device_open,
 .release = hwcrypt_device_close
};
 
static ssize_t sys_serialnum(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
 if (count > HWCRYPT_MSG_MAX) {
   printk("sys_serialnum: param exceed internal buffer %d > %d\n", count, HWCRYPT_MSG_MAX);
   return -1;
 }
 memcpy(hwcrypt_msg, buf, count);

 return count;
}
 
/* Declare the sysfs entries. The macros create instances of dev_attr_serialnumber */
static DEVICE_ATTR(serialnumber, S_IWUSR, NULL, sys_serialnum);
 
/* Module initialization and release */
static int __init hwcrypt_module_init(void)
{
 int retval;
 
 /* First, see if we can dynamically allocate a major for our device */
 hwcrypt_major = register_chrdev(0, DEVICE_NAME, &fops);
 if (hwcrypt_major < 0) {
  err("failed to register device: error %d\n", hwcrypt_major);
  retval = hwcrypt_major;
  goto failed_chrdevreg;
 }
 
 /* We can either tie our device to a bus (existing, or one that we create)
  * or use a "virtual" device class. For this example, we choose the latter */
 hwcrypt_class = class_create(THIS_MODULE, CLASS_NAME);
 if (IS_ERR(hwcrypt_class)) {
  err("failed to register device class '%s'\n", CLASS_NAME);
  retval = PTR_ERR(hwcrypt_class);
  goto failed_classreg;
 }
 
 /* With a class, the easiest way to instantiate a device is to call device_create() */
 hwcrypt_device = device_create(hwcrypt_class, NULL, MKDEV(hwcrypt_major, 0), NULL, CLASS_NAME "_" DEVICE_NAME);
 if (IS_ERR(hwcrypt_device)) {
  err("failed to create device '%s_%s'\n", CLASS_NAME, DEVICE_NAME);
  retval = PTR_ERR(hwcrypt_device);
  goto failed_devreg;
 }
 
 /* Now we can create the sysfs endpoints (don't care about errors).
  * dev_attr_serialnumber come from the DEVICE_ATTR(...) earlier */
 retval = device_create_file(hwcrypt_device, &dev_attr_serialnumber);
 if (retval < 0) {
  info("failed to create  /sys endpoint - continuing without\n");
 }
 
 mutex_init(&hwcrypt_device_mutex);
 strcpy(hwcrypt_msg, "INIT.VALUE");
 
 return 0;
 
failed_devreg:
 class_unregister(hwcrypt_class);
 class_destroy(hwcrypt_class);
failed_classreg:
 unregister_chrdev(hwcrypt_major, DEVICE_NAME);
failed_chrdevreg:
 return -1;
}
 
static void __exit hwcrypt_module_exit(void)
{
 device_remove_file(hwcrypt_device, &dev_attr_serialnumber);
 device_destroy(hwcrypt_class, MKDEV(hwcrypt_major, 0));
 class_unregister(hwcrypt_class);
 class_destroy(hwcrypt_class);
 unregister_chrdev(hwcrypt_major, DEVICE_NAME);
}
 
/* Let the kernel know the calls for module init and exit */
module_init(hwcrypt_module_init);
module_exit(hwcrypt_module_exit);
