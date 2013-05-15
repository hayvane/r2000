DEFAULT_TOOLCHAIN_REV=4.5.1
INSTALL_DIR=/home/hayvane/r2000
TOOLCHAIN_DIR=/opt/FriendlyARM/toolschain
EXTENT_DIR=/mnt/hgfs/share/
ROOT_FS=/home/rootfs_qtopia_qt4


ifdef R
	CC = $(TOOLCHAIN_DIR)/$(R)/bin/arm-linux-gcc
else
	CC = $(TOOLCHAIN_DIR)/$(DEFAULT_TOOLCHAIN_REV)/bin/arm-linux-gcc
	ifdef ARCH
		ifeq ("$(ARCH)", "x86")
			CC = gcc
		endif
   	endif
endif


CCFLAGS = -O2
LDFLAGS = -lpthread
OBJECTS=main.o
EXPORT_NAME=r2000_test

$(EXPORT_NAME):$(OBJECTS)
	$(CC) $(LDFLAGS) -o $(EXPORT_NAME) $(OBJECTS)
	cp -f $(EXPORT_NAME) $(INSTALL_DIR)
	cp -f $(EXPORT_NAME) $(EXTENT_DIR)
	cp -f $(EXPORT_NAME) $(ROOT_FS)

%.o : %.c include.h
	$(CC) $(CCFLAGS) -c $<


.PHONY: clean
clean:
	rm -f *.o
	rm -f $(EXPORT_NAME) *.out
