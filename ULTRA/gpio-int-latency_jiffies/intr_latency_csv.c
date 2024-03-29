/* -------------------------------------------------------------------------------
 *  intr_latency.c
 *
 *  author: Mark McDermott
 *  Created: Jan 30, 2009
 *  Updated: April 23, 2017 for Zedboard
 *
 * This routine measures the average latecncy from the time an interrupt input
 * to the SOC occurs to when it is handled. This is used to determine how fast
 * the SOC can handle interrupts from the H1KP
 * 
 */
 
 

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/version.h>
#include <math.h>    
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

//#define DEBUG



/* -------------------------------------------------------------------------------  
 * One-bit masks for bits 0-31
 */
#define ONE_BIT_MASK(_bit)    (0x00000001 << (_bit))

/* -------------------------------------------------------------------------------*/

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

/* ------------------------------------------------------------------------------- 
 * Device path name for the MX6 GPIO device
 */
 
#define GPIO_DEV_PATH    "/dev/gpio_int"

#define GPIO_PIN_NUM      0x0
#define GPIO_DR    0x43c00000    // Data register
#define GPIO_ISR   0x43c00000    // Interrupt status register

/* -------------------------------------------------------------------------------
 * Number of interrupt latency measurements to take:
 */
 
#define NUM_MEASUREMENTS    10000

/* -------------------------------------------------------------------------------
 * File descriptor for GPIO device
 */

int gpio_dev_fd  = -1;

/* -------------------------------------------------------------------------------
 * counter of number of times sigio_signal_handler() has been executed
 */
volatile int sigio_signal_count = 0;

/* -------------------------------------------------------------------------------
 * Flag to indicate that a SIGIO signal has been processed
 */
static volatile sig_atomic_t sigio_signal_processed = 0;
 
/* -------------------------------------------------------------------------------
 * Time stamp set in the last sigio_signal_handler() invocation:
 */
struct timeval sigio_signal_timestamp;

/* -------------------------------------------------------------------------------
 * Array of interrupt latency measurements (in micro seconds):
 */
unsigned long intr_latency_measurements[NUM_MEASUREMENTS];

/* -------------------------------------------------------------------------------
 * Function prototypes
 */

void sigio_signal_handler(int signo);

void compute_interrupt_latency_stats(
    unsigned long   *min_latency_p, 
    unsigned long   *max_latency_p, 
    double          *average_latency_p, 
    double          *std_deviation_p); 


/* -------------------------------------------------------------------------------
 * Main routine
 */
 
