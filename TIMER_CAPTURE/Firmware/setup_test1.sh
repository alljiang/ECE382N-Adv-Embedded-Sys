#!/bin/sh

# if [ -f "/dev/timer_int" ]; 
#    then rm /dev/timer_int
# fi
rm /dev/timer_int
rm /dev/dma_int

/bin/mknod /dev/dma_int c 236 0

# if [ -f "/proc/timer-interrupt" ]; 
#      then /sbin/rmmod timer_interrupt
# fi
/sbin/rmmod timer_interrupt
/sbin/rmmod dma_interrupt

sleep 3
sync
    
/sbin/insmod dma_interrupt.ko


#/bin/pm 0x43c00000 0x55 > /dev/null
#sleep 1.0
#/bin/pm 0x43c00000 0xaa > /dev/null
#sleep 1.0
#cat /proc/interrupts | grep gpio

#while (ls > /dev/null) do ./test_timer 0x10 0x3; cat /proc/interrupts | grep xilinx-timer; done

