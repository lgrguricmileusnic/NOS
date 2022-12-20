/*
 * pipenos.c -- module implementation
 *
 * Example module which creates a virtual device driver.
 * Circular buffer (kfifo) is used to store received data (with write) and
 * reply with them on read operation.
 *
 * Copyright (C) 2021 Leonardo Jelenkovic
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 * No warranty is attached.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/log2.h>

#include "config.h"

/* Buffer size */
static int buffer_size = BUFFER_SIZE;
static int tcount_max = 20;

/* Parameter buffer_size can be given at module load time */
module_param(buffer_size, int, S_IRUGO);
MODULE_PARM_DESC(buffer_size, "Buffer size in bytes, must be a power of 2");
module_param(tcount_max, int, S_IRUGO);
MODULE_PARM_DESC(tcount_max, "Maximum number of threads");


MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE(LICENSE);

struct pipenos_dev *Pipenos = NULL;
struct buffer *Buffer = NULL;
static dev_t Dev_no = 0;

/* prototypes */
static struct buffer *buffer_create(size_t, int *);
static void buffer_delete(struct buffer *);
static struct pipenos_dev *pipenos_create(dev_t, struct file_operations *,
	struct buffer *, int *);
static void pipenos_delete(struct pipenos_dev *);
static void cleanup(void);
static void dump_buffer(struct buffer *);

static int pipenos_open(struct inode *, struct file *);
static int pipenos_release(struct inode *, struct file *);
static ssize_t pipenos_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t pipenos_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations pipenos_fops = {
	.owner =    THIS_MODULE,
	.open =     pipenos_open,
	.release =  pipenos_release,
	.read =     pipenos_read,
	.write =    pipenos_write
};

/* init module */
static int __init pipenos_module_init(void)
{
	int retval;
	struct buffer *buffer;
	struct pipenos_dev *pipenos;
	dev_t dev_no = 0;

	printk(KERN_NOTICE "Module 'pipenos' started initialization\n");

	/* get device number(s) */
	retval = alloc_chrdev_region(&dev_no, 0, 1, DRIVER_NAME);
	if (retval < 0) {
		printk(KERN_WARNING "%s: can't get major device number %d\n",
			DRIVER_NAME, MAJOR(dev_no));
		return retval;
	}

	/* create a buffer */
	/* buffer size must be a power of 2 */
	if (!is_power_of_2(buffer_size))
		buffer_size = roundup_pow_of_two(buffer_size);
	buffer = buffer_create(buffer_size, &retval);
	if (!buffer)
		goto no_driver;
	Buffer = buffer;

	/* create a device */
	pipenos = pipenos_create(dev_no, &pipenos_fops, buffer, &retval);
	if (!pipenos)
		goto no_driver;

	printk(KERN_NOTICE "Module 'pipenos' initialized with major=%d, minor=%d\n and buffer_size=%d, tcount_max=%d\n",
		MAJOR(dev_no), MINOR(dev_no), buffer_size, tcount_max);

	Pipenos = pipenos;
	Dev_no = dev_no;

	return 0;

no_driver:
	cleanup();

	return retval;
}

static void cleanup(void) {
	if (Pipenos)
		pipenos_delete(Pipenos);
	if (Buffer)
		buffer_delete(Buffer);
	if (Dev_no)
		unregister_chrdev_region(Dev_no, 1);
}

/* called when module exit */
static void __exit pipenos_module_exit(void)
{
	printk(KERN_NOTICE "Module 'pipenos' started exit operation\n");
	cleanup();
	printk(KERN_NOTICE "Module 'pipenos' finished exit operation\n");
}

module_init(pipenos_module_init);
module_exit(pipenos_module_exit);

/* Create and initialize a single buffer */
static struct buffer *buffer_create(size_t size, int *retval)
{
	struct buffer *buffer = kmalloc(sizeof(struct buffer) + size, GFP_KERNEL);
	if (!buffer) {
		*retval = -ENOMEM;
		printk(KERN_NOTICE "pipenos:kmalloc failed\n");
		return NULL;
	}
	*retval = kfifo_init(&buffer->fifo, buffer + 1, size);
	if (*retval) {
		kfree(buffer);
		printk(KERN_NOTICE "pipenos:kfifo_init failed\n");
		return NULL;
	}
	mutex_init(&buffer->lock);
	*retval = 0;

	return buffer;
}

static void buffer_delete(struct buffer *buffer)
{
	kfree(buffer);
}

/* Create and initialize a single pipenos_dev */
static struct pipenos_dev *pipenos_create(dev_t dev_no,
	struct file_operations *fops, struct buffer *buffer, int *retval)
{
	struct pipenos_dev *pipenos = kmalloc(sizeof(struct pipenos_dev), GFP_KERNEL);
	if (!pipenos){
		*retval = -ENOMEM;
		printk(KERN_NOTICE "pipenos:kmalloc failed\n");
		return NULL;
	}
	memset(pipenos, 0, sizeof(struct pipenos_dev));
	pipenos->buffer = buffer;
	pipenos->tcount = 0;
	init_waitqueue_head(&(pipenos->inq));
	init_waitqueue_head(&(pipenos->outq));

