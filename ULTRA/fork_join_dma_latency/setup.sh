#!/bin/sh

if [ -f "/dev/dma_int" ]; 
   then rm /dev/dma_int
fi

/bin/mknod /dev/dma_int c 235 0

if [ -f "/proc/dma-interrupt" ]; 
     then /sbin/rmmod dma_interrupt
fi
sleep 3
sync
    
/sbin/insmod dma_interrupt.ko


#/bin/pm 0x43c00000 0x55 > /dev/null
#sleep 1.0
#/bin/pm 0x43c00000 0xaa > /dev/null
#sleep 1.0
#cat /proc/interrupts | grep gpio

#while (ls > /dev/null) do ./test_dma 0x10 0x3; cat /proc/interrupts | grep xilinx-dma; done

