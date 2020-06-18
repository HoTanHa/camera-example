/*
 * Copyright 2007-2015 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file mxc_v4l2_tvin.c
 *
 * @brief Mxc TVIN For Linux 2 driver test application
 *
 */

/*=======================================================================
										INCLUDE FILES
=======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include "mxcfb.h"
#include "g2d.h"
#include <signal.h>

#define info_msg(fmt, ...)                                               \
	do                                                                   \
	{                                                                    \
		printf("[INFO]\t%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
	} while (0)
#define warn_msg(fmt, ...)                                               \
	do                                                                   \
	{                                                                    \
		printf("[WARN]\t%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#define G2D_CACHEABLE 0

#define TFAIL -1
#define TPASS 0

#define NUMBER_BUFFERS 4

char v4l_capture_dev[100] = "/dev/video0";

char fb_display_dev[100] = "/dev/fb0";
char fb_display_bg_dev[100] = "/dev/fb0";
int fd_capture_v4l = 0;
int fd_fb_display = 0;
unsigned char *g_fb_display = NULL;
int g_input = 1;
int g_display_num_buffers = 3;
int g_capture_num_buffers = NUMBER_BUFFERS;
int g_in_width = 0;
int g_in_height = 0;
int g_in_fmt = V4L2_PIX_FMT_UYVY;
int g_display_width = 0;
int g_display_height = 0;
int g_display_top = 0;
int g_display_left = 0;
int g_display_fmt = V4L2_PIX_FMT_UYVY;
int g_display_base_phy;
int g_display_size;
int g_display_fg = 1;
int g_display_id = 1;
struct fb_var_screeninfo g_screen_info;
int g_frame_count = 0x7FFFFFFF;
int g_frame_size;
// bool g_g2d_render = 0;
int g_g2d_render = 1;

int g_g2d_fmt;
int g_mem_type = V4L2_MEMORY_USERPTR;

struct testbuffer
{
	unsigned char *start;
	size_t offset;
	unsigned int length;
};

/*test--------------------------------------------------***/
int quitflag =0;
sigset_t sigset;
int stt_cam;
struct v4l2_format fmt;
struct v4l2_requestbuffers req;
unsigned char *buffers;

struct testbuffer capture_v4l2_buffer[NUMBER_BUFFERS];
struct g2d_buf *g2d_v4l2_buffers[NUMBER_BUFFERS];

struct testbuffer capture_buffers[NUMBER_BUFFERS * 4];
struct g2d_buf *g2d_buffers[NUMBER_BUFFERS * 4];

unsigned char clamp(double value)
{
	int more = (int)value;
	// if(more){
	//     int sign = !(more >> 7);
	//     return sign * 0xff;
	// }
	int abc = more < 0 ? 0 : (more > 0xff ? 0xff : more);
	return abc;
}




static int signal_thread(void *arg)
{
	int sig;

	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	while (1)
	{
		sigwait(&sigset, &sig);
		if (sig == SIGINT)
		{
			warn_msg("Ctrl-C received\n");
			quitflag = 1;
		}
		else if (sig==SIGUSR1){
			
		}
		else if (sig==SIGUSR2){
		}
		else
		{
			warn_msg("Unknown signal. Still exiting\n");
			quitflag =1;
		}
		
		if (quitflag) break;
	}
	
	// pthread_exit(0);
	return 0;
}

static void draw_image_to_framebuffer(struct g2d_buf *buf, int img_width, int img_height, int img_format,
									  struct fb_var_screeninfo *screen_info, int left, int top, int to_width, int to_height, int set_alpha, int rotation)
{
	struct g2d_surface src, dst;
	void *g2dHandle;

	if (((left + to_width) > (int)screen_info->xres) || ((top + to_height) > (int)screen_info->yres))
	{
		printf("Bad display image dimensions!\n");
		return;
	}

#if G2D_CACHEABLE
	g2d_cache_op(buf, G2D_CACHE_FLUSH);
#endif

	if (g2d_open(&g2dHandle) == -1 || g2dHandle == NULL)
	{
		printf("Fail to open g2d device!\n");
		g2d_free(buf);
		return;
	}

