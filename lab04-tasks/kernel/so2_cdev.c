/*
 * SO2 Lab - Linux device drivers (#4)
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <asm/ioctl.h>
#include <linux/types.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT
//#define MY_IOCTL_IN _IOC(_IOC_WRITE, 'k', 1, sizeof(my_ioctl_data))
#define MY_MAJOR		42
#define MY_MINOR		0
#define NUM_MINORS		1
#define MODULE_NAME		"so2_cdev"
#define MESSAGE			"hello\n"
#define IOCTL_MESSAGE		"Hello ioctl"

#define MIN(a, b)	(a > b ? b : a)
#define MAX(a, b)	(a > b ? a : b)
#ifndef BUFSIZ
#define BUFSIZ		4096
#endif

static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static int my_read(struct file *file, char __user *user_buffer,
	size_t size, loff_t *offset);
static int my_write(struct file *file, const char __user *user_buffer, 
	size_t size, loff_t * offset);
static long my_ioctl (struct file *file, unsigned int cmd, unsigned long arg);

struct my_device_data {
	struct cdev cdev;
 	atomic_t locker;

	/* my data starts here */
	char buffer[BUFSIZ];
	size_t size;
};

struct my_device_data devs[NUM_MINORS];

struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
    .unlocked_ioctl = my_ioctl
};

static int my_open(struct inode *inode, struct file *file)
{
	int r = atomic_cmpxchg(&devs[0].locker, 1, 0);
	if (r == 0) {		
		return -EBUSY;
	}

	struct my_device_data *my_data =
	    container_of(inode->i_cdev, struct my_device_data, cdev);

	/* validate access to device */
	file->private_data = my_data;

	/* initialize device */
	//..
	printk(LOG_LEVEL "open device\n");
	
	return 0;
}

static int my_read(struct file *file, char __user *user_buffer, 
                                size_t size, loff_t *offset)
{
	struct my_device_data *my_data =
	     (struct my_device_data *) file->private_data;

	size_t bufSize = MIN(size, BUFSIZ - *offset);

	/* read data from device in my_data->buffer */
	if(copy_to_user(user_buffer, my_data->buffer + *offset, bufSize))
		return -EFAULT;

	*offset += bufSize;
	
	return bufSize;
}

static int my_write(struct file *file, const char __user *user_buffer, 
                                             size_t size, loff_t * offset)
{
	// write data from user buffer into kernel buffer
	// update file offset in userspace
	// ..
	struct my_device_data *my_data =
	     (struct my_device_data *) file->private_data;

	printk(LOG_LEVEL "size :%lld, offset:%lld\n", size, *offset);

	size_t bufSize = MIN(BUFSIZ - *offset, size);

	if(copy_from_user(my_data->buffer + *offset, user_buffer, bufSize))
		return -EFAULT;

	*offset += bufSize;	
 
	return *offset;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
/*
    struct my_device_data *my_data =
         (struct my_device_data*) file->private_data;
    my_ioctl_data mid;
 
    switch(cmd) {
    case MY_IOCTL_IN:
        if( copy_from_user(&mid, (my_ioctl_data *) arg, 
                           sizeof(my_ioctl_data)) )
            oseturn -EFAULT; 
        * process data and execute command 
 
        break;
    default:
        return -ENOTTY;
    }
 */
    return 0;
}
 
static int my_release(struct inode *inode, struct file *file)
{
	printk(LOG_LEVEL "release device\n");
	devs[0].locker.counter = 1;

	return 0;
}


static int so2_cdev_init(void)
{
	int err, i;

	printk(LOG_LEVEL "Register driver\n");    	
	devs[0].locker.counter = 1;

	err = register_chrdev_region(
		MKDEV(MY_MAJOR, MY_MINOR), 
		NUM_MINORS,
		MODULE_NAME);

	if (err != 0) {
		printk(LOG_LEVEL "Cannot register device driver");
		return err;
	}


	for(i = 0; i < NUM_MINORS; i++) {
		/* initialize devs[i] fields */
		cdev_init(&devs[i].cdev, &my_fops);
		cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1);
		strcpy(devs[i].buffer, MESSAGE); 
		devs[i].size = strlen(devs[i].buffer);
	}	

	return 0;
}

static void so2_cdev_exit(void)
{
	int i;

	for(i = 0; i < NUM_MINORS; i++) {
		/* release devs[i] fields */
		cdev_del(&devs[i].cdev);
	}

	unregister_chrdev_region(MKDEV(MY_MAJOR, 0), NUM_MINORS);
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);
