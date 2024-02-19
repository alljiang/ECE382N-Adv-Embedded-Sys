#!/bin/sh

# Enable IOMUX port for CSPI
/bin/pm 0x020e0144 0x00000001
/bin/pm 0x020e0148 0x00000001
/bin/pm 0x020e014c 0x00000001
/bin/pm 0x020e01cc 0x00000001
/bin/pm 0x020e07dc 0x00000002
/bin/pm 0x020e024c 0x00000000

# Set drive strength for SPI_SCLK
/bin/pm 0x020e0514 0x00002090

# Enable clock gating to ECSPI
/bin/pm 0x020c406c 0x00300c03

# Reset SPI
/bin/pm 0x02008008 0x01f01010

# Enable SPI
#/bin/pm 0x02008008 0x01f00af9
/bin/pm 0x02008008 0x01f002f9

/bin/pm 0x0200800c 0x00000100

# Eanble interrupts
/bin/pm 0x02008010 0x00000010

# Enable test mode
/bin/pm 0x02008020 0x80000000

# Enble GPIO_19 as a 48 MHz reference clock
/bin/pm 0x020e0220 0x00000003
#/bin/pm 0x020e05f0 0x0001b088
/bin/pm 0x020e05f0 0x000020e9
/bin/pm 0x020c4060 0x010e00c0

# Dump registers
#/bin/dm 0x02008000 0x10
#/bin/dm 0x020c4000 
#/bin/dm 0x020c406c



