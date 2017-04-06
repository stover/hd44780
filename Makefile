obj-m += hd44780.o

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dev:
	/usr/bin/sudo /bin/mknod -m 666 /dev/hd44780 c 222 0

clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
