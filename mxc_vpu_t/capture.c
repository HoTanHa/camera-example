/*
 * Copyright 2004-2013 Freescale Semiconductor, Inc.
 *
 * Copyright (c) 2006, Chips & Media.  All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <malloc.h>
#include <linux/videodev2.h>
#include <linux/mxcfb.h>
#include "vpu_test.h"
#include <linux/ipu.h>
#include <linux/mxc_v4l2.h>


#define TEST_BUFFER_NUM 4
#define MAX_CAPTURE_MODES   10


struct capture_testbuffer cap_buffers[TEST_BUFFER_NUM];
///./
struct v4l2_requestbuffers req;
struct capture_testbuffer buffer_img[TEST_BUFFER_NUM];

struct g2d_buf *g2d_buffers[TEST_BUFFER_NUM];

int fd_capture_v4l = 0;
int fd_display = 0;
int g_in_fmt = V4L2_PIX_FMT_NV12;
int g_in_width = 1280;
int g_in_height = 720;
int g_frame_size;
int g_g2d_fmt = G2D_NV12;
int g_display_buf_count = 0;

extern int g_display_width;
extern int g_display_height;
extern int g_display_top;
extern int g_display_left;
extern int g_display_fg;
extern int g_fb_phys;
extern int g_fb_size;
extern struct fb_var_screeninfo g_screen_info;


static int mode_sizes_buf[MAX_CAPTURE_MODES][2];/*  */

int prepare_buffers(void)
{
	int i;

	for (i = 0; i < TEST_BUFFER_NUM; i++) {
		g2d_buffers[i] = g2d_alloc(g_frame_size, 0);//alloc physical contiguous memory for source image data

		if(!g2d_buffers[i]) {
			printf("Fail to allocate physical memory for image buffer!\n");
			return -1;
		}
		cap_buffers[i].offset = g2d_buffers[i]->buf_paddr;
		cap_buffers[i].start = (unsigned char *)g2d_buffers[i]->buf_vaddr;
		cap_buffers[i].length = g_frame_size;
		//printf("g2d_buffers[%d].buf_paddr = 0x%x.\r\n", i, g2d_buffers[i]->buf_paddr);
	}

	return 0;
}

static int getCaptureMode(int width, int height) {
	int i, mode = -1;

	for (i = 0; i < MAX_CAPTURE_MODES; i++) {
		if (width == mode_sizes_buf[i][0] && height == mode_sizes_buf[i][1]) {
			mode = i;
			break;
		}
	}

	return mode;
}




