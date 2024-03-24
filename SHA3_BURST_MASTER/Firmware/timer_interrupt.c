/* ===================================================================
 *  timer_interrupt.c
 *
 *  AUTHOR:     Mark McDermott
 *
 *  CREATED:    Aug 13, 2009     For TLL 6000 Board
 *  UPDATED:    May 19, 2019     Updated for ZED Board
 *  UPDATED:    Feb 6,  2021     For the Ultra96 board
 *
 *  DESCRIPTION: This kernel module registers interrupts from the DMA
 *               unit and measures the time between them. This is
 *               used to also measure the latency through the kernel
 *               to respond to interrupts. 
 *               
 *  DEPENDENCIES: Works only on Avnet Ultra96 board
 *
 */
 

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
#include <linux/io.h>
#include <linux/module.h>

#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/gpio/driver.h>

#include <linux/pm_runtime.h>
#include <linux/of.h>

#define MODULE_VER "1.0"


#define CAPTURE_TIMER_MAJOR 235                  // Need to mknod /dev/dma_int c 235 0
#define MODULE_NM "capture_timer_interrupt"

// #undef DEBUG
#define DEBUG
// #undef DEBUG1
#define DEBUG1


int             interruptcount  = 0;
int             temp            = 0;
int             len             = 0;
char            *msg            = NULL;

unsigned int    gic_interrupt;              // Interrupt number

static struct fasync_struct *fasync_capture_timer_queue ;

/* ===================================================================
 * function: dma_int_handler
 */
 
//static irqreturn_t dma_int_handler(int irq, void *dev_id, struct pt_regs *regs)

irq_handler_t capture_timer_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
  interruptcount++;
  
    #ifdef DEBUG1
  printk(KERN_INFO
	     "capture_timer_int: Interrupt detected in kernel \n");  // DEBUG
#endif
  
    /* Signal the user application that an interupt occured */

  kill_fasync(&fasync_capture_timer_queue, SIGIO, POLL_IN);

  return  (irq_handler_t) IRQ_HANDLED;

}


static struct proc_dir_entry *proc_capture_timer_int;

/* ===================================================================
*    function: read_proc -- Example code  --- Not used
*/

ssize_t read_proc(struct file *filp,char *buf,size_t count,loff_t *offp ) 
    {
    printk("Interrupt count value = %d\n", interruptcount);    
    return 0;
}


/* ===================================================================
*    function: write_proc   --- Example code   --- Not used
*/

ssize_t write_proc(struct file *filp,const char *buf,size_t count,loff_t *offp)
    {
    printk("write_proc count \n");
    interruptcount = 0x0;

return count;
}

/* ===================================================================
 * function: capture_timer_open     ---- Example code
 *
 * This function is called when the capture_timer_int device is opened
 *
 */
 
static int capture_timer_open (struct inode *inode, struct file *file) {

    #ifdef DEBUG1
        printk(KERN_INFO "capture_timer_int: Inside capture_timer_open \n");  // DEBUG
    #endif
    return 0;
}

/* ===================================================================
 * function: capture_timer_release   ---- Example code
 *
 * This function is called when the capture_timer_int device is
 * released
 *
 */
 
static int capture_timer_release (struct inode *inode, struct file *file) {
    #ifdef DEBUG1
        printk(KERN_INFO "\ncapture_timer_int: Inside capture_timer_release \n");  // DEBUG
    #endif
    return 0;
}

/* ===================================================================
 * function: capture_timer_fasync
 *
 * This is invoked by the kernel when the user program opens this
 * input device and issues fcntl(F_SETFL) on the associated file
 * descriptor. fasync_helper() ensures that if the driver issues a
 * kill_fasync(), a SIGIO is dispatched to the owning application.
 */

static int capture_timer_fasync (int fd, struct file *filp, int on)
{
    #ifdef DEBUG
    printk(KERN_INFO "\ncapture_timer_int: Inside capture_timer_fasync \n");  // DEBUG
    #endif
    
    return fasync_helper(fd, filp, on, &fasync_capture_timer_queue);
}; 

/* ===================================================================
*
*  Define which file operations are supported
*
*/

struct file_operations capture_timer_fops = {
    .owner          =    THIS_MODULE,
    .llseek         =    NULL,
    .read           =    NULL,
    .write          =    NULL,
    .poll           =    NULL,
    .unlocked_ioctl =    NULL,
    .mmap           =    NULL,
    .open           =    capture_timer_open,
    .flush          =    NULL,
    .release        =    capture_timer_release,
    .fsync          =    NULL,
    .fasync         =    capture_timer_fasync,
    .lock           =    NULL,
    .read           =    NULL,
    .write          =    NULL,
};

static const struct proc_ops proc_fops = {
    // .proc_read = read_proc,
    // .proc_write = write_proc,
};

/* =================== This struct is critical   =====================
    
   Make sure that the 'compatible' value matches what is in the DTS file:
   
         dma@40000000 {
			#dma-cells = <0x1>;
			clock-names = "s_axi_lite_aclk", "m_axi_aclk";
			clocks = <0x1 0xf 0x1 0xf>;
			compatible = "xlnx,axi-cdma";
			:
			:
			:
		};
 */

