
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <i2c/smbus.h>
extern __s32 i2c_smbus_read_byte_data(int file, __u8 command);


int main(void)
{

    int fd = open("/dev/i2c-1", O_RDWR);
    if (fd < 0)
    {
        printf("open i2c fail.\r\n");
        exit(0);
    }
    else
    {
        printf("Open device success!!\r\n");
    }
    char address = 0x44;

    if (ioctl(fd, I2C_SLAVE_FORCE, address) < 0)
    //if (ioctl(fd, I2C_SLAVE, address) < 0)
    {
        printf("ioctl Slave fail..\r\n");
        exit(1);
    }
    else
    {
        printf("i2cSuccess.\r\n");
    }

    // unsigned char inbuf, outbuf;
    // struct i2c_rdwr_ioctl_data packets;
    // struct i2c_msg messages[2];
    // outbuf = 0x03;
    // messages[0].addr  = address;
    // messages[0].flags = 0;
    // messages[0].len   = sizeof(outbuf);
    // messages[0].buf   = &outbuf;

    // messages[1].addr  = address;
    // messages[1].flags = I2C_M_RD;
    // messages[1].len   = sizeof(inbuf);
    // messages[1].buf   = &inbuf;

    // packets.msgs      = messages;
    // packets.nmsgs     = 2;
    // if(ioctl(fd, I2C_RDWR, &packets) < 0) {
    //     return 0;
    // }
    // printf("Value of register %02x :  %02x \r\n", outbuf, inbuf);

    __u8 reg = 0x01; /* Device register to access */
    __s32 res;
    char buf[10];

    // /* Using SMBus commands */
    res = i2c_smbus_read_byte_data(fd, reg);
    if (res < 0)
    {
        /* ERROR HANDLING: i2c transaction failed */
        printf("READ Fail\r\n");
        exit(0);
    }
    else
    {
        /* res contains the read word */
        printf("Read Success %d", res);
    }

    /*
   * Using I2C Write, equivalent of
   * i2c_smbus_write_word_data(file, reg, 0x6543)
   */
    //buf[0] = reg;
    // buf[1] = 0x43;
    // buf[2] = 0x65;
    // if (write(file, buf, 3) != 3)
    // {
    //     /* ERROR HANDLING: i2c transaction failed */
    // }

    /* Using I2C Read, equivalent of i2c_smbus_read_byte(file) */
    sleep(1);
    if (read(fd, buf, 1) != 1)
    {
        /* ERROR HANDLING: i2c transaction failed */
        printf("READ Fail\r\n");
        exit(0);
    }
    else
    {
        /* buf[0] contains the read byte */
         printf("Read Success %02x\r\n", buf[0]);
    }

    sleep(1);
    return 0;
}