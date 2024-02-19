

KDIR= /usr/src/4.14.0-xilinx-v2018.3
LIBPATH=$(KDIR)/lib
INCPATH=$(KDIR)/include

CC := aarch64-linux-gnu-gcc


PWD		:= $(shell pwd)
CFLAGS=  -I$(INCPATH) -L$(LIBPATH)

//STRIP=$(PROC)-$(TYPE)-strip

pm:
	$(CC) $(CFLAGS)  pm.c  -o $@

clean:
	rm pm

