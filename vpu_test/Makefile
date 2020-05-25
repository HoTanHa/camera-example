TOOL_DIR=/home/hotanha/opt/fsl-imx-xwayland/4.14-sumo/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi
ROOTFS_DIR=/home/hotanha/opt/fsl-imx-xwayland/4.14-sumo/sysroots/cortexa9hf-neon-poky-linux-gnueabi
CC=$(TOOL_DIR)/arm-poky-linux-gnueabi-gcc --sysroot=$(ROOTFS_DIR)
LD=$(TOOL_DIR)/arm-poky-linux-gnueabi-ld --sysroot=$(ROOTFS_DIR)
LIB_DIR=/home/hotanha/opt/fsl-imx-xwayland/4.14-sumo/sysroots/x86_64-pokysdk-linux/usr/lib


CFLAGS= -D_POSIX_C_SOURCE=200809L -std=c11 -W -Wall -Wunused-parameter -O2 -DLINUX -march=armv7-a -mfloat-abi=hard -mfpu=neon -mtune=cortex-a7 -static -I$(ROOTFS_DIR) -I$(ROOTFS_DIR)/usr/include/imx
#  -pedantic  -W -Wall -Wunused-parameter 
# -D_POSIX_C_SOURCE=200809L  -std=c11 -pedantic
LFLAGS +=  -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -L $(LIB_DIR) -L $(ROOTFS_DIR)/lib -L $(ROOTFS_DIR)/usr/lib -L $(ROOTFS_DIR)/usr/include/imx/linux -L $(ROOTFS_DIR)/usr/src -lg2d -lvpu -lipu -lrt -lpthread -lm

OBJS = main.o \
      dec.o \
      enc.o \
      capture.o \
      display.o \
      fb.o \
      utils.o \
      loopback.o \
      transcode.o

TARGET = mxc_vpu_test.out

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJS)  $(LFLAGS)

clean:
	rm -f *.o *.out