int v4l_start_capturing(void)
{
	unsigned int i;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;
	//buffer_img = 
	//calloc(req.count, sizeof(*buffer_img));

	for (i = 0; i < TEST_BUFFER_NUM; i++) {
		memset(&buf, 0, sizeof (buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.length = cap_buffers[i].length;
		buf.m.userptr = (unsigned long) cap_buffers[i].offset;
		if (ioctl(fd_capture_v4l, VIDIOC_QUERYBUF, &buf) < 0) {
				printf("VIDIOC_QUERYBUF error\n");
				return -1;
		}

		// buffer_img[i].length = buf.length;
		// printf("buf.lengh: %u\r\n", buf.length);

		// buffer_img[i].start = mmap(NULL, buf.length,
		// 						PROT_READ | PROT_WRITE, MAP_SHARED,
		// 						fd_capture_v4l, buf.m.offset);

	}

	

	for (i = 0; i < TEST_BUFFER_NUM; i++) {
		memset(&buf, 0, sizeof (buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.m.offset = (unsigned int)cap_buffers[i].start;
		buf.length = cap_buffers[i].length;
		if (ioctl (fd_capture_v4l, VIDIOC_QBUF, &buf) < 0) {
			printf("VIDIOC_QBUF error\n");
			return -1;
		}

	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl (fd_capture_v4l, VIDIOC_STREAMON, &type) < 0) {
		printf("VIDIOC_STREAMON error\n");
		return -1;
	}
	return 0;
}

void
v4l_stop_capturing(void)
{
	int i;
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd_capture_v4l, VIDIOC_STREAMOFF, &type);

	for (i = 0; i < TEST_BUFFER_NUM; i++)
	{
		g2d_free(g2d_buffers[i]);
	}

	if (g_display_fg)
		ioctl(fd_display, FBIOBLANK, FB_BLANK_NORMAL);

	close(fd_display);
	close(fd_capture_v4l);
	fd_capture_v4l = -1;
}

int v4l_capture_setup(struct encode *enc, int width, int height, int fps)
{
	char v4l_capture_dev[80], node[8];
	struct v4l2_input input;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	// struct v4l2_requestbuffers req;
	struct v4l2_frmsizeenum fsize;
	struct v4l2_control ctl;
	struct v4l2_streamparm parm = {0};
	unsigned int min, i, mode;
	int input_path = 1;

	if (fd_capture_v4l > 0) {
		warn_msg("capture device already opened\n");
		return -1;
	}

	g_display_fg = enc->cmdl->display_fg;
	g_display_width = enc->cmdl->display_width;
	g_display_height = enc->cmdl->display_height;
	g_display_top = enc->cmdl->display_top;
	g_display_left = enc->cmdl->display_left;
	g_frame_size = width * height * 2;
	printf("v4l_capture_setup: display fg = %d, left = %d, top = %d, width = %d, height = %d.\r\n", g_display_fg, g_display_left, g_display_top, g_display_width, g_display_height);

	sprintf(node, "%d", enc->cmdl->video_node_capture);
	strcpy(v4l_capture_dev, "/dev/video");
	strcat(v4l_capture_dev, node);

	fd_display = fb_display_setup();

	if ((fd_capture_v4l = open(v4l_capture_dev, O_RDWR, 0)) < 0) {
		err_msg("Unable to open %s\n", v4l_capture_dev);
		return -1;
	}

	if (prepare_buffers() < 0) {
		printf("prepare_output failed\n");
		return -1;
	}

	memset(mode_sizes_buf, 0, sizeof(mode_sizes_buf));
	for (i = 0; i < MAX_CAPTURE_MODES; i++) {
		fsize.index = i;
		if (ioctl(fd_capture_v4l, VIDIOC_ENUM_FRAMESIZES, &fsize))
			break;
		else {
			mode_sizes_buf[i][0] = fsize.discrete.width;
			mode_sizes_buf[i][1] = fsize.discrete.height;
		}
	}

	if (ioctl (fd_capture_v4l, VIDIOC_QUERYCAP, &cap) < 0) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n",
					v4l_capture_dev);
			return -1;
		} else {
			fprintf (stderr, "%s isn not V4L device,unknow error\n",
			v4l_capture_dev);
			return -1;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is no video capture device\n",
			v4l_capture_dev);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf (stderr, "%s does not support streaming i/o\n",
			v4l_capture_dev);
		return -1;
	}

	input.index = 0;
	while (ioctl(fd_capture_v4l, VIDIOC_ENUMINPUT, &input) >= 0)
	{
		input.index += 1;
	}

	fmtdesc.index = 0;
	while (ioctl(fd_capture_v4l, VIDIOC_ENUM_FMT, &fmtdesc) >= 0)
	{
		fmtdesc.index += 1;
	}

	ctl.id = V4L2_CID_PRIVATE_BASE;
	ctl.value = 0;
	if (ioctl(fd_capture_v4l, VIDIOC_S_CTRL, &ctl) < 0) {
		fprintf (stderr, "%s set control failed.\n",
			v4l_capture_dev);
		return -1;
	}

	mode = getCaptureMode(width, height);
	if (mode == -1) {
		printf("Not support the resolution in camera\n");
		return -1;
	}
	printf("sensor frame size is %dx%d\n", mode_sizes_buf[mode][0],
			mode_sizes_buf[mode][1]);

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = fps;
	parm.parm.capture.capturemode = mode;
	if (ioctl(fd_capture_v4l, VIDIOC_S_PARM, &parm) < 0) {
		fprintf (stderr, "%s set frame rate failed.\n",
			v4l_capture_dev);
		return -1;
	}

	input_path = 1;
	if (ioctl(fd_capture_v4l, VIDIOC_S_INPUT, &input_path) < 0) {
		fprintf (stderr, "%s VIDIOC_S_INPUT failed \n",
			v4l_capture_dev);
		return -1;
	}

	/* Select video input, video standard and tune here. */
	memset(&fmt, 0, sizeof(fmt));
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = width;
	fmt.fmt.pix.height      = height;
	fmt.fmt.pix.pixelformat = g_in_fmt;
	fmt.fmt.pix.field       = V4L2_FIELD_ANY;
	if (ioctl (fd_capture_v4l, VIDIOC_S_FMT, &fmt) < 0){
		fprintf (stderr, "%s iformat not supported \n",
			v4l_capture_dev);
		return -1;
	}
	printf("DEbug:..pixelformat......:%d\r\n", g_in_fmt);

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
		return -1;
	}

	g_in_width = fmt.fmt.pix.width;
	g_in_height = fmt.fmt.pix.height;
	printf("g_in_width = %d, g_in_height = %d.\r\n", g_in_width, g_in_height);

	memset(&req, 0, sizeof (req));
	req.count = TEST_BUFFER_NUM;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	if (ioctl (fd_capture_v4l, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
					 "memory mapping\n", v4l_capture_dev);
			return -1;
		} else {
			fprintf (stderr, "%s does not support "
					 "memory mapping, unknow error\n", v4l_capture_dev);
			return -1;
		}
	}

	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n",
			 v4l_capture_dev);
		return -1;
	}

	return 0;
}
//// dung ioctl de lay du lieu capture (cho lay tung frame)
int
v4l_get_capture_data(struct v4l2_buffer *buf)
{
	memset(buf, 0, sizeof(struct v4l2_buffer));
	buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf->memory = V4L2_MEMORY_USERPTR;
	if (ioctl(fd_capture_v4l, VIDIOC_DQBUF, buf) < 0) {
		err_msg("VIDIOC_DQBUF failed\n");
		return -1;
	}

	return 0;
}

