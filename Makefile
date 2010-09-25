obj-m	:= kugelfall.o

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)
EXTRA_CFLAGS = -I. -I/usr/realtime/include -D_FORTIFY_SOURCE=0 -ffast-math -mhard-float -I/usr/include

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -r .tmp_versions
	rm  .`basename $(obj-m) .o`.*
	rm `basename $(obj-m) .o`.o
	rm `basename $(obj-m) .o`.ko
	rm `basename $(obj-m) .o`.mod.*

