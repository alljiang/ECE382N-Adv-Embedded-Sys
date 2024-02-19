#!/bin/sh

if [ -f "/dev/dma_int" ]; 
   then rm /dev/dma_int
fi

/bin/mknod /dev/dma_int c 241 0

/sbin/rmmod dma_interrupt
/sbin/insmod dma_interrupt.ko
