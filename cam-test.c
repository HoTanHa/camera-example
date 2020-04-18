#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h> /* getopt_long() */
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

int fd_dev_video;
char *dev_name = "/dev/video0";

static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
	int r;

	do
	{
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

int main(int argc, char **argv)
{
	//// OPEN DEVICE /dev/video0
	struct stat st;

	if (-1 == stat(dev_name, &st))
	{
		fprintf(stderr, "Cannot identify '%s': %d, %s\r\n",
				dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode))
	{
		fprintf(stderr, "%s is no devicen\r\n", dev_name);
		exit(EXIT_FAILURE);
	}

	fd_dev_video = open(dev_name, O_RDWR | O_NONBLOCK, 0);
	if (fd_dev_video < 0)
	{
		perror("Cannot open device");
		exit(EXIT_FAILURE);
	}
	printf("Open %s success!!!\r\n", dev_name);

	//----INIT DEVICE

	struct v4l2_capability cap;
	// struct v4l2_cropcap cropcap;
	// struct v4l2_crop crop;
	// struct v4l2_format fmt;
	// unsigned int min;

	if (-1 == xioctl(fd_dev_video, VIDIOC_QUERYCAP, &cap))
	{
		if (EINVAL == errno)
		{
			fprintf(stderr, "%s is no V4L2 device\\n",
					dev_name);
			exit(EXIT_FAILURE);
		}
		else
		{
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		fprintf(stderr, "%s is no video capture device\\n", dev_name);
		exit(EXIT_FAILURE);
	}


	printf("Close divice !! \r\n");
	close(fd_dev_video);
}
