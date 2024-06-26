/* ===================================================================
 *  dma_interrupt.c
 *
 *  AUTHOR:     Mark McDermott

 *  CREATED:    May 19, 2019     Updated for ZED Board
 *
 *  DESCRIPTION: This kernel module registers interrupts from the DMA
 *               unit and measures the time between them. This is
 *               used to also measure the latency through the kernel
 *               to respond to interrupts. 
 *               
 *  DEPENDENCIES: Works on Xilinx ZED Board
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
//#include <asm/gpio.h>
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
//#define INTERRUPT  164                  // Should use gic_interrupt number

#define DMA_MAJOR 235                  // Need to mknod /dev/dma_int c 241 0
#define MODULE_NM "dma_interrupt"

#undef DEBUG
#define DEBUG

int             interruptcount  = 0;
int             temp            = 0;
int             len             = 0;
char            *msg            = NULL;
unsigned int    gic_interrupt;

static struct fasync_struct *fasync_dma_queue ;

/* ===================================================================
 * function: dma_int_handler
 *
 * This function is the dma_interrupt handler. It sets the tv2
 * structure using do_gettimeofday.
 */
 
//static irqreturn_t dma_int_handler(int irq, void *dev_id, struct pt_regs *regs)

irq_handler_t dma_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
  interruptcount++;
  
    #ifdef DEBUG
    printk(KERN_INFO "dma_int: Interrupt detected in kernel \n");  // DEBUG
    #endif
  
    /* Signal the user application that an interupt occured */
  
    kill_fasync(&fasync_dma_queue, SIGIO, POLL_IN);

return  (irq_handler_t) IRQ_HANDLED;

}


static struct proc_dir_entry *proc_dma_int;

/* ===================================================================
*    function: read_proc   --- Example code
*/


static ssize_t read_proc(struct file *filp,char *buf,size_t count,loff_t *offp ) 
{

   
    printk("read_proc count value = %d\n", 44);
      
return 0;
}


/* ===================================================================
*    function: write_proc   --- Example code
*/

static ssize_t  write_proc(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
    

    printk("write_proc count value = %d\n", 67);

return count;
}


/* ===================================================================
 * function: dma_open
 *
 * This function is called when the dma_int device is opened
 *
 */
 
static int dma_open (struct inode *inode, struct file *file) {

#ifdef DEBUG
        printk(KERN_INFO "dma_int: Inside dma_open \n");  // DEBUG
#endif
    return 0;
}

/* ===================================================================
 * function: dma_release
 *
 * This function is called when the dma_int device is
 * released
 *
 */
 
static int dma_release (struct inode *inode, struct file *file) {

#ifdef DEBUG
        printk(KERN_INFO "\ndma_int: Inside dma_release \n");  // DEBUG
#endif
    return 0;
}

/* ===================================================================
 * function: dma_fasync
 *
 * This is invoked by the kernel when the user program opens this
 * input device and issues fcntl(F_SETFL) on the associated file
 * descriptor. fasync_helper() ensures that if the driver issues a
 * kill_fasync(), a SIGIO is dispatched to the owning application.
 */

static int dma_fasync (int fd, struct file *filp, int on)
{
    #ifdef DEBUG
    printk(KERN_INFO "\ndma_int: Inside dma_fasync \n");  // DEBUG
    #endif
    
    return fasync_helper(fd, filp, on, &fasync_dma_queue);
}; 

/* ===================================================================
*
*  Define which file operations are supported
*
*/

struct file_operations dma_fops = {
    .owner          =    THIS_MODULE,
    .llseek         =    NULL,
    .read           =    NULL,
    .write          =    NULL,
    .poll           =    NULL,
    .unlocked_ioctl =    NULL,
    .mmap           =    NULL,
    .open           =    dma_open,
    .flush          =    NULL,
    .release        =    dma_release,
    .fsync          =    NULL,
    .fasync         =    dma_fasync,
    .lock           =    NULL,
    .read           =    NULL,
    .write          =    NULL,
};

static const struct proc_ops proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

/* ===================================================================
 
   This struct is critical. Make sure that the 'compatible' value
   matches what is in the the DTS file:
   
         dma@40000000 {
			#dma-cells = <0x1>;
			clock-names = "s_axi_lite_aclk", "m_axi_aclk";
			clocks = <0x1 0xf 0x1 0xf>;
			compatible = "xlnx,cdma_int";  <------------------------------
			:
			:
			:
		};
 */

static const struct of_device_id zynq_dma_of_match[] = {
    {.compatible = "xlnx,axi-cdma-4.1"}, {/* end of table */}};

