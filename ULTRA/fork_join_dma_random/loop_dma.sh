#!/bin/sh

/bin/pm 0x43c0000c 0x0 > /dev/null

while (ls > /dev/null  ) do
/bin/smb 0x43c00000 0x7 0x1  > /dev/null ;
./dma_test 1023 1023;
/bin/dm 0x43c00000 4  > /dev/null ;
/bin/smb 0x43c00000 0x7 0x0 > /dev/null ;
cat /proc/interrupts | grep cdma-controller
done
