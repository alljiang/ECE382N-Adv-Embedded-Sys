/*
 * DMA_test.c: Test the DMA Channel on the ZED board
 *
 *  AUTHOR: 	Mark McDermott
 *  CREATED: 	May 18, 2019

 *
 *  DESCRIPTION: This program sets up the CDMA in the PL and performs a
 *               multiple DMA transfers
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
#define DEBUG                   // Comment out to turn off debug messages
//#define DEBUG1                   // Comment out to turn off debug messages

#define CDMA                0x70000000
#define BRAM0               0x40000000
#define BRAM1               0x40000000
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


// *************************************************************************
//  This routine does the interrupt handling for the main loop.
//
static volatile int det_int=0;  // Global flag that is volatile i.e., no caching

void sighandler(int signo)
{
    if (signo==SIGIO)
        det_int++;      // Set flag
    #ifdef DEBUG1        
         printf("Interrupt captured by SIGIO\n");  // DEBUG
    #endif
   
    return;  /* Return to main loop */

}

// *************************************************************************
//                              MAIN 
// *************************************************************************

int main(int argc, char **argv)
{
    unsigned int rx_cnt;        // Receive count
    struct sigaction action;    // Structure for signalling
    int fd;                     // File descriptor
    int rc;
    int fc;

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
    
    // Setup data to be transferred
    uint32_t c[20] = {  0x2caf3444,
                        0x1ab0eab6,
                        0x11ccbda2,
                        0x81991ac6,
                        0x11110a03,
                        0x2499aef8,
                        0x55aa55aa,
                        0x44556677,
                        0xaaacccd6,
                        0x00000008,
                        0x10000003,
                        0x00911a42,
                        0x1ab0eab6,
                        0x11ffbda2,
                        0x8199aac6,
                        0x1aa10a03,
                        0x24944ef8,
                        0x553355aa,
                        0x12345678,
                        0x000cccd6};  

    // *************************************************************************
    // Set up the OCM with data to be transferred to the BRAM
    //
    
    uint32_t* ocm = mmap(NULL, 65536, PROT_READ | PROT_WRITE, MAP_SHARED, dh, OCM);
    
    for(int i=0; i<20; i++)
        ocm[i] = c[i];
    
    // RESET DMA
    dma_set(cdma_virtual_address, CDMACR, 0x0004);	    


    pid_t   childpid;
    int     cnt;
    int     status;
    int     length  =   20;     // Arbitrary value

// ------------------------------------------------------------------------------------
// This for loop performs 32 DMA Transfers
// ------------------------------------------------------------------------------------

    for(cnt = 0; cnt < 32; cnt++) 
    {

        // -----------------------------------------------------------------------------
        // Fork off a child process to send data to SPI TXFIFO
        // -----------------------------------------------------------------------------
        
        childpid = fork();

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
                dma_set(cdma_virtual_address, BTT, length*4);   // Start transfer
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
                dma_set(cdma_virtual_address, CDMACR, 0x0004);  // Disable interrupts
                
              
                // ----------------------------------------------------------------
                // Check to make sure transfer was correct
                // ----------------------------------------------------------------

                for(int i=0; i < length; i++)
                {
                    if(BRAM_virtual_address[i] != c[i])
                    {
                        #ifdef DEBUG
                            printf("test failed!!\n"); 
                        #endif

                        #ifdef DEBUG1
                            printf("RAM result: 0x%.8x and c result is 0x%.8x  element %d\n", 
                               BRAM_virtual_address[i], c[i], i);
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
                // Clear BRAM for next transfer
                // ----------------------------------------------------------------

                for(int i=0; i<20; i++)
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

    munmap(ocm,65536);
    munmap(cdma_virtual_address,4096);
    munmap(BRAM_virtual_address,4096);
    return 0; 
    
}   // END OF main




