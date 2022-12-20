/*
 * config.h -- structures, constants, macros
 *
 * Copyright (C) 2021 Leonardo Jelenkovic
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 * No warranty is attached.
 *
 */

#pragma once

#define DRIVER_NAME 	"pipenos"

#define AUTHOR		"Lovro Grguric Mileusnic"
#define LICENSE		"Dual BSD/GPL"

#define BUFFER_SIZE	64

/* Circular buffer */
struct buffer {
	struct kfifo fifo;
	struct mutex lock;	/* prevent parallel access */
};

/* Device driver */
struct pipenos_dev {
	wait_queue_head_t inq, outq;
	dev_t dev_no;		/* device number */
	struct cdev cdev;	/* Char device structure */
	struct semaphore sem;   /* mutual exclusion semaphore */
	struct buffer *buffer;	/* Pointer to buffer */
};

#define klog(LEVEL, format, ...)	\
printk ( LEVEL "[shofer] %d: " format "\n", __LINE__, ##__VA_ARGS__)

//printk ( LEVEL "[shofer]%s:%d]" format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
//printk ( LEVEL "[shofer]%s:%d]" format "\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__)

//#define SHOFER_DEBUG

#ifdef SHOFER_DEBUG
#define LOG(format, ...)	klog(KERN_DEBUG, format,  ##__VA_ARGS__)
#else /* !SHOFER_DEBUG */
#warning Debug not activated
#define LOG(format, ...)
#endif /* SHOFER_DEBUG */