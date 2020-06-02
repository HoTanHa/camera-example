TOOL_DIR=$(OECORE_NATIVE_SYSROOT)/usr/bin/arm-poky-linux-gnueabi
ROOTFS_DIR= $(SDKTARGETSYSROOT)
CC=$(TOOL_DIR)/arm-poky-linux-gnueabi-gcc --sysroot=$(ROOTFS_DIR)
LD=$(TOOL_DIR)/arm-poky-linux-gnueabi-ld --sysroot=$(ROOTFS_DIR)


CFLAGS=  -W -Wall -O2 -DLINUX -march=armv7-a -mfloat-abi=hard -mfpu=neon -mtune=cortex-a7 -static -I$(ROOTFS_DIR) -I$(ROOTFS_DIR)/usr/include/imx -DIMX
  
LFLAGS +=  -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -L $(ROOTFS_DIR)/lib -L $(ROOTFS_DIR)/usr/lib -L $(ROOTFS_DIR)/usr/include/imx/linux -lrt -lpthread -lm

OBJS1 = signal_example.o
OBJS2 = child_process.o 
      

TARGET1 = signal_example.out
TARGET2 = child_process.out

all: $(OBJS1)  $(OBJS2)
	$(CC) $(FLAGS) -o $(TARGET1) $(OBJS1)  $(LFLAGS)
	$(CC) $(FLAGS) -o $(TARGET2) $(OBJS2)  $(LFLAGS)

clean:
	rm -f *.o *.out

