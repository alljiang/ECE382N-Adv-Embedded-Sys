/*
 * DMA_test.c: Test the DMA Channel on the ZED board
 *
 *  AUTHOR: 	Mark McDermott
 *  CREATED: 	May 18, 2019

 *
 *  DESCRIPTION: This program sets up the CDMA in the PL and performs a
 *               multiple DMA transfers using random data
 *
 *  DEPENDENCIES: Works on the Xilinx ZED Board only
 *
 *
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>                                                                      
#include <sys/types.h>                                                                     
#include <sys/stat.h>  
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>


#define MAP_SIZE 4096UL 
#define MAP_MASK (MAP_SIZE - 1)     

#undef DEBUG 
#undef DEBUG1
//#define DEBUG                   // Comment out to turn off debug messages
//#define DEBUG1                  // Comment out to turn off debug messages

#define CDMA                0xB0000000
#define BRAM0               0xB0004000
#define BRAM1               0xB0006000
#define OCM                 0xFFFC0000


#define CDMACR              0x00
#define CDMASR              0x04
#define CURDESC_PNTR        0x08
#define CURDESC_PNTR_MSB    0x0C
#define TAILDESC_PNTR       0x10
#define TAILDESC_PNTR_MSB   0x14
#define SA                  0x18
#define SA_MSB              0x1C
#define DA                  0x20
#define DA_MSB              0x24
#define BTT                 0x28

#define ONE_BIT_MASK(_bit)    (0x00000001 << (_bit))

int smb(unsigned int target_addr, unsigned int pin_number, unsigned int bit_val);
int pm(unsigned int target_addr, unsigned int value );
unsigned int rm( unsigned int target_addr);

unsigned long int_sqrt(unsigned long n);

void compute_interrupt_latency_stats( unsigned long   *min_latency_p, 
                                      unsigned long   *max_latency_p, 
                                      unsigned long   *average_latency_p, 
                                      unsigned long   *std_deviation_p); 
                                      
/***************************  DMA_SET ************************************   
*/

unsigned int dma_set(unsigned int* dma_virtual_address, int offset, unsigned int value) {
    dma_virtual_address[offset>>2] = value;
}

/***************************  DMA_GET ************************************ 
*/

unsigned int dma_get(unsigned int* dma_virtual_address, int offset) {
    return dma_virtual_address[offset>>2];
}


// ***********************************************************************
//  This routine does the interrupt handling for the main loop.
//
static volatile int det_int=0;  // Global flag that is volatile i.e., no caching
static volatile int num_int=0;  

void sighandler(int signo)
{
    if (signo==SIGIO)
        det_int++;      // Set flag
        num_int++;
        
    #ifdef DEBUG1        
         printf("Interrupt captured by SIGIO\n  num_int = 0x%.8x\n", num_int);   // DEBUG
    #endif
   
    return;  /* Return to main loop */

}

// *************************************************************************
//                              MAIN 
// *************************************************************************

//int main(int argc, char **argv)

int main(int argc, char * argv[])   {
    unsigned int rx_cnt;        // Receive count
    struct sigaction action;    // Structure for signalling
    int fd;                     // File descriptor
    int rc;
    int fc;
    
   	volatile unsigned int  data_cnt, lp_cnt;     
    
    if ((argc < 2))
	{
		printf("DMA Test w/ Random Data - USAGE:  dma_test # loops (# random data) \n");
		return -1;
	}
		
	lp_cnt      = 1;        
	data_cnt    = 32;      // Transfer 32 words
	
	lp_cnt = strtoul(argv[1], 0, 0);   
    
    
    if (argc == 3) {
        data_cnt  = strtoul(argv[2], 0, 0);
    }

    if (data_cnt > 0x3ff)  {      // Max is 4096 bytes
        data_cnt = 0x3ff; 
        printf("Setting max repeat value to 0x3ff\n");
    }

//    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGIO);
    
    action.sa_handler = sighandler;
    action.sa_flags = 0;

    sigaction(SIGIO, &action, NULL);
    
    fd = open("/dev/dma_int", O_RDWR);
    if (fd == -1) {
    	perror("Unable to open /dev/dma_interrupt");
    	rc = fd;
    	exit (-1);
    }
    
    #ifdef DEBUG1
        printf("/dev/dma_int opened successfully \n");    	
    #endif
    
    fc = fcntl(fd, F_SETOWN, getpid());
    
    #ifdef DEBUG1
        printf("Made it through fcntl\n");
    #endif
    
        
    if (fc == -1) {
    	perror("SETOWN failed\n");
    	rc = fd;
    	exit (-1);
    } 
    
    fc= fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC);

    if (fc == -1) {
    	perror("SETFL failed\n");
    	rc = fd;
    	exit (-1);
    }   