	/*
	NOTE: in this example, all the test image data meet with the alignment requirement.
	Thus, in your code, you need to pay attention on that.

	Pixel buffer address alignment requirement,
	RGB/BGR:  pixel data in planes [0] with 16bytes alignment,
	NV12/NV16:  Y in planes [0], UV in planes [1], with 64bytes alignment,
	I420:    Y in planes [0], U in planes [1], V in planes [2], with 64 bytes alignment,
	YV12:  Y in planes [0], V in planes [1], U in planes [2], with 64 bytes alignment,
	NV21/NV61:  Y in planes [0], VU in planes [1], with 64bytes alignment,
	YUYV/YVYU/UYVY/VYUY:  in planes[0], buffer address is with 16bytes alignment.
*/

	src.format = img_format;
	switch (src.format)
	{
	case G2D_RGB565:
	case G2D_RGBA8888:
	case G2D_RGBX8888:
	case G2D_BGRA8888:
	case G2D_BGRX8888:
	case G2D_BGR565:
	case G2D_YUYV:
	case G2D_UYVY:
		src.planes[0] = buf->buf_paddr;
		break;
	case G2D_NV12:
		src.planes[0] = buf->buf_paddr;
		src.planes[1] = buf->buf_paddr + img_width * img_height;
		break;
	case G2D_I420:
		src.planes[0] = buf->buf_paddr;
		src.planes[1] = buf->buf_paddr + img_width * img_height;
		src.planes[2] = src.planes[1] + img_width * img_height / 4;
		break;
	case G2D_YV12:
		src.planes[0] = buf->buf_paddr;
		src.planes[2] = buf->buf_paddr + img_width * img_height;
		src.planes[1] = src.planes[2] + img_width * img_height / 4;
		break;
	case G2D_NV16:
		src.planes[0] = buf->buf_paddr;
		src.planes[1] = buf->buf_paddr + img_width * img_height;
		break;
	default:
		printf("Unsupport image format in the example code\n");
		return;
	}

	src.left = 0;
	src.top = 0;
	src.right = img_width;
	src.bottom = img_height;
	src.stride = img_width;
	src.width = img_width;
	src.height = img_height;
	src.rot = G2D_ROTATION_0;

	dst.planes[0] = g_display_base_phy;
	dst.left = left;
	dst.top = top;
	dst.right = left + to_width;
	dst.bottom = top + to_height;
	dst.stride = screen_info->xres;
	dst.width = screen_info->xres;
	dst.height = screen_info->yres;
	dst.rot = rotation;
	dst.format = screen_info->bits_per_pixel == 16 ? G2D_RGB565 : (screen_info->red.offset == 0 ? G2D_RGBA8888 : G2D_BGRA8888);

	if (set_alpha)
	{
		src.blendfunc = G2D_ONE;
		dst.blendfunc = G2D_ONE_MINUS_SRC_ALPHA;

		src.global_alpha = 0x80;
		dst.global_alpha = 0xff;

		g2d_enable(g2dHandle, G2D_BLEND);
		g2d_enable(g2dHandle, G2D_GLOBAL_ALPHA);
	}

	g2d_blit(g2dHandle, &src, &dst);
	g2d_finish(g2dHandle);

	if (set_alpha)
	{
		g2d_disable(g2dHandle, G2D_GLOBAL_ALPHA);
		g2d_disable(g2dHandle, G2D_BLEND);
	}

	g2d_close(g2dHandle);
}

int start_capturing(void)
{
	int i;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	for (i = 0; i < g_capture_num_buffers; i++)
	{
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.length = capture_buffers[i].length;
		buf.m.userptr = (unsigned long)capture_buffers[i].offset;

		if (ioctl(fd_capture_v4l, VIDIOC_QUERYBUF, &buf) < 0)
		{
			printf("VIDIOC_QUERYBUF error\n");
			return TFAIL;
		}
	}

	for (i = 0; i < g_capture_num_buffers; i++)
	{
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.m.offset = (unsigned int)capture_buffers[i].start;
		buf.length = capture_buffers[i].length;
		if (ioctl(fd_capture_v4l, VIDIOC_QBUF, &buf) < 0)
		{
			printf("VIDIOC_QBUF error\n");
			return TFAIL;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd_capture_v4l, VIDIOC_STREAMON, &type) < 0)
	{
		printf("VIDIOC_STREAMON error\n");
		return TFAIL;
	}
	return TPASS;
}

