TOOL_DIR=$(OECORE_NATIVE_SYSROOT)/usr/bin/aarch64-poky-linux
ROOTFS_DIR= $(SDKTARGETSYSROOT)
CC=$(TOOL_DIR)/aarch64-poky-linux-gcc --sysroot=$(ROOTFS_DIR)
LD=$(TOOL_DIR)/aarch64-poky-linux-ld --sysroot=$(ROOTFS_DIR)

CFLAGS = -march=armv8-a -static -I$(ROOTFS_DIR)/usr/include
#  -mfloat-abi=hard -mfpu=neon -mtune=cortex-a7 

LFLAGS += -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -L $(ROOTFS_DIR)/lib -L $(ROOTFS_DIR)/usr/lib -lg2d -lpthread

OBJS = tvin_imx8.o

TARGET = tvin_imx8.out

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJS)  $(LFLAGS)

clean:
	rm -f *.o *.out
	
