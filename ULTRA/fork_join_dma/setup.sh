#!/bin/sh

if [ -f "/dev/dma_int" ]; 
   then rm /dev/dma_int
fi

/bin/mknod /dev/dma_int c 235 0

/sbin/rmmod dma_interrupt
/sbin/insmod dma_interrupt.ko


#/bin/pm 0x43c00000 0x55 > /dev/null
#sleep 1.0
#/bin/pm 0x43c00000 0xaa > /dev/null
#sleep 1.0
#cat /proc/interrupts | grep gpio

#while (ls > /dev/null) do ./intr_latency.exe; cat /proc/interrupts | grep gpio; done

