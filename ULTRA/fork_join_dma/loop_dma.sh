#!/bin/sh

while (ls > /dev/null  ) do
#/bin/dm 0x02008000
/usr/bin/pm 0x02008004 0x54887881 > /dev/null
sleep 1.5
#/bin/dm 0x02008000
#/bin/dm 0x02008000
/usr/bin/pm 0x02008004 0xaaaaaaaa > /dev/null
sleep 1.5
done