static const struct of_device_id zynq_capture_timer_of_match[] = {
    {.compatible = "xlnx,Capture-Timer-1.0"},
    {/* end of table */}};

MODULE_DEVICE_TABLE(of, zynq_capture_timer_of_match);


/* ===================================================================
 *
 * zynq_capture_timer_probe - Initialization method for a zynq_capture_timer device
 *
 * Return: 0 on success, negative error otherwise.
 */

static int zynq_capture_timer_probe(struct platform_device *pdev)
{
    struct resource *res;
        
    printk("In probe funtion\n");

    // This code gets the IRQ number by probing the system.

    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
   
    if (!res) {
        printk("No IRQ found\n");
        return 0;
    } 
    
    // Get the IRQ number 
    gic_interrupt  = res->start;

    printk("Probe IRQ # = %lld\n", res->start);

    return 0;

}

/* ===================================================================
 *
 * zynq_capture_timer_remove - Driver removal function
 *
 * Return: 0 always
 */
 
static int zynq_capture_timer_remove(struct platform_device *pdev)
{
    //struct zynq_capture_timer *capture_timer = platform_get_drvdata(pdev)

    return 0;
}

/* ===================================================================
 *
 * zynq_capture_timer  function
 *
 * Return: 0 always
 */
 
static struct platform_driver zynq_capture_timer_driver = {
    .driver    = {
        .name = MODULE_NM,
        .of_match_table = zynq_capture_timer_of_match,
    },
    .probe = zynq_capture_timer_probe,
    .remove = zynq_capture_timer_remove,
};


/* ===================================================================
 * function: init_capture_timer_int
 *
 * This function creates the /proc directory entry capture_timer_interrupt.
 */
 
static int __init init_capture_timer_int(void)
{

    int rv = 0;
    int err = 0;
    
//    platform_driver_unregister(&zynq_capture_timer_driver);
    
   
    printk("Ultra96 Interrupt Module\n");
    printk("Ultra96 Interrupt Driver Loading.\n");
    printk("Using Major Number %d on %s\n", CAPTURE_TIMER_MAJOR, MODULE_NM); 

    err = platform_driver_register(&zynq_capture_timer_driver);
      
    if(err !=0) printk("Driver register error with number %d\n",err);       
    else        printk("Driver registered with no error\n");
    
    if (register_chrdev(CAPTURE_TIMER_MAJOR, MODULE_NM, &capture_timer_fops)) {
        printk("capture_timer_int: unable to get major %d. ABORTING!\n", CAPTURE_TIMER_MAJOR);
    goto no_capture_timer_interrupt;
    }

    proc_capture_timer_int = proc_create("capture-timer-interrupt", 0444, NULL, &proc_fops );
    msg=kmalloc(GFP_KERNEL,10*sizeof(char));
    
    if(proc_capture_timer_int == NULL) {
          printk("capture_timer_int: create /proc entry returned NULL. ABORTING!\n");
    goto no_capture_timer_interrupt;
    }

    // Request interrupt
    printk("Getting the interrupt %d\n", gic_interrupt);
    
    rv = request_irq(gic_interrupt, 
                    (irq_handler_t) capture_timer_int_handler, 
                     IRQF_TRIGGER_RISING,
                     "capture-timer-controller", 
                     NULL);
    
    printk("Got the interrupt %d\n", gic_interrupt);
    
  
  
    if ( rv ) {
        printk("Can't get interrupt %d\n", gic_interrupt);
    goto no_capture_timer_interrupt;
    }

    printk(KERN_INFO "%s %s Initialized\n",MODULE_NM, MODULE_VER);
    
    return 0;

    // remove the proc entry on error
    
no_capture_timer_interrupt:
    kfree(msg);
    unregister_chrdev(CAPTURE_TIMER_MAJOR, MODULE_NM);
    platform_driver_unregister(&zynq_capture_timer_driver);
    remove_proc_entry("capture-timer-interrupt", NULL);
    printk(KERN_INFO  "Exitting");
    return -EBUSY;
};

/* ===================================================================
 * function: cleanup_capture_timer_interrupt
 *
 * This function frees interrupt then removes the /proc directory entry 
 * capture_timer_interrupt. 
 */
 
static void __exit cleanup_capture_timer_interrupt(void)
{

    free_irq(gic_interrupt,NULL);                   // Release IRQ    
    unregister_chrdev(CAPTURE_TIMER_MAJOR, MODULE_NM);       // Release character device
    platform_driver_unregister(&zynq_capture_timer_driver);  // Unregister the driver
    remove_proc_entry("capture-timer-interrupt", NULL);      // Remove process entry
    kfree(msg);
    printk(KERN_INFO "%s %s removed\n", MODULE_NM, MODULE_VER);
     
}


/* ===================================================================
 *
 *
 *
 */


module_init(init_capture_timer_int);
module_exit(cleanup_capture_timer_interrupt);

MODULE_AUTHOR("Mark McDermott");
MODULE_DESCRIPTION("capture-timer-interrupt proc module");
MODULE_LICENSE("GPL");