int
main(void)
{ 
    volatile int rc;
    int i;
    struct timeval start_timestamp;
    
    //printf("Entering main routine\n");    

    /* --------------------------------------------------------------------------
     * Register signal handler for SIGIO signal:
     */

    struct sigaction sig_action; 
    
    memset(&sig_action, 0, sizeof sig_action);
    sig_action.sa_handler = sigio_signal_handler;

    /* --------------------------------------------------------------------------
     * Block all signals while our signal handler is executing:
     */
    (void)sigfillset(&sig_action.sa_mask);

    rc = sigaction(SIGIO, &sig_action, NULL);

    if (rc == -1) {
        perror("sigaction() failed");
        return -1;
    }

    /* -------------------------------------------------------------------------
     * Open the device file
     */    
    gpio_dev_fd = open(GPIO_DEV_PATH, O_RDWR);

     if(gpio_dev_fd == -1)    {
        perror("open() of " GPIO_DEV_PATH " failed");
        return -1;
    }    

    /* -------------------------------------------------------------------------
     * Set our process to receive SIGIO signals from the GPIO device:
     */
    rc = fcntl(gpio_dev_fd, F_SETOWN, getpid());
    
    if (rc == -1) {
        perror("fcntl() SETOWN failed\n");
        return -1;
    } 

    /* -------------------------------------------------------------------------
     * Enable reception of SIGIO signals for the gpio_dev_fd descriptor
     */
    int fd_flags = fcntl(gpio_dev_fd, F_GETFL);
        rc = fcntl(gpio_dev_fd, F_SETFL, fd_flags | O_ASYNC);

    if (rc == -1) {
        perror("fcntl() SETFL failed\n");
        return -1;
    } 

    /* -------------------------------------------------------------------------
     * Take interrupt latency measurements in a loop:
     */

    rc = gpio_set_pin(GPIO_DR, GPIO_PIN_NUM, 0);  // Clear output pin
    if (rc != 0) { 
        perror("gpio_set_pin() failed");
    return -1;
    }

    sigset_t signal_mask, signal_mask_old, signal_mask_most;
    
    for (i = 0; i < NUM_MEASUREMENTS; i ++) {

        /* ---------------------------------------------------------------------
         * Reset sigio_signal_processed flag:
         */
        sigio_signal_processed = 0; 

        /* ---------------------------------------------------------------------
         * NOTE: This next section of code must be excuted each cycle to prevent
         * a race condition between the SIGIO signal handler and sigsuspend()
         */
         
        (void)sigfillset(&signal_mask);
        (void)sigfillset(&signal_mask_most);
        (void)sigdelset(&signal_mask_most, SIGIO);
        (void)sigprocmask(SIG_SETMASK,&signal_mask, &signal_mask_old);       
        
        /* ---------------------------------------------------------------------
         * Take a start timestamp for interrupt latency measurement
         */
        (void)gettimeofday(&start_timestamp, NULL); 

        /* ---------------------------------------------------------------------
         * Assert GPIO output pin to trigger generation of edge sensitive interrupt:
         */

        rc = gpio_set_pin(GPIO_DR, GPIO_PIN_NUM, 1);
        if (rc != 0) { 
            perror("gpio_set_pin() failed");
        return -1;
        }
                
        /* ---------------------------------------------------------------------
         * Wait for SIGIO signal handler to be executed. 
         */
        if (sigio_signal_processed == 0) { 

            rc = sigsuspend(&signal_mask_most);
            
            /* Confirm we are coming out of suspend mode correcly */
            assert(rc == -1 && errno == EINTR && sigio_signal_processed);
        }   
              
        (void)sigprocmask(SIG_SETMASK, &signal_mask_old, NULL);
    
        assert(sigio_signal_count == i + 1);   // Critical assertion!!

        rc = gpio_set_pin(GPIO_DR, GPIO_PIN_NUM, 0);
        
        if (rc != 0) { 
            perror("gpio_set_pin() failed");
        return -1;
        } 
         
        /* ---------------------------------------------------------------------
         * Compute interrupt latency:
         */
        intr_latency_measurements[i] = 
            (sigio_signal_timestamp.tv_sec -
             start_timestamp.tv_sec) * 1000000 + 
            (sigio_signal_timestamp.tv_usec -
             start_timestamp.tv_usec);
        
    }  // End of for loop

    /* -------------------------------------------------------------------------
     * Close device file
     */
    (void)close(gpio_dev_fd);

    /* -------------------------------------------------------------------------
     * Compute interrupt latency stats:
     */

    unsigned long   min_latency; 
    unsigned long   max_latency; 
    double          average_latency; 
    double          std_deviation; 
     
    compute_interrupt_latency_stats(
                        &min_latency, 
                        &max_latency,
                        &average_latency, 
                        &std_deviation);

    /*
     * Print interrupt latency stats:
     */
    printf("%lu ," "%lu ," "%f ," "%f\n",
            min_latency, 
            max_latency, 
            average_latency, 
            std_deviation);

    return 0;
}


/* -----------------------------------------------------------------------------
 * SIGIO signal handler
 */
 
