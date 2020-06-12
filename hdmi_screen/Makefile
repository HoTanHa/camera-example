# TOOL_DIR=$(OECORE_NATIVE_SYSROOT)/usr/bin/aarch64-poky-linux
# ROOTFS_DIR= $(SDKTARGETSYSROOT)
# CC=$(TOOL_DIR)/aarch64-poky-linux-gcc --sysroot=$(ROOTFS_DIR)
# LD=$(TOOL_DIR)/aarch64-poky-linux-ld --sysroot=$(ROOTFS_DIR)

# CFLAGS = -march=armv8-a -static -I$(ROOTFS_DIR)/usr/include
# LFLAGS += -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -L $(ROOTFS_DIR)/lib -L $(ROOTFS_DIR)/usr/lib -L $(ROOTFS_DIR)/usr/include 

TOOL_DIR=$(OECORE_NATIVE_SYSROOT)/usr/bin/arm-poky-linux-gnueabi
ROOTFS_DIR= $(SDKTARGETSYSROOT)
CC=$(TOOL_DIR)/arm-poky-linux-gnueabi-gcc --sysroot=$(ROOTFS_DIR)
LD=$(TOOL_DIR)/arm-poky-linux-gnueabi-ld --sysroot=$(ROOTFS_DIR)

CFLAGS=  -W -Wall -O2 -DLINUX -march=armv7-a -mfloat-abi=hard -mfpu=neon -mtune=cortex-a7 -static -I$(ROOTFS_DIR) -I$(ROOTFS_DIR)/usr/include/imx  -I../mp4v2/include 
LFLAGS +=  -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -L $(ROOTFS_DIR)/lib -L $(ROOTFS_DIR)/usr/lib -L $(ROOTFS_DIR)/usr/include/imx/linux  -L ../mp4v2/build/.libs -lg2d -lvpu -lipu -lrt -lpthread -lm 

OBJS = screen.o

TARGET = screen.out

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJS)  $(LFLAGS)

clean:
	rm -f *.o *.out
	
