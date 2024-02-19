#!/bin/sh

#/bin/pm 0x43c0000c 0x0

while (ls > /dev/null  ) do
#/bin/smb 0x43c00000 0x7 0x1;
./test_dma 1023 1023;
#/bin/dm 0x43c00000 4;
#/bin/smb 0x43c00000 0x7 0x0;
cat /proc/interrupts | grep cdma-controller
done
