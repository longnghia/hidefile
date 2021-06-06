KDIR = /lib/modules/`uname -r`/build
one:
	make -C $(KDIR) M=`pwd`
	sudo insmod hidefile.ko
	sudo sh -c "echo /home/long/hidefile/test/test/testfile>/dev/hidefile"
all:
	make -C $(KDIR) M=`pwd`
clean:
	make -C $(KDIR) M=`pwd` clean
in: 
	sudo insmod hidefile.ko
out: 
	sudo rmmod hidefile.ko
	sudo dmesg -c
check:
	lsmod | grep hidefile
mess:
	sudo dmesg
hide:
	sudo sh -c "echo /home/long/hidefile/test/test/testfile>/dev/hidefile"
read:
	cat /home/long/hidefile/test/test/testfile
# clean:
# 	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

