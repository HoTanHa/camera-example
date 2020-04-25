TOOL_DIR=/home/hotanha/opt/fsl-imx-xwayland/4.14-sumo/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi
ROOTFS_DIR=/home/hotanha/opt/fsl-imx-xwayland/4.14-sumo/sysroots/cortexa9hf-neon-poky-linux-gnueabi
CC=$(TOOL_DIR)/arm-poky-linux-gnueabi-gcc --sysroot=$(ROOTFS_DIR)
LD=$(TOOL_DIR)/arm-poky-linux-gnueabi-ld --sysroot=$(ROOTFS_DIR)


CFLAGS= -march=armv7-a -mfloat-abi=hard -mfpu=neon -mtune=cortex-a7 -static -I$(ROOTFS_DIR)
# -o2 -Wall `pkg-config --cflags --libs libv4l2` -I$(SDKTARGETSYSROOT)/usr/include
LFLAGS+= -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed `pkg-config --cflags --libs libv4l2` -lv4l1 -lv4l2 -L $(ROOTFS_DIR)/lib -L $(ROOTFS_DIR)/usr/lib

#CC=gcc
#LD=ld
#CFLAGS = -o2 -Wall `pkg-config --cflags --libs libv4l2` -I$(SDKTARGETSYSROOT)/usr/include
#LFLAGS +=  -lv4l1 -lv4l2 -L $(ROOTFS_DIR)/lib -L $(ROOTFS_DIR)/usr/lib

# OBJS = v4l2-grab.o
# $(OBJS):
# 	$(CC) -c $*.c $(CFLAGS)  -o $(OBJS) $(LFLAGS)

OBJS = v4l2-capture.o
TARGET = capture.out
$(TARGET): $(OBJS)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJS) $(LFLAGS)
clean:
	rm -f *.o *.out
	
#	/opt/fsl-imx-xwayland/4.14-sumo/sysroots/cortexa9hf-neon-poky-linux-gnueabi/lib