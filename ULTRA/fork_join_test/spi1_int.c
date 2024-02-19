/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  spi1_int.c
 *
 *  AUTHOR:     Mark McDermott
 *  CREATED:     May 12, 2015
 *
 *  DESCRIPTION: This kernel module registers interrupts from spi1 
 *               and does a call back to the application routine
 *               that is waiting on the interrupt. The module is
 *               inserted into the kernel during boot time.
 *
 *  DEPENDENCIES: none
 *
 *  PROJECT: H1KP
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/ioport.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MODULE_VER "1.0"

#define INTERRUPT 63    // CSPI-1
#define SPI1_MAJOR 241   // Setup during boot process 
#define MODULE_NM "spi1_int"

#undef DEBUG

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
       Define structures
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static struct proc_dir_entry *interrupt_spi1_file;

static struct fasync_struct *fasync_spi1_queue ;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: interrupt_spi1_int
 *
 * This function is the interrupt handler for interrupt 63
 *
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
 
int interruptcount = 0;

static irqreturn_t interrupt_spi1_int(int irq, void *dev_id, struct pt_regs *regs)
{
  interruptcount++;
  
#ifdef DEBUG
    printk(KERN_INFO "spi1_int: Interrupt detected in kernel \n");  
#endif
  
/* Signal the user application that an interupt occured */  

  kill_fasync(&fasync_spi1_queue, SIGIO, POLL_IN);

return IRQ_HANDLED;

}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: spi1_open
 *
 * This function is called when the spi1_int device is opened.
 * Used primarily for debug
 *
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
 
static int spi1_open (struct inode *inode, struct file *file) 
{
#ifdef DEBUG
    printk(KERN_INFO "spi1_int: Inside spi1_open \n");  
#endif
    return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: spi1_release
 *
 * This function is called when the spi1_int device is released
 *
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
 
static int spi1_release (struct inode *inode, struct file *file) 
{

#ifdef DEBUG
    printk(KERN_INFO "\nspi1_int: Inside spi1_release \n");  
#endif
    return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: spi1_fasync
 *
 * This is invoked by the kernel when the user program opens this
 * input device and issues fcntl(F_SETFL) on the associated file
 * descriptor. fasync_helper() ensures that if the driver issues a
 * kill_fasync(), a SIGIO is dispatched to the owning application.
 *
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int spi1_fasync (int fd, struct file *filp, int on)
{

#ifdef DEBUG
    printk(KERN_INFO "\nspi1_int: Inside spi1_fasync \n");  
#endif
    return fasync_helper(fd, filp, on, &fasync_spi1_queue);
} 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*  Define which file operations are supported
*
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-*/

struct file_operations spi1_fops = {
    .owner          =    THIS_MODULE,
    .llseek         =    NULL,
    .read           =    NULL,
    .write          =    NULL,
    .poll           =    NULL,
    .unlocked_ioctl =    NULL,
    .mmap           =    NULL,
    .open           =    spi1_open,
    .flush          =    NULL,
    .release        =    spi1_release,
    .fsync          =    NULL,
    .fasync         =    spi1_fasync,
    .lock           =    NULL,
    .read           =    NULL,
    .write          =    NULL,
};

static const struct file_operations proc_fops = {
    .owner          = THIS_MODULE,
    .open           = NULL,
    .read           = NULL,
};  

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: init_interrupt_spi1
 *
 * This function creates the /proc directory entry interrupt_spi1. It
 * also configures the parallel port then requests interrupt 63 from Linux.
 *
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
 
static int __init init_interrupt_spi1(void)
{

 int rv = 0;
 
  printk("H1KP SPI1 Interrupt Module\n");
  printk("H1KP SPI1 Driver Loading.\n");
  printk("Using Major Number %d on %s\n", SPI1_MAJOR, MODULE_NM); 
    
  if (register_chrdev(SPI1_MAJOR, MODULE_NM, &spi1_fops)) {
    printk("spi1_int: unable to get major %d. ABORTING!\n", SPI1_MAJOR);
    return -EBUSY;
    }

    interrupt_spi1_file = proc_create("interrupt_spi1", 0444, NULL, &proc_fops );
  
  if(interrupt_spi1_file == NULL) {
      printk("spi1_int: create /proc entry returned NULL. ABORTING!\n");
    return -ENOMEM;
  }


// Request interrupt 63 
 
  rv = request_irq(INTERRUPT, interrupt_spi1_int, IRQF_TRIGGER_RISING,
                   "interrupt_spi1", NULL);
  
  if ( rv ) {
    printk("Can't get interrupt %d\n", INTERRUPT);
    goto no_interrupt_spi1;
  }


/* Everything is initialized */

  printk(KERN_INFO "%s %s Initialized\n",MODULE_NM, MODULE_VER);
  return 0;

/* Remove the proc_entry on error */

  no_interrupt_spi1:
  remove_proc_entry("interrupt_spi1", NULL);
  return -EBUSY;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: cleanup_interrupt_spi1
 *
 * This function frees interrupt 240 then removes the /proc directory entry 
 * interrupt_spi1. 
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
 
static void __exit cleanup_interrupt_spi1(void)
{

/* free the interrupt */
  free_irq(INTERRUPT,NULL);
  
  unregister_chrdev(SPI1_MAJOR, MODULE_NM);

  remove_proc_entry("interrupt_spi1", NULL);
  printk(KERN_INFO "%s %s removed\n", MODULE_NM, MODULE_VER);
}

module_init(init_interrupt_spi1);
module_exit(cleanup_interrupt_spi1);

MODULE_AUTHOR("Mark McDermott");
MODULE_DESCRIPTION("H1KP SPI1 interrupt module");
MODULE_LICENSE("GPL");

