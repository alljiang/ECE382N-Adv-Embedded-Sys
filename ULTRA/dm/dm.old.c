/*
 *  peekpoke.c
 *
 *  author: Tarun Tuli <tarun@virtualcogs.com>
 *  Created: Sept 22, 2007
 *  Copyright: 2007 Virtual Cogs Embedded Systems Inc.
 *
 *  Simple utility to allow the use of the /dev/mem device to peek and poke
 *	addresses in memory on the i.MX21.
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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int main(int argc, char * argv[]) {

int i;
for (i =0 ; i <= 40; i=i+4){

	unsigned int v;
        int mtd= open("/dev/mtdblock0", O_RDONLY|O_SYNC);
        if(!mtd){
          printf("Unable to open /dev/mtdblock0.  Ensure it exists (major=31, minor=0)\n");
             return -1;
        }
        if (    lseek(mtd,  0x00000000 ,SEEK_SET) > -1){
                read(mtd, &v, 4);
         }
         else {printf("Unable to move to start of memory for /dev/mtdblock0.\n");}
         close(mtd);

	
	int fd = open("/dev/mem", O_RDWR|O_SYNC);
	volatile unsigned int *regs, *address ;
	
	unsigned long target_addr, offset, value;

	if(fd == -1)
	{
		printf("Unable to open /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	if ((argc != 2) && (argc != 3))
	{
		printf("USGAE:  dm (word access):\n");
		close(fd);
		return -1;
	}
		
	target_addr = strtoul(argv[1], 0, 0);
	target_addr = target_addr + i;
	
	regs = (unsigned int *)mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, target_addr & ~MAP_MASK);		

	printf("VA = 0x%.4x ", regs);	
		

		
	printf("0x%.4x", target_addr);
    	address = regs + ((target_addr & MAP_MASK)>>2);    	

    	
	if (argc == 3) 
	{
		value = strtoul(argv[2], 0, 0);
		*address = value; 		// perform poke command
	}
	
	printf(" = 0x%.4x\n", *address);			// display register value
	
//	printf("0x%.4x", target_addr);
//	printf(" = 0x%.4x\n", *address);			// display register value
			
	int temp = close(fd);
	if(temp == -1)
	{
		printf("Unable to close /dev/mem.  Ensure it exists (major=1, minor=1)\n");
		return -1;
	}	

	munmap(NULL, MAP_SIZE);
}
	
	return 0;
}
