#!/bin/sh

./test_dma 2 20;
#/bin/dm 0x40000000 20;
#/bin/dm 0xFFFC0000 20;
cat /proc/interrupts | grep cdma-controller