// *************************************************************************
// Open up /dev/mem for mmap operations
//

	// Open /dev/mem which represents the whole physical memory
    int dh = open("/dev/mem", O_RDWR | O_SYNC); 
    if(dh == -1)
	{
		printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		printf("Must be root to run this routine.\n");
		return -1;
	}
	                                          
    
    #ifdef DEBUG1                                                                       
        printf("Getting ready to mmap cdma_virtual_address \n");    
    #endif
    
    uint32_t* cdma_virtual_address = mmap(NULL, 
                                          4096, 
                                          PROT_READ | PROT_WRITE, 
                                          MAP_SHARED, 
                                          dh, 
                                          CDMA); // Memory map AXI Lite register block
    #ifdef DEBUG1 
        printf("cdma_virtual_address = 0x%.8x\n", cdma_virtual_address); 
    #endif
    
    #ifdef DEBUG1                                  
        printf("Getting ready to mmap BRAM_virtual_address \n");    
    #endif
        
    uint32_t* BRAM_virtual_address = mmap(NULL, 
                                          4096, 
                                          PROT_READ | PROT_WRITE, 
                                          MAP_SHARED, 
                                          dh, 
                                          BRAM0); // Memory map AXI Lite register block
    #ifdef DEBUG1        
        printf("BRAM_virtual_address = 0x%.8x\n", BRAM_virtual_address); 
    #endif                                    
    
    
    // *************************************************************************
    // Set up the OCM with data to be transferred to the BRAM
    //
    
    uint32_t* ocm = mmap(NULL, 65536, PROT_READ | PROT_WRITE, MAP_SHARED, dh, OCM);
    
    for(int i=0; i<data_cnt; i++)
        ocm[i] = rand();
    
    // RESET DMA
    dma_set(cdma_virtual_address, CDMACR, 0x0004);	    


    pid_t   childpid;
    int     cnt;
    int     status;
    //int     length  =   20;     // Arbitrary value

// ------------------------------------------------------------------------------------
// This for loop performs "lp_cnt" # DMA Transfers
// ------------------------------------------------------------------------------------

    for(cnt = 0; cnt < lp_cnt; cnt++) 
    {

        // -----------------------------------------------------------------------------
        // Fork off a child process to set up the DMA and start the transfer
        // -----------------------------------------------------------------------------
        
        childpid = vfork();

        if (childpid >=0)    // Fork suceeded    
        {
            // -------------------------------------------------------------------------
            // This code runs in the child process as the childpid == 0
            // -------------------------------------------------------------------------
            
            if (childpid == 0)
            {
                sleep (.5);
                dma_set(cdma_virtual_address, DA, BRAM1);       // Write destination address
                dma_set(cdma_virtual_address, SA, OCM);         // Write source address
                dma_set(cdma_virtual_address, CDMACR, 0x1000);  // Enable interrupts
                dma_set(cdma_virtual_address, BTT, data_cnt*4); // Start transfer
                exit(0);  // Exit the child process
            }
            // -------------------------------------------------------------------------
            // This code runs in the parent process as the childpid != 0
            // -------------------------------------------------------------------------
            
           else 
           {
 
                // ----------------------------------------------------------------
                // Wait for child process to terminate before checking for interrupt 
                // ----------------------------------------------------------------

                waitpid(childpid, &status, WCONTINUED);

                if (!det_int) continue;     // Go back to top of while loop.
                
                det_int = 0;                // Clear interrupt detected flag
                
                dma_set(cdma_virtual_address, CDMACR, 0x0000);  // Disable interrupts
                
              
                // ----------------------------------------------------------------
                // Check to make sure transfer was correct
                // ----------------------------------------------------------------

                for(int i=0; i < data_cnt; i++)
                {
                    if(BRAM_virtual_address[i] != ocm[i])
                    {
                        printf("test failed!!\n"); 

                        #ifdef DEBUG1
                            printf("RAM result: 0x%.8x and c result is 0x%.8x  element %d\n", 
                               BRAM_virtual_address[i], 
                               c[i], i);
                        #endif
                        
                        munmap(ocm,65536);
                        munmap(cdma_virtual_address,4096);
                        munmap(BRAM_virtual_address,4096);
                        
                    return -1;
                    }
                }
                #ifdef DEBUG
                printf("test passed!!\n");
                #endif
                
                // ----------------------------------------------------------------
                // Clear 20 locations in the BRAM for next transfer
                // ----------------------------------------------------------------

                for(int i=0; i < 20; i++)
                {
                    BRAM_virtual_address[i] = 0x00;
                } 

             } // if chilpid ==0
       } // if childpid >=0

       else  // fork failed
       {
           perror("Fork failed");
           exit(0);
       } // if childpid >=0

        
    }  //  for ......
    
    printf("Test passed ----- %d loops and %d words \n", lp_cnt, (data_cnt+1));
    munmap(ocm,65536);
    munmap(cdma_virtual_address,4096);
    munmap(BRAM_virtual_address,4096);
    return 0; 
    
}   // END OF main



