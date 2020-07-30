CC=/usr/bin/gcc 
obj-m += hd44780.o

all: module dev example 

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dev:
	/usr/bin/sudo /bin/mknod -m 666 /dev/hd44780 c 222 0

example: 
	$(CC) example.c -o example -O2 -g3

clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	/usr/bin/sudo rm -rf /dev/hd44780
	rm -rf example
