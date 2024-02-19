/*
 *  dm.c
 *
 *  author: Mark McDermott 
 *  Created: Feb 12, 2012
 *
     dm - display memory location
     USAGE:     dm (address) (repeat #)
     
     example:   dm 0x40000000 
     output:    0x40000000 = 0x00000044

     example:   dm 0x40000000 0x3
     output:    0x40000000 = 0x00000044
                0x40000004 = 0x00000056
                0x40000008 = 0x00000057
 */
 
#include "stdio.h"
#include "stdlib.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)


int main(int argc, char * argv[]) {

	volatile unsigned int *regs, *address ;
	volatile unsigned int target_addr, offset, value, lp_cnt;
	
    int fd = open("/dev/mem", O_RDWR|O_SYNC, S_IRUSR);

	if(fd == -1)
	{
		printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	if ((argc != 2) && (argc != 3))
	{
		printf("Display Memory - USAGE:  dm (Address) (# addresses) :\n");
		close(fd);
		return -1;
	}
		
	offset = 0;
	target_addr = strtoul(argv[1], 0, 0);
    lp_cnt = 1; // Display at least 1 locations
    
    
    if (argc == 3) 	lp_cnt = strtoul(argv[2], 0, 0);
    
    if (lp_cnt > MAP_SIZE)  { // Max is MAP_SIZE
        lp_cnt = MAP_MASK>>2; 
        printf("Setting max repeat value to  0x%.8x \n", lp_cnt);
        
    }  
 
                                

    regs = (unsigned int *)mmap(NULL, 
                                MAP_SIZE,
                                PROT_READ|PROT_WRITE, 
                                MAP_SHARED, 
                                fd, 
                                target_addr & ~MAP_MASK );	


    
    while( lp_cnt) {
	
	  printf("0x%.4x" , (target_addr+offset));

      address = regs + (((target_addr+ offset) & MAP_MASK)>>2);    	
	
	  printf(" = 0x%.8x\n", *address);		// display register value
	  
	  lp_cnt -= 1;
	  offset  += 4; // WORD alligned
	
	} // End while loop
			
	int temp = close(fd);
	if(temp == -1)
	{
		printf("Unable to close /dev/ram1.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	munmap(NULL, MAP_SIZE);
	
	return 0;
}