// ************************ READ MEMORY (RM) ROUTINE **************************
//
unsigned int rm( unsigned int target_addr) 
{
	int fd = open("/dev/mem", O_RDWR|O_SYNC);
	volatile unsigned int *regs, *address ;
	
	if(fd == -1)
	{
		perror("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	
		
	regs = (unsigned int *)mmap(NULL, 
	                            MAP_SIZE, 
	                            PROT_READ|PROT_WRITE, 
	                            MAP_SHARED, 
	                            fd, 
	                            target_addr & ~MAP_MASK);		

    address = regs + (((target_addr) & MAP_MASK)>>2);    	
    //printf("Timer register = 0x%.8x\n", *address);
    
	unsigned int rxdata = *address;         // Perform read of SPI 
            	
	int temp = close(fd);                   // Close memory
	if(temp == -1)
	{
		perror("Unable to close /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	munmap(NULL, MAP_SIZE);                 // Unmap memory
	return rxdata;                          // Return data from read

}   // End of em routine



// ************************ PUT MEMORY (PM) ROUTINE **************************
//

int pm( unsigned int target_addr, unsigned int value ) 
{
	int fd = open("/dev/mem", O_RDWR|O_SYNC);
	volatile unsigned int *regs, *address ;
	
	if(fd == -1)
	{
		perror("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");		
		return -1;
	}	
		
	regs = (unsigned int *)mmap(NULL, 
	                            MAP_SIZE, 
	                            PROT_READ|PROT_WRITE, 
	                            MAP_SHARED, 
	                            fd, 
	                            target_addr & ~MAP_MASK);		

    address = regs + (((target_addr) & MAP_MASK)>>2);    	

	*address = value; 	                    // Perform write command
        	
	int temp = close(fd);                   // Close memory
	if(temp == -1)
	{
		perror("Unable to close /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	munmap(NULL, MAP_SIZE);                 // Unmap memory
	return 0;                               // Return status

}   // End of pm routine



// ***********************  SET MEMORY BIT (SMB) ROUTINE  *****************************
// 

int smb(unsigned int target_addr, unsigned int pin_number, unsigned int bit_val)
{
    unsigned int reg_data;

    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    
    if(fd == -1)
    {
        printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
        return -1;
    }    

    volatile unsigned int *regs, *address ;
    
    regs = (unsigned int *)mmap(NULL, 
                                MAP_SIZE, 
                                PROT_READ|PROT_WRITE, 
                                MAP_SHARED, 
                                fd, 
                                target_addr & ~MAP_MASK);
    
    address = regs + (((target_addr) & MAP_MASK)>>2);

#ifdef DEBUG1
    printf("REGS           = 0x%.8x\n", regs);    
    printf("Target Address = 0x%.8x\n", target_addr);
    printf("Address        = 0x%.8x\n", address);       // display address value      
#endif 
   
    /* Read register value to modify */
    
    reg_data = *address;
    
    if (bit_val == 0) {
        
        // Deassert output pin in the target port's DR register
        
        reg_data &= ~ONE_BIT_MASK(pin_number);
        *address = reg_data;
    } else {
        
        // Assert output pin in the target port's DR register
                
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
   
}    // End of smb routine