MODULE_DEVICE_TABLE(of, zynq_dma_of_match);


/* ===================================================================
 *
 * zynq_dma_probe - Initialization method for a zynq_dma device
 *
 * Return: 0 on success, negative error otherwise.
 */

static int zynq_dma_probe(struct platform_device *pdev)
{
    struct resource *res;
        
    printk("In probe funtion\n");

    // This code gets the IRQ number by probing the system.

    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
   
    if (!res) {
        printk("No IRQ found\n");
        return 0;
        }
    else   {
        printk("IRQ Found");
        
    } 
    
    // Get the IRQ number 
    gic_interrupt  = res->start;

    printk("Probe IRQ # = %d\n", res->start);

    return 0;

}

/* ===================================================================
 *
 * zynq_dma_remove - Driver removal function
 *
 * Return: 0 always
 */
 
static int zynq_dma_remove(struct platform_device *pdev)
{
    //struct zynq_dma *dma = platform_get_drvdata(pdev)

    return 0;
}


static struct platform_driver zynq_dma_driver = {
    .driver    = {
        .name = MODULE_NM,
        .of_match_table = zynq_dma_of_match,
    },
    .probe = zynq_dma_probe,
    .remove = zynq_dma_remove,
};


/* ===================================================================
 * function: init_dma_int
 *
 * This function creates the /proc directory entry dma_interrupt.
 */
 
static int __init init_dma_int(void)
{

    int rv = 0;
    int err = 0;
    
    //platform_driver_unregister(&zynq_dma_driver);
    
   
    printk("ZED Interrupt Module\n");
    printk("ZED Interrupt Driver Loading.\n");
    printk("Using Major Number %d on %s\n", DMA_MAJOR, MODULE_NM); 

    err = platform_driver_register(&zynq_dma_driver);
    printk("err = %d\n", err); 
    
    if(err !=0) printk("Driver register error with number %d\n",err);       
    else        printk("Driver registered with no error\n");
    
    if (register_chrdev(DMA_MAJOR, MODULE_NM, &dma_fops)) {
        printk("dma_int: unable to get major %d. ABORTING!\n", DMA_MAJOR);
    goto no_dma_interrupt;
    }

    proc_dma_int = proc_create("dma-interrupt", 0444, NULL, &proc_fops );
    
    msg=kmalloc(GFP_KERNEL,10*sizeof(char));
    
    if(proc_dma_int == NULL) {
          printk("dma_int: create /proc entry returned NULL. ABORTING!\n");
    goto no_dma_interrupt;
    }
    
    printk("Getting ready to request IRQ using gic_interrupt = %d\n", gic_interrupt);
    
    // Request interrupt
    
    rv = request_irq(gic_interrupt, 
                    (irq_handler_t) dma_int_handler, 
                     IRQF_TRIGGER_RISING,
                     "dma-controller", 
                     NULL);
   
    
   /* 
    rv = request_irq(46, 
                    (irq_handler_t) dma_int_handler, 
                     0x84,
                    "xilinx-dma-controller", NULL);
  */
  
    if ( rv ) {
        printk("Can't get interrupt %d\n", gic_interrupt);
    goto no_dma_interrupt;
    }

    printk(KERN_INFO "%s %s Initialized\n",MODULE_NM, MODULE_VER);
    
    return 0;

    // remove the proc entry on error
    
no_dma_interrupt:
    unregister_chrdev(DMA_MAJOR, MODULE_NM);
    platform_driver_unregister(&zynq_dma_driver);
    remove_proc_entry("dma-interrupt", NULL);
    return -EBUSY;
};

/* ===================================================================
 * function: cleanup_dma_interrupt
 *
 * This function frees interrupt then removes the /proc directory entry 
 * dma_interrupt. 
 */
 
static void __exit cleanup_dma_interrupt(void)
{

    free_irq(gic_interrupt,NULL);                   // Release IRQ    
    unregister_chrdev(DMA_MAJOR, MODULE_NM);       // Release character device
    platform_driver_unregister(&zynq_dma_driver);  // Unregister the driver
    remove_proc_entry("dma-interrupt", NULL);      // Remove process entry
    kfree(msg);
    printk(KERN_INFO "%s %s removed\n", MODULE_NM, MODULE_VER);
     
}


/* ===================================================================
 *
 *
 *
 */


module_init(init_dma_int);
module_exit(cleanup_dma_interrupt);

MODULE_AUTHOR("Mark McDermott");
MODULE_DESCRIPTION("dma-interrupt proc module");
MODULE_LICENSE("GPL");