void
sigio_signal_handler(int signo)
{
    volatile int rc1;

    assert(signo == SIGIO);   // Confirm correct signal #
    sigio_signal_count ++;

    //printf("sigio_signal_handler called (signo=%d)\n", signo);

    /* -------------------------------------------------------------------------
     * Set global flag
     */
     
    sigio_signal_processed = 1; 
    
    /* -------------------------------------------------------------------------
     * Take end timestamp for interrupt latency measurement
     */
    (void)gettimeofday(&sigio_signal_timestamp, NULL); 


    /* -------------------------------------------------------------------------
     * De-assert GPIO output pin:
     */
/*     
    rc1 = gpio_set_pin(GPIO_DR, GPIO_PIN_NUM, 0);
    
    if (rc1 != 0) { 
        perror("ioctl to de-assert output pin failed");
    } */
    
    //printf("Exitting sigio_signal_handler  (signo=%d)\n", signo);

}


/* -----------------------------------------------------------------------------
 * Compute interrupt latency stats
 */
void
compute_interrupt_latency_stats(
    unsigned long   *min_latency_p, 
    unsigned long   *max_latency_p, 
    double          *average_latency_p, 
    double          *std_deviation_p)
{
    int i;
    unsigned long   val;
    unsigned long   min = ULONG_MAX;
    unsigned long   max = 0;
    unsigned long   sum = 0;
    unsigned long   sum_squares = 0;

    for (i = 0; i < NUM_MEASUREMENTS; i ++) {
        val = intr_latency_measurements[i];

        if (val < min) {
            min = val;
        } 
        
        if (val > max) {
            max = val;
        }

        sum += val;
        sum_squares += val * val;
    }

    *min_latency_p = min;
    *max_latency_p = max;

    unsigned long average = (unsigned long)sum / NUM_MEASUREMENTS;

    unsigned long std_deviation = int_sqrt((sum_squares / NUM_MEASUREMENTS) -
                    (average * average));

                    
       *average_latency_p = average;
    *std_deviation_p = std_deviation;
}



/* ---------------------------------------------------------------
* sqrt routine
*/

 int_sqrt(unsigned long n)
{
   unsigned long root = 0;
   unsigned long bit;
   unsigned long trial;
   
   bit = (n >= 0x10000) ? 1<<30 : 1<<14;
   do
   {
      trial = root+bit;
      if (n >= trial)
      {
         n -= trial;
         root = trial+bit;
      }
      root >>= 1;
      bit >>= 2;
   } while (bit);
   return root;
}

/* -----------------------------------------------------------------------------
*
* gpio_set_pin routine: This routine sets and clears a single bit
* in a GPIO register.
*
*/

int gpio_set_pin(unsigned int target_addr, unsigned int pin_number, unsigned int bit_val)
{
    unsigned int reg_data;

    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    
    if(fd == -1)
    {
        printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
        return -1;
    }    

    volatile unsigned int *regs, *address ;
    
    regs = (unsigned int *)mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, target_addr & ~MAP_MASK);
    
    address = regs + (((target_addr) & MAP_MASK)>>2);

#ifdef DEBUG1
    printf("REGS           = 0x%.8x\n", regs);    
    printf("Target Address = 0x%.8x\n", target_addr);
    printf("Address        = 0x%.8x\n", address);                    // display address value    
    //printf("Mask           = 0x%.8x\n", ONE_BIT_MASK(pin_number));  // Display mask value
#endif 
   
    /* Read register value to modify */
    
    reg_data = *address;
    
    if (bit_val == 0) {
        
        /* Deassert output pin in the target port's DR register*/
        
        reg_data &= ~ONE_BIT_MASK(pin_number);
        *address = reg_data;
    } else {
        
        /* Assert output pin in the target port's DR register*/
                
        reg_data |= ONE_BIT_MASK(pin_number);
        *address = reg_data;
    }
    
    int temp = close(fd);
    if(temp == -1)
    {
        printf("Unable to close /dev/mem.  Ensure it exists (major=1, minor=1)\n");
        return -1;
    }    

    munmap(NULL, MAP_SIZE);        
    return 0;
    

    
}    
    
    




