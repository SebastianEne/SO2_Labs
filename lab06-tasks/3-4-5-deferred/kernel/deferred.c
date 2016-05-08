/*
 * SO2 - Lab 6 - Deferred Work
 *
 * Exercises #3, #4, #5: deferred work
 *
 * Code skeleton.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "../include/deferred.h"

#define LOG_LEVEL 		KERN_ALERT
#define MY_MAJOR		42
#define MY_MINOR		0
#define MODULE_NAME		"deferred"

#define TIMER_TYPE_NONE		-1
#define TIMER_TYPE_SET		0
#define TIMER_TYPE_ALLOC	1

MODULE_DESCRIPTION("Deferred work character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");


static struct my_device_data {
	struct cdev cdev;

	/* TODO 1: add timer here */
 	struct timer_list timer;
	
	/* TODO 2: add flag here; will be used with TIMER_TYPE_* values.  */
	int flag;

	/* TODO 3: add work_struct here */
	struct work_struct work;	
} dev;

static void alloc_io(void)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(5000);
	printk(LOG_LEVEL "Yawn! I've been sleeping for 5 seconds.\n");
}

/* TODO 3: define work handler */
static void my_work_handler(struct work_struct *work) 
{

}

static void timer_handler(unsigned long var)
{
	/* TODO 1: timer handler */
	printk(LOG_LEVEL "[timer_handler] callback timer\n");		

	/* TODO 2: take into account flag value (TIMER_TYPE_SET or TIMER_TYPE_ALLOC) */
	if (dev.flag == TIMER_TYPE_ALLOC)
		alloc_io();

	/* TODO 3: schedule work for TIMER_TYPE_ALLOC */
	schedule_work(&dev.work);
}

static int deferred_open(struct inode *inode, struct file *file)
{
	struct my_device_data *my_data =
		container_of(inode->i_cdev, struct my_device_data, cdev);
	file->private_data = my_data;
	printk(LOG_LEVEL "[deferred_open] Device opened\n");
	return 0;
}

static int deferred_release(struct inode *inode, struct file *file)
{
	printk(LOG_LEVEL "[deferred_release] Device released\n");
	return 0;
}

static long deferred_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct my_device_data *my_data = (struct my_device_data*) file->private_data;
	int interval;

	printk(LOG_LEVEL "[deferred_ioctl] Command: %s\n", ioctl_command_to_string(cmd));

	switch (cmd) {
		case MY_IOCTL_TIMER_SET:
			/* TODO 2: set flag to TIMER_TYPE_SET */
			my_data->flag = TIMER_TYPE_SET;

			/* TODO 1: schedule timer to print current's pid and comm */
			mod_timer(
				&my_data->timer, 
				jiffies + arg * HZ);
			break;
		
		case MY_IOCTL_TIMER_CANCEL:
			/* TODO 1: cancel timer */
			mod_timer(
				&my_data->timer,
				0);				
			break;

		case MY_IOCTL_TIMER_ALLOC:
			/* TODO 2: set flag to TIMER_TYPE_ALLOC */
			my_data->flag = TIMER_TYPE_ALLOC;

			/* TODO 2: schedule timer to call alloc_io */
			mod_timer(
				&my_data->timer, 
				jiffies + arg * HZ);
			break;

		default:
			return -ENOTTY;
	}
	return 0;
}

struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = deferred_open,
	.release = deferred_release,
	.unlocked_ioctl = deferred_ioctl
};

static int deferred_init(void)
{
	int err;

	printk(LOG_LEVEL "[deferred_init] Init module\n");
	err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1, MODULE_NAME);
	if (err) {
		printk(LOG_LEVEL "[deffered_init] register_chrdev_region: %d\n", err);
		return err;
	}

	/* TODO 1: initialize timer */
	setup_timer(
		&dev.timer,
                timer_handler,
                0);	

	/* TODO 1: schedule timer for the first time */
	mod_timer(
		&dev.timer, 
		jiffies + 2 * HZ);

	/* TODO 2: initialize flag */
	dev.flag = 0;	

	/* TODO 3: initialize work */
	INIT_WORK(&dev.work, my_work_handler);

	cdev_init(&dev.cdev, &my_fops);
	cdev_add(&dev.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);

	return 0;
}

static void deferred_exit(void)
{
	printk(LOG_LEVEL "[deferred_exit] Exit module\n" );

	cdev_del(&dev.cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);

	/* TODO 1: cleanup; make sure the timer is not running after we exit */
	del_timer_sync(&dev.timer);

	/* TODO 3:  make sure the work handler is not scheduled */
//	destroy_workqueue(&dev.work);	
}

module_init(deferred_init);
module_exit(deferred_exit);
