KDIR = /lib/modules/`uname -r`/build
all:
	make -C $(KDIR) M=`pwd`
in: 
	sudo insmod hidefile.ko
out: 
	sudo rmmod hidefile.ko
info:
	modinfo hidefile.ko
clean:
	make -C $(KDIR) M=`pwd` clean
check:
	lsmod | grep hidefile