void
v4l_put_capture_data(struct v4l2_buffer *buf)
{
	ioctl(fd_capture_v4l, VIDIOC_QBUF, buf);
}

void g2d_render_capture_data(int index)
{
	int fb_phys;

//qiang_debug add start
/*
	int total_time;
	struct timeval tv_start, tv_current;

	gettimeofday(&tv_start, 0);
*/
//qiang_debug add end

	if(g_display_fg) {
		g_display_buf_count ++;
		if(g_display_buf_count >= 3)
			g_display_buf_count = 0;
		fb_phys = g_fb_phys + g_display_buf_count * g_fb_size;
		draw_image_to_framebuffer(g2d_buffers[index], g_in_width, g_in_height, g_g2d_fmt, &g_screen_info, 0, 0, g_display_width, g_display_height, 0, G2D_ROTATION_0, fb_phys);

		g_screen_info.xoffset = 0;
		g_screen_info.yoffset = g_display_buf_count * g_display_height;
		if (ioctl(fd_display, FBIOPAN_DISPLAY, &g_screen_info) < 0) {
			printf("FBIOPAN_DISPLAY failed, index = %d\n", index);
		}
	} else {
		draw_image_to_framebuffer(g2d_buffers[index], g_in_width, g_in_height, g_g2d_fmt, &g_screen_info, g_display_left, g_display_top, g_display_width, g_display_height, 0, G2D_ROTATION_0, g_fb_phys);
	}

//qiang_debug add start
/*
	gettimeofday(&tv_current, 0);
	total_time = (tv_current.tv_sec - tv_start.tv_sec) * 1000000L;
	total_time += tv_current.tv_usec - tv_start.tv_usec;
	printf("g2d time = %u us\n", total_time);
*/
//qiang_debug add end
}