int prepare_g2d_buffers(void)
{
	int i;

	g_frame_size = 1282*2880*2;
	for (i = 0; i < g_capture_num_buffers; i++)
	{
		g2d_v4l2_buffers[i] = g2d_alloc(g_frame_size, 0); //alloc physical contiguous memory for source image data
		if (!g2d_v4l2_buffers[i])
		{
			printf("Fail to allocate physical memory for image buffer!\n");
			return TFAIL;
		}

		capture_v4l2_buffer[i].start = g2d_v4l2_buffers[i]->buf_vaddr;
		capture_v4l2_buffer[i].offset = g2d_v4l2_buffers[i]->buf_paddr;
		capture_v4l2_buffer[i].length = g_frame_size;
	}

	int size = 1280 * 720 * 2;
	for (i = 0; i < g_capture_num_buffers * 4; i++)
	{
		g2d_buffers[i] = g2d_alloc(size, 0); //alloc physical contiguous memory for source image data
		if (!g2d_buffers[i])
		{
			printf("Fail to allocate physical memory for image buffer!\n");
			return TFAIL;
		}

		capture_buffers[i].start = g2d_buffers[i]->buf_vaddr;
		capture_buffers[i].offset = g2d_buffers[i]->buf_paddr;
		capture_buffers[i].length = g_frame_size;
	}

	return 0;
}