	cdev_init(&pipenos->cdev, fops);
	pipenos->cdev.owner = THIS_MODULE;
	pipenos->cdev.ops = fops;
	*retval = cdev_add (&pipenos->cdev, dev_no, 1);
	pipenos->dev_no = dev_no;
	if (*retval) {
		printk(KERN_NOTICE "Error (%d) when adding device pipenos\n",
			*retval);
		kfree(pipenos);
		pipenos = NULL;
	}

	return pipenos;
}
static void pipenos_delete(struct pipenos_dev *pipenos)
{
	cdev_del(&pipenos->cdev);
	kfree(pipenos);
}

/* Called when a process calls "open" on this device */
static int pipenos_open(struct inode *inode, struct file *filp)
{
	struct pipenos_dev *pipenos; /* device information */

	pipenos = container_of(inode->i_cdev, struct pipenos_dev, cdev);
	filp->private_data = pipenos; /* for other methods */
	
	if (pipenos->tcount >= tcount_max)
		return -1;
	
	pipenos->tcount++;

	if ( ((filp->f_flags & O_ACCMODE) != O_RDONLY) && ((filp->f_flags & O_ACCMODE) != O_WRONLY))
		return -EPERM;

	return 0;
}

/* Called when a process performs "close" operation */
static int pipenos_release(struct inode *inode, struct file *filp)
{
	struct pipenos_dev *pipenos = filp->private_data;
	pipenos->tcount--;
	return 0; /* nothing to do; could not set this function in fops */
}

/* Read count bytes from buffer to user space ubuf */
static ssize_t pipenos_read(struct file *filp, char __user *ubuf, size_t count,
	loff_t *f_pos /* ignoring f_pos */)
{
	ssize_t retval = 0;
	struct pipenos_dev *pipenos = filp->private_data;
	struct buffer *buffer = pipenos->buffer;
	struct kfifo *fifo = &buffer->fifo;
	unsigned int copied;

	if (mutex_lock_interruptible(&buffer->lock))
		return -ERESTARTSYS;

	dump_buffer(buffer);

	while (kfifo_len(fifo) == 0) { /* nothing to read */
		mutex_unlock(&buffer->lock);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		LOG("pipenos:read: waiting for something to read");
		if (wait_event_interruptible(pipenos->inq, (kfifo_len(fifo) > 0)))
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		/* otherwise loop, but first reacquire the lock */
	if (mutex_lock_interruptible(&buffer->lock))
		return -ERESTARTSYS;
	}
	if (kfifo_len(fifo) < count)
		count = kfifo_len(fifo);

	LOG("pipenos:read: reading %d elements", count);
	retval = kfifo_to_user(fifo, (char __user *) ubuf, count, &copied);

	if (retval)
	{
		klog(KERN_WARNING, "kfifo_from_user failed\n");
		mutex_unlock(&buffer->lock);
		return -EFAULT;
	}
	else
		retval = copied;

	mutex_unlock(&buffer->lock);

	/* finally, awake any writers and return */
	wake_up_interruptible(&pipenos->outq);
	LOG("pipenos:read: read %d elements", retval);
	return retval;
}


/* Write count bytes from user space ubuf to buffer */
static ssize_t pipenos_write(struct file *filp, const char __user *ubuf,
	size_t count, loff_t *f_pos /* ignoring f_pos */)
{
	ssize_t retval = 0;
	struct pipenos_dev *pipenos = filp->private_data;
	struct buffer *buffer = pipenos->buffer;
	struct kfifo *fifo = &buffer->fifo;
	unsigned int copied;

	if (count > BUFFER_SIZE)
		return -1;

	if (mutex_lock_interruptible(&buffer->lock))
		return -ERESTARTSYS;

	dump_buffer(buffer);
	LOG("pipenos:write: trying to write %d bytes", count);
	while (kfifo_avail(fifo) < count) { /* no space to write */
		mutex_unlock(&buffer->lock);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		LOG("pipenos:write: waiting for space to free");
		if (wait_event_interruptible(pipenos->outq, (kfifo_avail(fifo) >= count)))
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		/* otherwise loop, but first reacquire the lock */
		if (mutex_lock_interruptible(&buffer->lock))
			return -ERESTARTSYS;
	}
	LOG("pipenos:write: %d free bytes in kfifo", kfifo_avail(fifo));
	retval = kfifo_from_user(fifo, (char __user *) ubuf, count, &copied);
	if (retval)
	{
		mutex_unlock(&buffer->lock);
		printk(KERN_NOTICE "pipenos:write:kfifo_from_user failed");
		return -EFAULT;
	}
	else
		retval = copied;
	LOG("pipenos:write: wrote %d elements", retval);
	dump_buffer(buffer);

	mutex_unlock(&buffer->lock);
	wake_up_interruptible(&pipenos->inq);  /* blocked in read() and select() */


	return retval;
}


static void dump_buffer(struct buffer *b)
{
	char buf[BUFFER_SIZE];
	size_t copied;

	memset(buf, 0, BUFFER_SIZE);
	copied = kfifo_out_peek(&b->fifo, buf, BUFFER_SIZE);

	printk(KERN_NOTICE "pipenos:buffer:size=%u:contains=%u:buf=%s\n",
		kfifo_size(&b->fifo), kfifo_len(&b->fifo), buf);
}
