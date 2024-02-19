/*
 *  dm.c
 *
 *  author: Mark McDermott 
 *  Created: Feb 12, 2012
 *
 *  Simple utility to allow the use of the /dev/mem device to display memory
 *  and write memory addresses on the i.MX21.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
#include "stdio.h"
#include "stdlib.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int main(int argc, char * argv[]) {

/* 
* This section of code is needed if you are accessing FLASH memory. the mmap() routine
* seems to leave the flash in a strange state after the first access
*/

unsigned int v;
        int mtd= open("/dev/mmcblk0p1", O_RDONLY|O_SYNC);
/*
        if(mtd){ 
    		printf("Opened /dev/mmcblk0p1\n");
	}
*/
        if(!mtd){
          printf("Unable to open /dev/mmcblk0p1.  Ensure it exists (major=31, minor=0)\n");
             return -1;
        }
        if (    lseek(mtd,  0x00000000 ,SEEK_SET) > -1){
                read(mtd, &v, 4);
        }
         else {printf("Unable to move to start of memory for /dev/mmcblk0p1.\n");}


        close(mtd);
	
	int fd = open("/dev/mem", O_RDWR|O_SYNC, S_IRUSR);
	volatile unsigned int *regs, *address ;
	
	unsigned long target_addr, offset, value, lp_cnt;
/*
	if(fd != -1){
		printf(" Opened /dev/mem \n");
	}
*/
	if(fd == -1)
	{
		printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	if ((argc != 2) && (argc != 3))
	{
		printf("USAGE:  dm (address) (repeat) :\n");
		close(fd);
		return -1;
	}
		
	offset = 0;
	target_addr = strtoul(argv[1], 0, 0);
    lp_cnt = 1; // Display at least 1 locations
    
    if (argc == 3) 	lp_cnt = strtoul(argv[2], 0, 0);
    
    regs = (unsigned int *)mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, target_addr & ~MAP_MASK);	
    
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
