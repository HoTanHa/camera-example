TOOL_DIR=/home/hotanha/opt/fsl-imx-xwayland/4.14-sumo/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi
ROOTFS_DIR=/home/hotanha/opt/fsl-imx-xwayland/4.14-sumo/sysroots/cortexa9hf-neon-poky-linux-gnueabi
CC=$(TOOL_DIR)/arm-poky-linux-gnueabi-g++ --sysroot=$(ROOTFS_DIR)
LD=$(TOOL_DIR)/arm-poky-linux-gnueabi-ld --sysroot=$(ROOTFS_DIR)

CFLAGS= -march=armv7-a -mfloat-abi=hard -mfpu=neon -mtune=cortex-a7 -static -I$(ROOTFS_DIR) -I$(ROOTFS_DIR)/usr/include -I$(ROOTFS_DIR)/usr/include/opencv4
# -o2 -Wall `pkg-config --cflags --libs libv4l2` -I$(SDKTARGETSYSROOT)/usr/include

LFLAGS+= -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -L $(ROOTFS_DIR)/lib -L $(ROOTFS_DIR)/usr/lib -lopencv_core -lopencv_imgproc -lopencv_objdetect -lopencv_highgui -lopencv_videoio

OBJS = opencv.o

TARGET = opencv.out
$(TARGET): $(OBJS)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJS) $(LFLAGS)

# $(OBJS):
# 	$(CC) -c $*.c $(CFLAGS) -o $(OBJS) $(LFLAGS)

clean:
	rm -f *.o *.out *.bin
	
	