int v4l_capture_setup(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	//	struct v4l2_format fmt;
	//	struct v4l2_requestbuffers req;
	struct v4l2_streamparm parm;
	v4l2_std_id id;
	unsigned int min;

	if (ioctl(fd_capture_v4l, VIDIOC_QUERYCAP, &cap) < 0)
	{
		if (EINVAL == errno)
		{
			fprintf(stderr, "%s is no V4L2 device\n",
					v4l_capture_dev);
			return TFAIL;
		}
		else
		{
			fprintf(stderr, "%s isn not V4L device,unknow error\n",
					v4l_capture_dev);
			return TFAIL;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		fprintf(stderr, "%s is no video capture device\n",
				v4l_capture_dev);
		return TFAIL;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		fprintf(stderr, "%s does not support streaming i/o\n",
				v4l_capture_dev);
		return TFAIL;
	}

	if (ioctl(fd_capture_v4l, VIDIOC_S_INPUT, &g_input) < 0)
	{
		printf("VIDIOC_S_INPUT failed\n");
		close(fd_capture_v4l);
		return TFAIL;
	}

	sleep(1); //wait for lock
	if (ioctl(fd_capture_v4l, VIDIOC_G_STD, &id) < 0)
	{
		printf("VIDIOC_G_STD failed\n");
	}
	else
	{
		ioctl(fd_capture_v4l, VIDIOC_S_STD, &id);
	}

	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd_capture_v4l, VIDIOC_CROPCAP, &cropcap) < 0)
	{
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (ioctl(fd_capture_v4l, VIDIOC_S_CROP, &crop) < 0)
		{
			switch (errno)
			{
			case EINVAL:
				/* Cropping not supported. */
				fprintf(stderr, "%s  doesn't support crop\n",
						v4l_capture_dev);
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	}
	else
	{
		/* Errors ignored. */
	}

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = 0;
	parm.parm.capture.capturemode = 0;
	if (ioctl(fd_capture_v4l, VIDIOC_S_PARM, &parm) < 0)
	{
		printf("VIDIOC_S_PARM failed\n");
		close(fd_capture_v4l);
		return TFAIL;
	}

	memset(&fmt, 0, sizeof(fmt));

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if ((g_input == 1) || (g_input == 3))
	{
		fmt.fmt.pix.width = g_in_width;
		fmt.fmt.pix.height = g_in_height;
	}
	else
	{
		fmt.fmt.pix.width = g_display_width;
		fmt.fmt.pix.height = g_display_height;
	}
	fmt.fmt.pix.pixelformat = g_in_fmt;

	fmt.fmt.pix.field = V4L2_FIELD_ANY;
	if (ioctl(fd_capture_v4l, VIDIOC_S_FMT, &fmt) < 0)
	{
		fprintf(stderr, "%s iformat not supported \n",
				v4l_capture_dev);
		return TFAIL;
	}

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;

	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	if (ioctl(fd_capture_v4l, VIDIOC_G_FMT, &fmt) < 0)
	{
		printf("VIDIOC_G_FMT failed\n");
		close(fd_capture_v4l);
		return TFAIL;
	}

	g_in_width = fmt.fmt.pix.width;
	g_in_height = fmt.fmt.pix.height;
	printf("g_in_width = %d, g_in_height = %d.\r\n", g_in_width, g_in_height);

	if (g_g2d_render)
	{
		switch (g_in_fmt)
		{
		case V4L2_PIX_FMT_RGB565:
			g_frame_size = g_in_width * g_in_height * 2;
			g_g2d_fmt = G2D_RGB565;
			break;

		case V4L2_PIX_FMT_UYVY:
			g_frame_size = g_in_width * g_in_height * 2;
			g_g2d_fmt = G2D_UYVY;
			break;

		case V4L2_PIX_FMT_YUYV:
			g_frame_size = g_in_width * g_in_height * 2;
			g_g2d_fmt = G2D_YUYV;
			break;

		case V4L2_PIX_FMT_YUV420:
			g_frame_size = g_in_width * g_in_height * 3 / 2;
			g_g2d_fmt = G2D_I420;
			break;

		case V4L2_PIX_FMT_NV12:
			g_frame_size = g_in_width * g_in_height * 3 / 2;
			g_g2d_fmt = G2D_NV12;
			break;

		default:
			printf("Unsupported format.\n");
			return TFAIL;
		}
	}

	memset(&req, 0, sizeof(req));
	req.count = g_capture_num_buffers;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = g_mem_type;
	if (ioctl(fd_capture_v4l, VIDIOC_REQBUFS, &req) < 0)
	{
		if (EINVAL == errno)
		{
			fprintf(stderr, "%s does not support memory mapping\n", v4l_capture_dev);
			return TFAIL;
		}
		else
		{
			fprintf(stderr, "%s does not support memory mapping, unknow error\n", v4l_capture_dev);
			return TFAIL;
		}
	}

	if (req.count < 2)
	{
		fprintf(stderr, "Insufficient buffer memory on %s\n",
				v4l_capture_dev);
		return TFAIL;
	}

	return TPASS;
}

int fb_display_setup(void)
{
	int fd_fb_bg = 0;
	struct mxcfb_gbl_alpha alpha;
	char node[8];
	struct fb_fix_screeninfo fb_fix;
	struct mxcfb_pos pos;

	if ((fd_fb_display = open(fb_display_dev, O_RDWR, 0)) < 0)
	{
		printf("Unable to open %s\n", fb_display_dev);
		close(fd_capture_v4l);
		return TFAIL;
	}
	

	if (ioctl(fd_fb_display, FBIOGET_VSCREENINFO, &g_screen_info) < 0)
	{
		printf("fb_display_setup FBIOGET_VSCREENINFO failed\n");
		return TFAIL;
	}

	info_msg("Screen info: %d - %d .\r\n", g_screen_info.width, g_screen_info.height);
	info_msg("Screen info: %d - %d .\r\n", g_screen_info.xres, g_screen_info.yres);
	info_msg("Screen info: %d - %d .\r\n", g_screen_info.xres_virtual, g_screen_info.yres_virtual);
	info_msg("Screen info: %d - %d - %d.\r\n", g_screen_info.colorspace, g_screen_info.right_margin, g_screen_info.upper_margin);
	info_msg("Screen info: %d - %d .\r\n", g_screen_info.xoffset, g_screen_info.yoffset);
	if (ioctl(fd_fb_display, FBIOGET_FSCREENINFO, &fb_fix) < 0)
	{
		printf("fb_display_setup FBIOGET_FSCREENINFO failed\n");
		return TFAIL;
	}
	

	printf("fb_fix.id = %s.\r\n", fb_fix.id);
	//  if ((strcmp(fb_fix.id, "DISP4 FG") == 0) || (strcmp(fb_fix.id, "DISP3 FG") == 0))
	{

		// info_msg("FB_fix.-.-.\r\n");
		// g_display_fg = 1;
		// if (g_g2d_render)
		// {
		// 	pos.x = 0;
		// 	pos.y = 0;
		// }
		
		// if (ioctl(fd_fb_display, MXCFB_SET_OVERLAY_POS, &pos) < 0)
		// {
		// 	printf("fb_display_setup MXCFB_SET_OVERLAY_POS failed\n");
		// 	return TFAIL;
		// }

		// strcpy(fb_display_bg_dev, "/dev/fb0");
		if ((fd_fb_bg = open(fb_display_bg_dev, O_RDWR)) < 0)
		{
			printf("Unable to open bg frame buffer\n");
			return TFAIL;
		}

		// /* Overlay setting */
		// alpha.alpha = 0;
		// alpha.enable = 1;
		// if (ioctl(fd_fb_bg, MXCFB_SET_GBL_ALPHA, &alpha) < 0)
		// {
		// 	printf("Set global alpha failed\n");
		// 	close(fd_fb_bg);
		// 	return TFAIL;
		// }

		if (g_g2d_render)
		{

			if( ioctl(fd_fb_bg, FBIOGET_VSCREENINFO, &g_screen_info)<0){
				info_msg("Can not get g-screen-info: %s \r\n", fb_display_bg_dev);
				return TFAIL;
			}

			// g_screen_info.yres_virtual = g_screen_info.yres * g_display_num_buffers;
			// memset(&g_screen_info, 0, sizeof(g_screen_info));
			g_screen_info.xres = 1920;
			g_screen_info.yres = 1080;
			// g_screen_info.yres_virtual = g_screen_info.yres * g_display_num_buffers;
			// g_screen_info.nonstd = G2D_BGR565;//g_display_fmt;
			g_screen_info.activate |= FB_ACTIVATE_FORCE;
			g_screen_info.vmode = FB_VMODE_MASK;
		
			if (ioctl(fd_fb_display, FBIOPUT_VSCREENINFO, &g_screen_info) < 0)
			{
				printf("fb_display_setup FBIOPUT_VSCREENINFO failed\n");
				return TFAIL;
			}
			ioctl(fd_fb_display, FBIOGET_FSCREENINFO, &fb_fix);
			ioctl(fd_fb_display, FBIOGET_VSCREENINFO, &g_screen_info);
			info_msg("Screen info: %d - %d .\r\n", g_screen_info.width, g_screen_info.height);

		}
	}
	
	

	if (ioctl(fd_fb_display, FBIOBLANK, FB_BLANK_UNBLANK)<0){
		info_msg("Can not set fb_blank_unblank");
	}

	g_display_base_phy = fb_fix.smem_start;
	printf("fb: smem_start = 0x%x, smem_len = 0x%x.\r\n", (unsigned int)fb_fix.smem_start, (unsigned int)fb_fix.smem_len);

	g_display_size = g_screen_info.xres * g_screen_info.yres * g_screen_info.bits_per_pixel / 8;
	printf("fb: frame buffer size = 0x%x bytes.\r\n", g_display_size);

	printf("fb: g_screen_info.xres = %d, g_screen_info.yres = %d.\r\n", g_screen_info.xres, g_screen_info.yres);
	printf("fb: g_display_left = %d.\r\n", g_display_left);
	printf("fb: g_display_top = %d.\r\n", g_display_top);
	printf("fb: g_display_width = %d.\r\n", g_display_width);
	printf("fb: g_display_height = %d.\r\n", g_display_height);

	return TPASS;
}

int mxc_v4l_tvin_test(void)
{
	struct v4l2_buffer capture_buf;
	int i;
	int total_time;
	struct timeval tv_start, tv_current;
	//	struct timeval fps_old, fps_current;  //qiang_debug added
	//	int fps_period;  //qiang_debug added
	int display_buf_count = 0;

	if (g_g2d_render)
	{
		if (prepare_g2d_buffers() < 0)
		{
			printf("prepare_g2d_buffers failed\n");
			return TFAIL;
		}
		else{
			info_msg("Prepare buffer success!\r\n");
		}
	}

	// if (start_capturing() < 0)
	// {
	// 	printf("start_capturing failed\n");
	// 	return TFAIL;
	// }

	gettimeofday(&tv_start, 0);
	info_msg("start time = %d s, %d us\n", (unsigned int)tv_start.tv_sec,  (unsigned int)tv_start.tv_usec);

	uint8_t x1, x2, x3, x4;
	x1 = 0; x2 = 60; x3 = 120; x4 = 180;
	i=0;
	// for (i = 0; i < 1000; i++)
	while (1)	
	{ 
		i++;
		// memset(&capture_buf, 0, sizeof(capture_buf));
		// capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		// capture_buf.memory = g_mem_type;
		// if (ioctl(fd_capture_v4l, VIDIOC_DQBUF, &capture_buf) < 0)
		// {
		// 	printf("VIDIOC_DQBUF failed.\n");
		// 	return TFAIL;
		// }

	
		if (g_g2d_render)
		{
			g_display_width = 960;
			g_display_height = 540;
			static int index =0;//= capture_buf.index;
			index++;
			if (index==4) index=0;
			memset(capture_buffers[index].start, x1, 1280*720);
			memset(capture_buffers[index+4].start, x2, 1280*720);
			memset(capture_buffers[index+8].start, x3, 1280*720);
			memset(capture_buffers[index+12].start, x4, 1280*720);
			memset(&(capture_buffers[index].start[1280*720]), 15, 1280*720);
			memset(&(capture_buffers[index+4].start[1280*720]), 72, 1280*720);
			memset(&(capture_buffers[index+8].start[1280*720]), 130, 1280*720);
			memset(&(capture_buffers[index+12].start[1280*720]), 200, 1280*720);

			g_g2d_fmt = G2D_NV12;
			draw_image_to_framebuffer(g2d_buffers[4 * 0 + index], 1280, 720, g_g2d_fmt, &g_screen_info, 0, 0, 960, 540, 0, G2D_ROTATION_0);
			// usleep(10);
			draw_image_to_framebuffer(g2d_buffers[4 * 1 + index], 1280, 720, g_g2d_fmt, &g_screen_info, 960, 0, 960, 540, 0, G2D_ROTATION_0);
			// usleep(10);
			draw_image_to_framebuffer(g2d_buffers[4 * 2 + index], 1280, 720, g_g2d_fmt, &g_screen_info, 0, 540, 960, 540, 0, G2D_ROTATION_0);
			// usleep(10);
			draw_image_to_framebuffer(g2d_buffers[4 * 3 + index], 1280, 720, g_g2d_fmt, &g_screen_info, 960, 540, 960, 540, 0, G2D_ROTATION_0);
			
		}
		// usleep(20000);
		// x1++; x2++; x3++; x4++;
		static int abc =0; 
		abc=0x000FFFFF;
		while (abc>1)
		{
		abc--;
		}
		
		printf("%02X - %02X - %02X - %02x - %d.. \r\n",x1++, x2++, x3++, x4++, display_buf_count++); 
		
		// if (ioctl(fd_capture_v4l, VIDIOC_QBUF, &capture_buf) < 0)
		// {
		// 	printf("VIDIOC_QBUF failed\n");
		// 	return TFAIL;
		// }

		if (quitflag){
			break;
		}
		
	}
	quitflag = 1;
	gettimeofday(&tv_current, 0);
	total_time = (tv_current.tv_sec - tv_start.tv_sec) * 1000000L;
	total_time += tv_current.tv_usec - tv_start.tv_usec;
	printf("total time for %u frames = %u us =  %lld fps\n", i, total_time, (i * 1000000ULL) / total_time);

	//	close(fd_still);

	return TPASS;
}

int process_cmdline(int argc, char **argv)
{
	int i, val;
	char node[8];

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-c") == 0)
		{
			g_frame_count = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-i") == 0)
		{
			g_input = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-x") == 0)
		{
			val = atoi(argv[++i]);
			stt_cam = val;
			sprintf(node, "%d", val);
			strcpy(v4l_capture_dev, "/dev/video");
			strcat(v4l_capture_dev, node);
		}
		else if (strcmp(argv[i], "-d") == 0)
		{
			val = atoi(argv[++i]);
			g_display_id = val;
			sprintf(node, "%d", val);
			strcpy(fb_display_dev, "/dev/fb");
			strcat(fb_display_dev, node);
		}
		else if (strcmp(argv[i], "-if") == 0)
		{
			i++;
			g_in_fmt = v4l2_fourcc(argv[i][0], argv[i][1], argv[i][2], argv[i][3]);
			if ((g_in_fmt != V4L2_PIX_FMT_NV12) &&
				(g_in_fmt != V4L2_PIX_FMT_RGB565) &&
				(g_in_fmt != V4L2_PIX_FMT_UYVY) &&
				(g_in_fmt != V4L2_PIX_FMT_YUYV) &&
				(g_in_fmt != V4L2_PIX_FMT_YUV420))
			{
				info_msg("Default capture format is used: UYVY\n");
				g_in_fmt = V4L2_PIX_FMT_UYVY;
			}
		}
		else if (strcmp(argv[i], "-of") == 0)
		{
			i++;
			g_display_fmt = v4l2_fourcc(argv[i][0], argv[i][1], argv[i][2], argv[i][3]);
			if ((g_display_fmt != V4L2_PIX_FMT_RGB565) &&
				(g_display_fmt != V4L2_PIX_FMT_RGB24) &&
				(g_display_fmt != V4L2_PIX_FMT_RGB32) &&
				(g_display_fmt != V4L2_PIX_FMT_BGR32) &&
				(g_display_fmt != V4L2_PIX_FMT_UYVY) &&
				(g_display_fmt != V4L2_PIX_FMT_YUYV))
			{
				printf("Default display format is used: UYVY\n");
				g_display_fmt = V4L2_PIX_FMT_UYVY;
			}
		}
		else if (strcmp(argv[i], "-help") == 0)
		{
			printf("MXC Video4Linux TVin Test\n\n"
				   "Syntax: mxc_v4l2_tvin.out\n"
				   " -i <CSI input path, 0 = CSI->IC->MEM; 1 = CSI->MEM; 2 = CSI->VDI->IC->MEM; 3 = CSI->VDI->MEM>\n"
				   " -c <capture counter> \n"
				   " -x <capture device> 0 = /dev/video0; 1 = /dev/video1 ...>\n"
				   " -d <output frame buffer> 0 = /dev/fb0; 1 = /dev/fb1 ...>\n"
				   " -if <capture format, only YU12, YUYV, UYVY and NV12 are supported> \n"
				   " -of <display format, only RGBP, RGB3, RGB4, BGR4, YUYV, and UYVY are supported> \n");
			return TFAIL;
		}
	}

	g_display_width=960; g_display_height = 540;
	if ((g_display_width == 0) || (g_display_height == 0))
	{
		printf("Zero display width or height\n");
		return TFAIL;
	}

	return TPASS;
}

int main(int argc, char **argv)
{
	int i;
	enum v4l2_buf_type type;
	pthread_t sigtid;

	if (process_cmdline(argc, argv) < 0)
	{
		return TFAIL;
	}

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);
	
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	pthread_create(&sigtid, NULL, (void *)&signal_thread, NULL);

	// if ((fd_capture_v4l = open(v4l_capture_dev, O_RDWR, 0)) < 0)
	// {
	// 	printf("Unable to open %s\n", v4l_capture_dev);
	// 	return TFAIL;
	// }

	// if (v4l_capture_setup() < 0)
	// {
	// 	printf("Setup v4l capture failed.\n");
	// 	return TFAIL;
	// }

	if (fb_display_setup() < 0)
	{
		printf("Setup fb display failed.\n");
		close(fd_capture_v4l);
		close(fd_fb_display);
		return TFAIL;
	}

	mxc_v4l_tvin_test();

	// type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	// ioctl(fd_capture_v4l, VIDIOC_STREAMOFF, &type);

	if (g_display_fg)
		ioctl(fd_fb_display, FBIOBLANK, FB_BLANK_NORMAL);

	info_msg("FINISH...............\r\n");
	if (g_g2d_render)
	{
		for (i = 0; i < g_capture_num_buffers*4; i++)
		{
			g2d_free(g2d_buffers[i]);
		}
		for (i = 0; i < g_capture_num_buffers; i++)
		{
			g2d_free(g2d_v4l2_buffers[i]);
		}
	}
	

	close(fd_capture_v4l);
	close(fd_fb_display);
	return 0;
}
