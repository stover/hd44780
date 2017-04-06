/**************************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2017 Stover Babcock <stoverbabcock@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **************************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <generated/utsrelease.h>
#include <linux/gpio.h>

#include "hd44780.h"

#define HIGH	1
#define LOW	0

#define IN	0	
#define OUT	1	

#define RS      17
#define RW      27
#define E       22
#define DB4     21
#define DB5     20
#define DB6     16
#define DB7     18

int export_pins[] = { RS, RW, E, DB4, DB5, DB6, DB7 };

void pulseEnable(void)
{
        gpio_set_value(E, HIGH);
        udelay(1640);
        gpio_set_value(E, LOW);
        udelay(1640);
}

void writeByte(int value)
{
        gpio_set_value(DB4, LOW);
        gpio_set_value(DB5, LOW);
        gpio_set_value(DB6, LOW);
        gpio_set_value(DB7, LOW);

        if(value & 0x10)
                gpio_set_value(DB4, HIGH);

        if(value & 0x20)
                gpio_set_value(DB5, HIGH);

        if(value & 0x40)
                gpio_set_value(DB6, HIGH);

        if(value & 0x80)
                gpio_set_value(DB7, HIGH);

        pulseEnable();

        gpio_set_value(DB4, LOW);
        gpio_set_value(DB5, LOW);
        gpio_set_value(DB6, LOW);
        gpio_set_value(DB7, LOW);

        if(value & 0x01)
                gpio_set_value(DB4, HIGH);

        if(value & 0x02)
                gpio_set_value(DB5, HIGH);

        if(value & 0x04)
                gpio_set_value(DB6, HIGH);

        if(value & 0x08)
                gpio_set_value(DB7, HIGH);

        pulseEnable();
}

void writeCommand(int value)
{
        gpio_set_value (RS, LOW);
        writeByte(value);
        return;
}

void writeData(int value)
{
        gpio_set_value(RS,HIGH);
        writeByte(value);
        return;
}

void writeString(char *str)
{
        while(*str !='\0') {
                writeData(*str);
                str++;
        }

        return;
}

void gpio_init(void)
{
	int i=0;

	gpio_set_value(E, HIGH); 	
	gpio_set_value(RS, LOW);
	gpio_set_value(RW, LOW);

	udelay(1000);

	for(i=0;i<3;i++) {
                writeByte(0x3);
                udelay(1640);
        }

        writeCommand(0x2);
        writeCommand(0x28);
        writeCommand(0x0C);
        writeCommand(0x01);
        udelay(40);

	return;
}


static char hd44780_data[20]="\0";

struct file_operations my_fops={
        open: hd44780_open,
        read: hd44780_read,
        write: hd44780_write,
        release: hd44780_release,
};

int hd44780_open(struct inode *inode,struct file *filep)
{
        return 0;
}

int hd44780_release(struct inode *inode,struct file *filep)
{
        return 0;
}

ssize_t hd44780_read(struct file *filep,char *buff,size_t count,loff_t *offp )
{
	if( copy_to_user(buff, hd44780_data, count) !=0) {
                printk(KERN_INFO "Kernel -> userspace copy failed!\n" );
		return 0;
	}

        return strlen(hd44780_data);
}

ssize_t hd44780_write(struct file *filep,const char *buff,size_t count,loff_t *offp )
{
	memset(hd44780_data, '\0', 20);

        if ( copy_from_user(hd44780_data, buff, count) != 0 ) {
                printk(KERN_INFO  "Userspace -> kernel copy failed!\n" );
		return 0;
	}

	writeCommand(0x01);
	udelay(40);

	writeString(hd44780_data);

        return count;
}

static int __init start_module(void)
{
 int i=0,j=0;

 for(j=0;j<4;j++) {
  for(i=0;i<=6;i++) {
   if(!gpio_is_valid(export_pins[i])) {
      printk(KERN_INFO "Invalid gpio pin %d\n", export_pins[i]);
      //return -ENODEV;
    }
   }
 }
 
  for(j=0;j<4;j++) {
   for(i=0;i<=6;i++) {
    if(!gpio_request(export_pins[i], "sysfs")) {
	printk(KERN_INFO "Error requesting gpio pin %d\n", export_pins[i]);
	//return -ENODEV;
    }
   } 
  }

  for(j=0;j<4;j++) {
   for(i=0;i<=6;i++) {
    if(!gpio_direction_output(export_pins[i], OUT)) {
	printk(KERN_INFO "Error setting gpio pin %d direction\n", export_pins[i]);
	//return -ENODEV;
    }
   }
  }


 for(j=0;j<4;j++) {
  for(i=0;i<=6;i++) {
   if(!gpio_export(export_pins[i], false)) {
	printk(KERN_INFO "Error exporting gpio pin %d\n", export_pins[i]);
	return -ENODEV;
   }   
  } 
 }

  gpio_init();

  writeString("HD44780 LCD driver.");

  if(register_chrdev(222, "hd44780", &my_fops)) {
	printk(KERN_INFO "Failed to register /dev/hd44780");
  }

  return 0;
}


/* Cleanup - undo whatever init_module did */
static void __exit end_module(void)
{
	writeCommand(0x01);

	unregister_chrdev(222, "hd44780");
	return;
}

module_init(start_module);
module_exit(end_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stover Babcock <stoverbabcock@gmail.com>");
MODULE_DESCRIPTION("HD44780 LCD driver.");
MODULE_VERSION("0.1");
