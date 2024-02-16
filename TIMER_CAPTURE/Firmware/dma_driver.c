#include <asm/gpio.h>
#include <asm/uaccess.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gpio/driver.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#define MODULE_VER "1.0"
#define CDMA_MAJOR 240  // TODO
#define MODULE_NM "cdma_interrupt"

// #define DMA_INTERRUPT 0x5B
// #define CAPTURE_TIMER_INTERRUPT 0x5C

int interruptcount = 0;
int temp           = 0;
int len            = 0;
unsigned int gic_interrupt;

static struct proc_dir_entry *proc_cdma_int;
static struct fasync_struct *fasync_cdma_queue;

irq_handler_t
cdma_int_handler(int irq, void *dev_id, struct pt_regs *regs) {
	interruptcount++;
	printk(KERN_INFO "cdma_int: Interrupt detected in kernel \n");

	kill_fasync(&fasync_cdma_queue, SIGIO, POLL_IN);
	return (irq_handler_t) IRQ_HANDLED;
}

static const struct of_device_id zynq_cdma_of_match[] = {
    {.compatible = "xlnx,axi-cdma-4.1"},
    {/* end of table */}};

MODULE_DEVICE_TABLE(of, zynq_cdma_of_match);

static int
zynq_cdma_probe(struct platform_device *pdev) {
	struct resource *res;
	printk("zynq_cdma_probe\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!res) {
		printk("No IRQ found\n");
		return -ENODEV;
	}

	gic_interrupt = res->start;

	return 0;
}

static struct platform_driver zynq_dcma_driver =
    {
        .driver =
            {
                .name           = "zynq-cdma",
                .of_match_table = zynq_cdma_of_match,
            },
        .probe  = zynq_cdma_probe,
        .remove = zynq_cdma_remove,
}

struct proc_ops proc_fops = {
    // NEW for 5.15.36 kernel
};

static int __init
init_cdma_interrupt(void) {
	int rv  = 0;
	int err = 0;

	printk("Initializing CDMA interrupt module\n");

	err = platform_driver_register(&zynq_cdma_driver);
	if (err) {
		printk("Failed to register CDMA with number %d\n", err);
		rv = err;
		goto no_gpio_interrupt;
	} else {
		printk("Driver registered with no error\n");
	}

	// cdma_fops points to routines that are called when
	// device is accessed from user application
	if (register_chrdev(CDMA_MAJOR, MODULE_NM, &cdma_fops)) {
		printk("cdma_int: unable to get major %d\n", CDMA_MAJOR);
		goto no_gpio_interrupt;
	}

	proc_cdma_int = proc_create("cdma_interrupt", 0444, NULL, &proc_fops);
	if (proc_cdma_int == NULL) {
		printk("cdma_int: unable to create /proc entry\n");
		goto no_gpio_interrupt;
	}

	// request interrupt
	rv = request_irq(gic_interrupt,
	                 cdma_int_handler,
	                 IRQF_TRIGGER_RISING,
	                 "cdma_interrupt",
	                 NULL);

	if (rv) {
		printk("cdma_int: unable to get IRQ %d\n", gic_interrupt);
		goto no_gpio_interrupt;
	}

	printk(KERN_INFO "%s %s initialized\n", MODULE_NM, MODULE_VER);

	return 0;

no_gpio_interrupt:
	free_irq(gic_interrupt, NULL);
	unregister_chrdev(CDMA_MAJOR, MODULE_NM);
	platform_driver_unregister(&zynq_cdma_driver);
	remove_proc_entry("cdma_interrupt", NULL);
	return -EBUSY;
}