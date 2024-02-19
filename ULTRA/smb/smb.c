/*
 *  smb.c
 *
 *  author: Mark McDermott 
 *  Created: Feb 12, 2012
 *
      smb - Set/Clear memory bit   
      USAGE:    smb (address) (pin number) (data)  

      EXAMPLE:  smb 0x40000000 0x2 0x1
                Read  0x40000000 = 0x00000000
                Write 0x40000000 = 0x00000004
                
                smb 0x40000000 0x2 0x0
                Read  0x40000000 = 0x00000004
                Write 0x40000000 = 0x00000000
 *
 */
 
#include "stdio.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/* -------------------------------------------------------------------------------  
 * One-bit masks for bits 0-31
 */

#define ONE_BIT_MASK(_bit)	(0x00000001 << (_bit))


#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

// ------------------------------------------------------------------------------
//
//  Main Routine
//
// -----------------------------------------------------------------------------


int main(int argc, char * argv[]) {

	volatile unsigned int *regs, *address ;
	volatile unsigned int target_addr;
    volatile unsigned int pin_number;
	volatile unsigned int bit_val;
	volatile unsigned int reg_val;
	
    int fd = open("/dev/mem", O_RDWR|O_SYNC);

	if(fd == -1)
	{
		printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	if (argc != 4)
	{
		printf("Set/Clear memory bit - USAGE:  smb (address) (pin number) (data)  \n");
		close(fd);
		return -1;
	}
		
	target_addr = strtoul(argv[1], 0, 0);
	pin_number  = strtoul(argv[2], 0, 0);
	bit_val     = strtoul(argv[3], 0, 0);
	
	regs = (unsigned int *)mmap(NULL, 
                                    MAP_SIZE, 
                                    PROT_READ|PROT_WRITE, 
                                    MAP_SHARED, 
                                    fd, 
                                    target_addr & ~MAP_MASK
                                    );		
   

    address = regs + (((target_addr) & MAP_MASK)>>2);      	
    reg_val = *address;		     // Read register value
	
	printf("Read  0x%.8x" , (target_addr));
    printf(" = 0x%.8x\n", *address);    // display register value
       
    if (bit_val == 0) 
        {
        
         /* Deassert output pin in the target port's DR register*/
		
    	reg_val &= ~ONE_BIT_MASK(pin_number);
	    *address = reg_val;
        } 
     else 
         {
	    
         /* Assert output pin in the target port's DR register*/
        	    
        reg_val |= ONE_BIT_MASK(pin_number);
	    *address = reg_val;
	 }	 
	 printf("Write 0x%.8x" , (target_addr));	
     printf(" = 0x%.8x\n", *address);    // display register value after write

  	 int temp = close(fd);
	 if(temp == -1)
	 {
		printf("Unable to close /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	 }	

	 munmap(NULL, MAP_SIZE);
	
     return 0;
}
