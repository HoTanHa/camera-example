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
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "vpu_test.h"
#include "vpu_jpegtable.h"
#include "g2d.h"

//////---------------------------------------
#include <sys/time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../../stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../stb/stb_image_write.h"

/* V4L2 capture buffers are obtained from here */
extern struct capture_testbuffer cap_buffers[];
int image_sizeabc = 1280 * 720 * 2;
char buff_Y[1280 * 720];
char buff_CrCb[1280 * 720];
int image_thread_wait = 0;
/* When app need to exit */
extern int quitflag;

#define FN_ENC_QP_DATA "enc_qp.log"
#define FN_ENC_SLICE_BND_DATA "enc_slice_bnd.log"
#define FN_ENC_MV_DATA "enc_mv.log"
#define FN_ENC_SLICE_DATA "enc_slice.log"
static FILE *fpEncSliceBndInfo = NULL;
static FILE *fpEncQpInfo = NULL;
static FILE *fpEncMvInfo = NULL;
static FILE *fpEncSliceInfo = NULL;

////------------------------------------------------

extern int device_video;
int fd_fb_display = 0;
// int g_display_width = 0;
// int g_display_height = 0;
// int g_display_top = 0;
// int g_display_left = 0;
// int g_display_fmt = V4L2_PIX_FMT_UYVY;
int g_display_base_phy;
int g_display_size;
int g_display_fg = 1;
int g_display_id;

struct fb_var_screeninfo g_screen_info;
int g_display_num_buffers = 3;
int g_capture_num_buffers = 4;
int g_in_width = 1280;
int g_in_height = 720;
int g_display_width = 960;
int g_display_height = 540;
int g_display_top = 0;
int g_display_left = 0;
int g_display_fmt = V4L2_PIX_FMT_NV12;
int g_g2d_fmt = G2D_NV12;
// int	g_frame_size = g_in_width * g_in_height * 3 / 2;
int g_frame_size = 1280 * 720 * 3 / 2;
extern struct g2d_buf *g2d_buffers[];
extern int screen_width;
extern int screen_height;
//------------------------------------------------------
unsigned char clamp(double value)
{
	int more = (int)value;
	int abc = more < 0 ? 0 : (more > 0xff ? 0xff : more);
	return (char)(abc & 0xff);
}
#define GOP_SIZE 25
////------------------------------------------------------------
struct display_queue
{
	int list[MAX_BUF_NUM + 1];
	int head;
	int tail;
};

struct display_queue display_q;

static pthread_mutex_t display_mutex;
static pthread_cond_t display_cond;
static int display_waiting = 0;
static int dislay_running = 0;
static inline void wait_dp_queue()
{
	pthread_mutex_lock(&display_mutex);
	display_waiting = 1;
	pthread_cond_wait(&display_cond, &display_mutex);
	pthread_mutex_unlock(&display_mutex);
}

static inline void wakeup_dp_queue()
{
	pthread_cond_signal(&display_cond);
}

static __inline int queue_dp_size(struct display_queue *dp_q)
{
	if (dp_q->tail >= dp_q->head)
		return (dp_q->tail - dp_q->head);
	else
		return ((dp_q->tail + QUEUE_SIZE) - dp_q->head);
}

static __inline int queue_dp_buf(struct display_queue *dp_q, int idx)
{
	if (((dp_q->tail + 1) % QUEUE_SIZE) == dp_q->head)
		return -1; /* queue full */
	pthread_mutex_lock(&display_mutex);
	dp_q->list[dp_q->tail] = idx;
	dp_q->tail = (dp_q->tail + 1) % QUEUE_SIZE;
	pthread_mutex_unlock(&display_mutex);
	return 0;
}

static __inline int dequeue_dp_buf(struct display_queue *dp_q)
{
	int ret;
	if (dp_q->tail == dp_q->head)
		return -1; /* queue empty */
	pthread_mutex_lock(&display_mutex);
	ret = dp_q->list[dp_q->head];
	dp_q->head = (dp_q->head + 1) % QUEUE_SIZE;
	pthread_mutex_unlock(&display_mutex);
	return ret;
}

static void draw_image_to_framebuffer_tvin(struct g2d_buf *buf, int img_width, int img_height, int img_format,
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

extern char fb_dev[];
extern char fb_dev_fg[];
int fb_display_setup_tvin(void)
{
	int fd_fb = 0;
	struct mxcfb_gbl_alpha alpha;
	char node[8];
	struct fb_fix_screeninfo fb_fix;
	struct mxcfb_pos pos;

	if ((fd_fb_display = open(fb_dev_fg, O_RDWR, 0)) < 0)
	{
		printf("Unable to open %s\n", fb_dev_fg);
		exit(1);
	}

	if (ioctl(fd_fb_display, FBIOGET_VSCREENINFO, &g_screen_info) < 0)
	{
		printf("fb_display_setup FBIOGET_VSCREENINFO failed 111\n");
		return 1;
	}

	if (ioctl(fd_fb_display, FBIOGET_FSCREENINFO, &fb_fix) < 0)
	{
		printf("fb_display_setup FBIOGET_FSCREENINFO failed 222\n");
		return 1;
	}

	printf("fb_fix.id = %s.\r\n", fb_fix.id);
	if ((strcmp(fb_fix.id, "DISP4 FG") == 0) || (strcmp(fb_fix.id, "DISP3 FG") == 0))
	{
		g_display_fg = 1;
		pos.x = 0;
		pos.y = 0;

		if (ioctl(fd_fb_display, MXCFB_SET_OVERLAY_POS, &pos) < 0)
		{
			printf("fb_display_setup MXCFB_SET_OVERLAY_POS failed\n");
			return 1;
		}

		if ((fd_fb = open(fb_dev, O_RDWR)) < 0)
		{
			printf("Unable to open bg frame buffer\n");
			return 1;
		}

		/* Overlay setting */
		alpha.alpha = 0;
		alpha.enable = 1;
		if (ioctl(fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0)
		{
			printf("Set global alpha failed\n");
			close(fd_fb);
			return 1;
		}

		ioctl(fd_fb, FBIOGET_VSCREENINFO, &g_screen_info);

		memset(&g_screen_info, 0, sizeof(g_screen_info));
		g_screen_info.xres = 1920;
		g_screen_info.yres = 1080;
		g_screen_info.yres_virtual = g_screen_info.yres * g_display_num_buffers;
		g_screen_info.nonstd = V4L2_PIX_FMT_RGB565; //g_display_fmt;
		if (ioctl(fd_fb_display, FBIOPUT_VSCREENINFO, &g_screen_info) < 0)
		{
			printf("fb_display_setup FBIOPUET_VSCREENINFO failed\n");
			return -1;
		}
		ioctl(fd_fb_display, FBIOGET_FSCREENINFO, &fb_fix);
		ioctl(fd_fb_display, FBIOGET_VSCREENINFO, &g_screen_info);
	}
	else
	{
		g_display_fg = 0;
	}

	ioctl(fd_fb_display, FBIOBLANK, FB_BLANK_UNBLANK);

	g_display_base_phy = fb_fix.smem_start;
	printf("fb: smem_start = 0x%x, smem_len = 0x%x.\r\n", (unsigned int)fb_fix.smem_start, (unsigned int)fb_fix.smem_len);

	g_display_size = g_screen_info.xres * g_screen_info.yres * g_screen_info.bits_per_pixel / 8;
	printf("fb: frame buffer size = 0x%x bytes.\r\n", g_display_size);

	printf("fb: g_screen_info.xres = %d, g_screen_info.yres = %d.\r\n", g_screen_info.xres, g_screen_info.yres);
	printf("fb: g_display_left = %d.\r\n", g_display_left);
	printf("fb: g_display_top = %d.\r\n", g_display_top);
	printf("fb: g_display_width = %d.\r\n", g_display_width);
	printf("fb: g_display_height = %d.\r\n", g_display_height);
	return fd_fb_display;
}

void SaveEncMbInfo(u8 *mbParaBuf, int size, int MbNumX, int EncNum)
{
	int i;
	if (!fpEncQpInfo)
		fpEncQpInfo = fopen(FN_ENC_QP_DATA, "w+");
	if (!fpEncQpInfo)
		return;

	if (!fpEncSliceBndInfo)
		fpEncSliceBndInfo = fopen(FN_ENC_SLICE_BND_DATA, "w+");
	if (!fpEncSliceBndInfo)
		return;

	fprintf(fpEncQpInfo, "FRAME [%1d]\n", EncNum);
	fprintf(fpEncSliceBndInfo, "FRAME [%1d]\n", EncNum);

	for (i = 0; i < size; i++)
	{
		fprintf(fpEncQpInfo, "MbAddr[%4d]: MbQs[%2d]\n", i, mbParaBuf[i] & 63);
		fprintf(fpEncSliceBndInfo, "MbAddr[%4d]: Slice Boundary Flag[%1d]\n",
				i, (mbParaBuf[i] >> 6) & 1);
	}

	fprintf(fpEncQpInfo, "\n");
	fprintf(fpEncSliceBndInfo, "\n");
	fflush(fpEncQpInfo);
	fflush(fpEncSliceBndInfo);
}

void SaveEncMvInfo(u8 *mvParaBuf, int size, int MbNumX, int EncNum)
{
	int i;
	if (!fpEncMvInfo)
		fpEncMvInfo = fopen(FN_ENC_MV_DATA, "w+");
	if (!fpEncMvInfo)
		return;

	fprintf(fpEncMvInfo, "FRAME [%1d]\n", EncNum);
	for (i = 0; i < size / 4; i++)
	{
		u16 mvX = (mvParaBuf[0] << 8) | (mvParaBuf[1]);
		u16 mvY = (mvParaBuf[2] << 8) | (mvParaBuf[3]);
		if (mvX & 0x8000)
		{
			fprintf(fpEncMvInfo, "MbAddr[%4d:For ]: Avail[0] Mv[%5d:%5d]\n", i, 0, 0);
		}
		else
		{
			mvX = (mvX & 0x7FFF) | ((mvX << 1) & 0x8000);
			fprintf(fpEncMvInfo, "MbAddr[%4d:For ]: Avail[1] Mv[%5d:%5d]\n", i, mvX, mvY);
		}
		mvParaBuf += 4;
	}
	fprintf(fpEncMvInfo, "\n");
	fflush(fpEncMvInfo);
}

void SaveEncSliceInfo(u8 *SliceParaBuf, int size, int EncNum)
{
	int i, nMbAddr, nSliceBits;
	if (!fpEncSliceInfo)
		fpEncSliceInfo = fopen(FN_ENC_SLICE_DATA, "w+");
	if (!fpEncSliceInfo)
		return;

	fprintf(fpEncSliceInfo, "EncFrmNum[%3d]\n", EncNum);

	for (i = 0; i < size / 8; i++)
	{
		nMbAddr = (SliceParaBuf[2] << 8) | SliceParaBuf[3];
		nSliceBits = (int)(SliceParaBuf[4] << 24) | (SliceParaBuf[5] << 16) |
					 (SliceParaBuf[6] << 8) | (SliceParaBuf[7]);
		fprintf(fpEncSliceInfo, "[%2d] mbIndex.%3d, Bits.%d\n", i, nMbAddr, nSliceBits);
		SliceParaBuf += 8;
	}

	fprintf(fpEncSliceInfo, "\n");
	fflush(fpEncSliceInfo);
}

void jpgGetHuffTable(EncMjpgParam *param)
{
	/* Rearrange and insert pre-defined Huffman table to deticated variable. */
	memcpy(param->huffBits[DC_TABLE_INDEX0], lumaDcBits, 16); /* Luma DC BitLength */
	memcpy(param->huffVal[DC_TABLE_INDEX0], lumaDcValue, 16); /* Luma DC HuffValue */

	memcpy(param->huffBits[AC_TABLE_INDEX0], lumaAcBits, 16);  /* Luma DC BitLength */
	memcpy(param->huffVal[AC_TABLE_INDEX0], lumaAcValue, 162); /* Luma DC HuffValue */

	memcpy(param->huffBits[DC_TABLE_INDEX1], chromaDcBits, 16); /* Chroma DC BitLength */
	memcpy(param->huffVal[DC_TABLE_INDEX1], chromaDcValue, 16); /* Chroma DC HuffValue */

	memcpy(param->huffBits[AC_TABLE_INDEX1], chromaAcBits, 16);	 /* Chroma AC BitLength */
	memcpy(param->huffVal[AC_TABLE_INDEX1], chromaAcValue, 162); /* Chorma AC HuffValue */
}

void jpgGetQMatrix(EncMjpgParam *param)
{
	/* Rearrange and insert pre-defined Q-matrix to deticated variable. */
	memcpy(param->qMatTab[DC_TABLE_INDEX0], lumaQ2, 64);
	memcpy(param->qMatTab[AC_TABLE_INDEX0], chromaBQ2, 64);

	memcpy(param->qMatTab[DC_TABLE_INDEX1], param->qMatTab[DC_TABLE_INDEX0], 64);
	memcpy(param->qMatTab[AC_TABLE_INDEX1], param->qMatTab[AC_TABLE_INDEX0], 64);
}

void jpgGetCInfoTable(EncMjpgParam *param)
{
	int format = param->mjpg_sourceFormat;

	memcpy(param->cInfoTab, cInfoTable[format], 6 * 4);
}

static int
enc_readbs_reset_buffer(struct encode *enc, PhysicalAddress paBsBufAddr, int bsBufsize)
{
	u32 vbuf;

	vbuf = enc->virt_bsbuf_addr + paBsBufAddr - enc->phy_bsbuf_addr;
	return vpu_write(enc->cmdl, (void *)vbuf, bsBufsize);
}

static int
enc_readbs_ring_buffer(EncHandle handle, struct cmd_line *cmd,
					   u32 bs_va_startaddr, u32 bs_va_endaddr, u32 bs_pa_startaddr,
					   int defaultsize)
{
	RetCode ret;
	int space = 0, room;
	PhysicalAddress pa_read_ptr, pa_write_ptr;
	u32 target_addr, size;

	ret = vpu_EncGetBitstreamBuffer(handle, &pa_read_ptr, &pa_write_ptr,
									(Uint32 *)&size);
	if (ret != RETCODE_SUCCESS)
	{
		err_msg("EncGetBitstreamBuffer failed\n");
		return -1;
	}

	/* No space in ring buffer */
	if (size <= 0)
		return 0;

	if (defaultsize > 0)
	{
		if (size < defaultsize)
			return 0;

		space = defaultsize;
	}
	else
	{
		space = size;
	}

	if (space > 0)
	{
		target_addr = bs_va_startaddr + (pa_read_ptr - bs_pa_startaddr);
		if ((target_addr + space) > bs_va_endaddr)
		{
			room = bs_va_endaddr - target_addr;
			vpu_write(cmd, (void *)target_addr, room);
			vpu_write(cmd, (void *)bs_va_startaddr, (space - room));
		}
		else
		{
			vpu_write(cmd, (void *)target_addr, space);
		}

		ret = vpu_EncUpdateBitstreamBuffer(handle, space);
		if (ret != RETCODE_SUCCESS)
		{
			err_msg("EncUpdateBitstreamBuffer failed\n");
			return -1;
		}
	}

	return space;
}

/*********************************************************/
char spsbuff[15];
char ppsbuff[10];
uint8_t fillheader = 0;
/* h.264 bitstreams */
extern uint32_t timestamp;
static int encoder_fill_headers_frequence(struct encode *enc)
{
	EncHeaderParam enchdr_param = {0};
	EncHandle handle = enc->handle;
	RetCode ret;
	int mbPicNum;

	/* Must put encode header before encoding */
	if (enc->cmdl->format == STD_MPEG4)
	{
	}
	else if (enc->cmdl->format == STD_AVC)
	{
		fillheader = 1;
		timestamp -= TIMESTAMP_RTP;
		ret = vpu_write(enc->cmdl, spsbuff, 13);
		fillheader = 1;
		timestamp -= TIMESTAMP_RTP;
		ret = vpu_write(enc->cmdl, ppsbuff, 9);
		
	}
	else if (enc->cmdl->format == STD_MJPG)
	{
	}

	return 0;
}

static int encoder_fill_headers(struct encode *enc)
{
	EncHeaderParam enchdr_param = {0};
	EncHandle handle = enc->handle;
	RetCode ret;
	int mbPicNum;

	/* Must put encode header before encoding */
	if (enc->cmdl->format == STD_MPEG4)
	{
		enchdr_param.headerType = VOS_HEADER;

		if (cpu_is_mx6x())
			goto put_mp4header;
		/*
		 * Please set userProfileLevelEnable to 0 if you need to generate
	         * user profile and level automaticaly by resolution, here is one
		 * sample of how to work when userProfileLevelEnable is 1.
		 */
		enchdr_param.userProfileLevelEnable = 1;
		mbPicNum = ((enc->enc_picwidth + 15) / 16) * ((enc->enc_picheight + 15) / 16);
		if (enc->enc_picwidth <= 176 && enc->enc_picheight <= 144 &&
			mbPicNum * enc->cmdl->fps <= 1485)
			enchdr_param.userProfileLevelIndication = 8; /* L1 */
		/* Please set userProfileLevelIndication to 8 if L0 is needed */
		else if (enc->enc_picwidth <= 352 && enc->enc_picheight <= 288 &&
				 mbPicNum * enc->cmdl->fps <= 5940)
			enchdr_param.userProfileLevelIndication = 2; /* L2 */
		else if (enc->enc_picwidth <= 352 && enc->enc_picheight <= 288 &&
				 mbPicNum * enc->cmdl->fps <= 11880)
			enchdr_param.userProfileLevelIndication = 3; /* L3 */
		else if (enc->enc_picwidth <= 640 && enc->enc_picheight <= 480 &&
				 mbPicNum * enc->cmdl->fps <= 36000)
			enchdr_param.userProfileLevelIndication = 4; /* L4a */
		else if (enc->enc_picwidth <= 720 && enc->enc_picheight <= 576 &&
				 mbPicNum * enc->cmdl->fps <= 40500)
			enchdr_param.userProfileLevelIndication = 5; /* L5 */
		else
			enchdr_param.userProfileLevelIndication = 6; /* L6 */

	put_mp4header:
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0)
		{
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}

		enchdr_param.headerType = VIS_HEADER;
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0)
		{
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}

		enchdr_param.headerType = VOL_HEADER;
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0)
		{
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			if (ret < 0)
				return -1;
		}
	}
	else if (enc->cmdl->format == STD_AVC)
	{
		info_msg("VAo cho fill header---std avc--------------------------\r\n");
		fillheader = 1;
		if (!enc->mvc_extension || !enc->mvc_paraset_refresh_en)
		{
			// info_msg("VAo cho fill header---std avc------111-\r\n");
			enchdr_param.headerType = SPS_RBSP;
			vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
			if (enc->ringBufferEnable == 0)
			{
				ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
				uint32_t vbuf = enc->virt_bsbuf_addr + enchdr_param.buf - enc->phy_bsbuf_addr;
				memcpy(spsbuff, vbuf, enchdr_param.size);
				for (int i = 0; i < enchdr_param.size; i++)
				{
					printf("%02x - ", spsbuff[i]);
				}
				printf("size of sps %d \r\n", enchdr_param.size);
				if (ret < 0)
					return -1;
			}
		}
		fillheader = 1;
		if (enc->mvc_extension)
		{
			// info_msg("VAo cho fill header---std avc-------222--\r\n");
			enchdr_param.headerType = SPS_RBSP_MVC;
			vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
			if (enc->ringBufferEnable == 0)
			{
				ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
				if (ret < 0)
					return -1;
			}
		}
		fillheader = 1;
		enchdr_param.headerType = PPS_RBSP;
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
		if (enc->ringBufferEnable == 0)
		{
			// info_msg("VAo cho fill header---std avc-------333--\r\n");
			ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
			uint32_t vbuf = enc->virt_bsbuf_addr + enchdr_param.buf - enc->phy_bsbuf_addr;
			memcpy(ppsbuff, vbuf, enchdr_param.size);
			for (int i = 0; i < enchdr_param.size; i++)
			{
				printf("%02x - ", ppsbuff[i]);
			}
			printf("size of pps %d \r\n", enchdr_param.size);
			if (ret < 0)
				return -1;
		}
		fillheader = 1;
		if (enc->mvc_extension)
		{ /* MVC */
			// info_msg("VAo cho fill header---std avc-------444--\r\n");
			enchdr_param.headerType = PPS_RBSP_MVC;
			vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
			if (enc->ringBufferEnable == 0)
			{
				ret = enc_readbs_reset_buffer(enc, enchdr_param.buf, enchdr_param.size);
				if (ret < 0)
					return -1;
			}
		}
	}
	else if (enc->cmdl->format == STD_MJPG)
	{
		if (enc->huffTable)
			free(enc->huffTable);
		if (enc->qMatTable)
			free(enc->qMatTable);
		if (cpu_is_mx6x())
		{
			EncParamSet enchdr_param = {0};
			enchdr_param.size = STREAM_BUF_SIZE;
			enchdr_param.pParaSet = malloc(STREAM_BUF_SIZE);
			if (enchdr_param.pParaSet)
			{
				vpu_EncGiveCommand(handle, ENC_GET_JPEG_HEADER, &enchdr_param);
				vpu_write(enc->cmdl, (void *)enchdr_param.pParaSet, enchdr_param.size);
				free(enchdr_param.pParaSet);
			}
			else
			{
				err_msg("memory allocate failure\n");
				return -1;
			}
		}
	}

	return 0;
}
////.. free buffer
void encoder_free_framebuffer(struct encode *enc)
{
	int i;

	for (i = 0; i < enc->totalfb; i++)
	{
		framebuf_free(enc->pfbpool[i]);
	}

	free(enc->fb);
	free(enc->pfbpool);
}

int encoder_allocate_framebuffer(struct encode *enc)
{
	EncHandle handle = enc->handle;
	int i, enc_stride, src_stride, src_fbid;
	int totalfb, minfbcount, srcfbcount, extrafbcount;
	RetCode ret;
	FrameBuffer *fb;
	PhysicalAddress subSampBaseA = 0, subSampBaseB = 0;
	struct frame_buf **pfbpool;
	EncExtBufInfo extbufinfo = {0};
	int enc_fbwidth, enc_fbheight, src_fbwidth, src_fbheight;

	minfbcount = enc->minFrameBufferCount;
	dprintf(4, "minfb %d\n", minfbcount);
	srcfbcount = 1;

	enc_fbwidth = (enc->enc_picwidth + 15) & ~15;
	enc_fbheight = (enc->enc_picheight + 15) & ~15;
	src_fbwidth = (enc->src_picwidth + 15) & ~15;
	src_fbheight = (enc->src_picheight + 15) & ~15;

	if (cpu_is_mx6x())
	{
		if (enc->cmdl->format == STD_AVC && enc->mvc_extension) /* MVC */
			extrafbcount = 2 + 2;								/* Subsamp [2] + Subsamp MVC [2] */
		else if (enc->cmdl->format == STD_MJPG)
			extrafbcount = 0;
		else
			extrafbcount = 2; /* Subsamp buffer [2] */
	}
	else
		extrafbcount = 0;

	enc->totalfb = totalfb = minfbcount + extrafbcount + srcfbcount;

	/* last framebuffer is used as src frame in the test */
	enc->src_fbid = src_fbid = totalfb - 1;

	fb = enc->fb = calloc(totalfb, sizeof(FrameBuffer));
	if (fb == NULL)
	{
		err_msg("Failed to allocate enc->fb\n");
		return -1;
	}

	pfbpool = enc->pfbpool = calloc(totalfb,
									sizeof(struct frame_buf *));
	if (pfbpool == NULL)
	{
		err_msg("Failed to allocate enc->pfbpool\n");
		free(fb);
		return -1;
	}

	if (enc->cmdl->mapType == LINEAR_FRAME_MAP)
	{
		/* All buffers are linear */
		for (i = 0; i < minfbcount + extrafbcount; i++)
		{
			pfbpool[i] = framebuf_alloc(enc->cmdl->format, enc->mjpg_fmt,
										enc_fbwidth, enc_fbheight, 0);
			if (pfbpool[i] == NULL)
			{
				goto err1;
			}
		}
	}
	else
	{
		/* Encoded buffers are tiled */
		for (i = 0; i < minfbcount; i++)
		{
			pfbpool[i] = tiled_framebuf_alloc(enc->cmdl->format, enc->mjpg_fmt,
											  enc_fbwidth, enc_fbheight, 0, enc->cmdl->mapType);
			if (pfbpool[i] == NULL)
				goto err1;
		}
		/* sub frames are linear */
		for (i = minfbcount; i < minfbcount + extrafbcount; i++)
		{
			pfbpool[i] = framebuf_alloc(enc->cmdl->format, enc->mjpg_fmt,
										enc_fbwidth, enc_fbheight, 0);
			if (pfbpool[i] == NULL)
				goto err1;
		}
	}

	for (i = 0; i < minfbcount + extrafbcount; i++)
	{
		fb[i].myIndex = i;
		fb[i].bufY = pfbpool[i]->addrY;
		fb[i].bufCb = pfbpool[i]->addrCb;
		fb[i].bufCr = pfbpool[i]->addrCr;
		fb[i].strideY = pfbpool[i]->strideY;
		fb[i].strideC = pfbpool[i]->strideC;
	}

	if (cpu_is_mx6x() && (enc->cmdl->format != STD_MJPG))
	{
		subSampBaseA = fb[minfbcount].bufY;
		subSampBaseB = fb[minfbcount + 1].bufY;
		if (enc->cmdl->format == STD_AVC && enc->mvc_extension)
		{ /* MVC */
			extbufinfo.subSampBaseAMvc = fb[minfbcount + 2].bufY;
			extbufinfo.subSampBaseBMvc = fb[minfbcount + 3].bufY;
		}
	}

	/* Must be a multiple of 16 */
	if (enc->cmdl->rot_angle == 90 || enc->cmdl->rot_angle == 270)
		enc_stride = (enc->enc_picheight + 15) & ~15;
	else
		enc_stride = (enc->enc_picwidth + 15) & ~15;
	src_stride = (enc->src_picwidth + 15) & ~15;

	extbufinfo.scratchBuf = enc->scratchBuf;
	ret = vpu_EncRegisterFrameBuffer(handle, fb, minfbcount, enc_stride, src_stride,
									 subSampBaseA, subSampBaseB, &extbufinfo);
	if (ret != RETCODE_SUCCESS)
	{
		err_msg("Register frame buffer failed\n");
		goto err1;
	}

	if (enc->cmdl->src_scheme == PATH_V4L2)
	{
		ret = v4l_capture_setup(enc, enc->src_picwidth, enc->src_picheight, enc->cmdl->fps);
		if (ret < 0)
		{
			goto err1;
		}
	}
	else
	{
		/* Allocate a single frame buffer for source frame */
		pfbpool[src_fbid] = framebuf_alloc(enc->cmdl->format, enc->mjpg_fmt,
										   src_fbwidth, src_fbheight, 0);
		if (pfbpool[src_fbid] == NULL)
		{
			err_msg("failed to allocate single framebuf\n");
			goto err1;
		}

		fb[src_fbid].myIndex = enc->src_fbid;
		fb[src_fbid].bufY = pfbpool[src_fbid]->addrY;
		fb[src_fbid].bufCb = pfbpool[src_fbid]->addrCb;
		fb[src_fbid].bufCr = pfbpool[src_fbid]->addrCr;
		fb[src_fbid].strideY = pfbpool[src_fbid]->strideY;
		fb[src_fbid].strideC = pfbpool[src_fbid]->strideC;
	}

	return 0;

err1:
	for (i = 0; i < totalfb; i++)
	{
		framebuf_free(pfbpool[i]);
	}

	free(fb);
	free(pfbpool);
	return -1;
}

static int read_from_file(struct encode *enc)
{
	u32 y_addr, u_addr, v_addr;
	struct frame_buf *pfb = enc->pfbpool[enc->src_fbid];
	int divX, divY;
	int src_fd = enc->cmdl->src_fd;
	int format = enc->mjpg_fmt;
	int chromaInterleave = enc->cmdl->chromaInterleave;
	int img_size, y_size, c_size;
	int ret = 0;

	if (enc->src_picwidth != pfb->strideY)
	{
		err_msg("Make sure src pic width is a multiple of 16\n");
		return -1;
	}

	divX = (format == MODE420 || format == MODE422) ? 2 : 1;
	divY = (format == MODE420 || format == MODE224) ? 2 : 1;

	y_size = enc->src_picwidth * enc->src_picheight;
	c_size = y_size / divX / divY;
	img_size = y_size + c_size * 2;

	y_addr = pfb->addrY + pfb->desc.virt_uaddr - pfb->desc.phy_addr;
	u_addr = pfb->addrCb + pfb->desc.virt_uaddr - pfb->desc.phy_addr;
	v_addr = pfb->addrCr + pfb->desc.virt_uaddr - pfb->desc.phy_addr;

	if (img_size == pfb->desc.size)
	{
		ret = freadn(src_fd, (void *)y_addr, img_size);
	}
	else
	{
		ret = freadn(src_fd, (void *)y_addr, y_size);
		if (chromaInterleave == 0)
		{
			ret = freadn(src_fd, (void *)u_addr, c_size);
			ret = freadn(src_fd, (void *)v_addr, c_size);
		}
		else
		{
			ret = freadn(src_fd, (void *)u_addr, c_size * 2);
		}
	}
	return ret;
}

struct vpu_queue
{
	int list[MAX_BUF_NUM + 1];
	int head;
	int tail;
};

struct vpu_queue vpu_q;

static pthread_mutex_t vpu_mutex;
static pthread_cond_t vpu_cond;
static int vpu_waiting = 0;
static int vpu_running = 0;
static inline void wait_queue()
{
	pthread_mutex_lock(&vpu_mutex);
	vpu_waiting = 1;
	pthread_cond_wait(&vpu_cond, &vpu_mutex);
	pthread_mutex_unlock(&vpu_mutex);
}

static inline void wakeup_queue()
{
	pthread_cond_signal(&vpu_cond);
}

static __inline int queue_size(struct vpu_queue *q)
{
	if (q->tail >= q->head)
		return (q->tail - q->head);
	else
		return ((q->tail + QUEUE_SIZE) - q->head);
}

static __inline int queue_buf(struct vpu_queue *q, int idx)
{
	if (((q->tail + 1) % QUEUE_SIZE) == q->head)
		return -1; /* queue full */
	pthread_mutex_lock(&vpu_mutex);
	q->list[q->tail] = idx;
	q->tail = (q->tail + 1) % QUEUE_SIZE;
	pthread_mutex_unlock(&vpu_mutex);
	return 0;
}

static __inline int dequeue_buf(struct vpu_queue *q)
{
	int ret;
	if (q->tail == q->head)
		return -1; /* queue empty */
	pthread_mutex_lock(&vpu_mutex);
	ret = q->list[q->head];
	q->head = (q->head + 1) % QUEUE_SIZE;
	pthread_mutex_unlock(&vpu_mutex);
	return ret;
}
////....Thread encoder, duoc tao trong ham encoder_start

void encoder_thread(void *arg)
{
	struct encode *enc = (struct encode *)arg;
	EncHandle handle = enc->handle;
	EncParam enc_param = {0};
	EncOpenParam encop = {0};
	EncOutputInfo outinfo = {0};
	RetCode ret = 0;
	int src_fbid = enc->src_fbid, img_size, frame_id = 0;
	FrameBuffer *fb = enc->fb;
	struct v4l2_buffer v4l2_buf;
	int src_scheme = enc->cmdl->src_scheme;
	int count = enc->cmdl->count;
	struct timeval tenc_begin, tenc_end, total_start, total_end;
	int sec, usec, loop_id;
	float tenc_time = 0, total_time = 0;
	PhysicalAddress phy_bsbuf_start = enc->phy_bsbuf_addr;
	u32 virt_bsbuf_start = enc->virt_bsbuf_addr;
	u32 virt_bsbuf_end = virt_bsbuf_start + STREAM_BUF_SIZE;
	int index;
	pthread_attr_t attr;
	//	int debug_time;  //qiang_debug added
	//	struct timeval tv_start, tv_current;  //qiang_debug added

	printf("DEBUG:...encoder_thread.............\r\n");

	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);

		if (enc->cmdl->dst_scheme == PATH_NET)
	{
		/* open udp port for send path */
		enc->cmdl->dst_fd = udp_open(enc->cmdl);
		if (enc->cmdl->dst_fd < 0)
		{
			//if (enc->cmdl->src_scheme == PATH_NET)
			close(enc->cmdl->src_fd);
		}

		info_msg("encoder sending on port %d\n", enc->cmdl->port);
	}

	char file_name[60];
	time_t t = time(NULL);
	struct tm tm;
	int min_old = 0;
	// t = time(NULL);
	// tm = *localtime(&t);
	sprintf(file_name, "/home/root/mount/cam%d", device_video);
	mkdir(file_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	while (1)
	{
		t = time(NULL);
		tm = *localtime(&t);
		memset(file_name, 0, 60);
		min_old = tm.tm_min;

		sprintf(file_name, "/home/root/mount/cam%d/cam%d_%04d%02d%02d_%02d%02d.h264", device_video, device_video, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
		enc->cmdl->dst_fd_file = open(file_name, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		if (enc->cmdl->dst_fd_file < 0)
		{
			printf("Open File Fail.\r\n");
			break;
		}
		else
		{
			printf("Open File Success. Save new File: %s\r\n", file_name);
		}

		/* Must put encode header here before encoding for all codec, except MX6 MJPG */
		if (!(cpu_is_mx6x() && (enc->cmdl->format == STD_MJPG)))
		{
			ret = encoder_fill_headers(enc);
			if (ret)
			{
				err_msg("Encode fill headers failed\n");
				goto err3;
			}
		}

		enc_param.sourceFrame = &enc->fb[src_fbid];
		enc_param.quantParam =25;//23;
		enc_param.forceIPicture = 0;
		enc_param.skipPicture = 0;
		enc_param.enableAutoSkip = 1;

		enc_param.encLeftOffset = 0;
		enc_param.encTopOffset = 0;
		if ((enc_param.encLeftOffset + enc->enc_picwidth) > enc->src_picwidth)
		{
			err_msg("Configure is failure for width and left offset\n");
			goto err3;
		}
		if ((enc_param.encTopOffset + enc->enc_picheight) > enc->src_picheight)
		{
			err_msg("Configure is failure for height and top offset\n");
			goto err3;
		}

		/* Set report info flag */
		if (enc->mbInfo.enable)
		{
			info_msg("mbbb...info.enable....1111\r\n");
			ret = vpu_EncGiveCommand(handle, ENC_SET_REPORT_MBINFO, &enc->mbInfo);
			if (ret != RETCODE_SUCCESS)
			{
				err_msg("Failed to set MbInfo report, ret %d\n", ret);
				goto err3;
			}
		}
		if (enc->mvInfo.enable)
		{
			info_msg("mvvvvv...info.enable....222\r\n");
			ret = vpu_EncGiveCommand(handle, ENC_SET_REPORT_MVINFO, &enc->mvInfo);
			if (ret != RETCODE_SUCCESS)
			{
				err_msg("Failed to set MvInfo report, ret %d\n", ret);
				goto err3;
			}
		}
		if (enc->sliceInfo.enable)
		{
			info_msg("slice...info.enable....3333\r\n");
			ret = vpu_EncGiveCommand(handle, ENC_SET_REPORT_SLICEINFO, &enc->sliceInfo);
			if (ret != RETCODE_SUCCESS)
			{
				err_msg("Failed to set slice info report, ret %d\n", ret);
				goto err3;
			}
		}
		if (src_scheme == PATH_V4L2)
		{
			vpu_running = 1;
			img_size = enc->src_picwidth * enc->src_picheight; ////..set kich thuoc image
		}
		else
		{
			img_size = enc->src_picwidth * enc->src_picheight * 3 / 2;
			if (enc->cmdl->format == STD_MJPG)
			{
				if (enc->mjpg_fmt == MODE422 || enc->mjpg_fmt == MODE224)
					img_size = enc->src_picwidth * enc->src_picheight * 2;
				else if (enc->mjpg_fmt == MODE400)
					img_size = enc->src_picwidth * enc->src_picheight;
			}
		}

		gettimeofday(&total_start, NULL);
		image_sizeabc = img_size;

		/* The main encoding loop */
		frame_id = 0;
		enc_param.forceIPicture = 1;
		while (1)
		{
			if ((frame_id % GOP_SIZE) == (GOP_SIZE - 1))
			{
				encoder_fill_headers_frequence(enc);
			}
			if (src_scheme == PATH_V4L2)
			{								 ///.. truong hop doc tu camera
				index = dequeue_buf(&vpu_q); ///-- lay vi tri buffer trong queue
				if (index < 0)
				{
					wait_queue(); ///-- doi queue neu chua co
					vpu_waiting = 0;
					index = dequeue_buf(&vpu_q);
					if (index < 0)
					{
						printf("thread is going to finish\n");
						quitflag = 1;
						break;
					}
				}

				fb[src_fbid].myIndex = enc->src_fbid + index;
				fb[src_fbid].bufY = cap_buffers[index].offset;
				fb[src_fbid].bufCb = fb[src_fbid].bufY + img_size;
				fb[src_fbid].bufCr = fb[src_fbid].bufCb + (img_size >> 2);
				fb[src_fbid].strideY = enc->src_picwidth;
				fb[src_fbid].strideC = enc->src_picwidth / 2;
			}
			else
			{
				ret = read_from_file(enc);
				if (ret <= 0)
					break;
			}

			/* Must put encode header before each frame encoding for mx6 MJPG */
			if (cpu_is_mx6x() && (enc->cmdl->format == STD_MJPG))
			{
				ret = encoder_fill_headers(enc);
				if (ret)
				{
					err_msg("Encode fill headers failed\n");
					quitflag = 1;
					goto err2;
				}
			}

			gettimeofday(&tenc_begin, NULL);
			ret = vpu_EncStartOneFrame(handle, &enc_param); ////.. start picture encoder
			if (ret != RETCODE_SUCCESS)
			{
				err_msg("vpu_EncStartOneFrame failed Err code:%d\n",
						ret);
				quitflag = 1;
				goto err2;
			}

			loop_id = 0;
			////.. wait the completion of picture encode operator
			while (vpu_IsBusy())
			{
				vpu_WaitForInt(200);
				if (enc->ringBufferEnable == 1)
				{
					ret = enc_readbs_ring_buffer(handle, enc->cmdl,
												 virt_bsbuf_start, virt_bsbuf_end,
												 phy_bsbuf_start, STREAM_READ_SIZE);
					if (ret < 0)
					{
						quitflag = 1;
						goto err2;
					}
				}
				if (loop_id == 20)
				{
					ret = vpu_SWReset(handle, 0);
					quitflag = 1;
				}
				loop_id++;
			}

			////..After encoding a frame is complete, check the results of encoder operation
			ret = vpu_EncGetOutputInfo(handle, &outinfo);
			if (ret != RETCODE_SUCCESS)
			{
				err_msg("vpu_EncGetOutputInfo failed Err code: %d\n",
						ret);
				quitflag = 1;
				goto err2;
			}

			// if (outinfo.picType == 0)
			// {
			// 	info_msg("I picture _frameId: %d\r\n", frame_id);
			// }

			if (outinfo.skipEncoded)
				info_msg("Skip encoding one Frame!\n");

			if (outinfo.mbInfo.enable && outinfo.mbInfo.size && outinfo.mbInfo.addr)
			{
				// printf("...\r\n");	////khong co
				SaveEncMbInfo(outinfo.mbInfo.addr, outinfo.mbInfo.size,
							  encop.picWidth / 16, frame_id);
			}

			if (outinfo.mvInfo.enable && outinfo.mvInfo.size && outinfo.mvInfo.addr)
			{
				// printf("***\r\n");	////khong co
				SaveEncMvInfo(outinfo.mvInfo.addr, outinfo.mvInfo.size,
							  encop.picWidth / 16, frame_id);
			}

			if (outinfo.sliceInfo.enable && outinfo.sliceInfo.size &&
				outinfo.sliceInfo.addr)
			{
				// printf("---\r\n");	////khong co
				SaveEncSliceInfo(outinfo.sliceInfo.addr,
								 outinfo.sliceInfo.size, frame_id);
			}

			if (quitflag)
				break;

			//		gettimeofday(&tv_start, 0);  //qiang_debug added
			if (enc->ringBufferEnable == 0)
			{
				//printf("...\r\n");  ///------ vao truonwg hop nay

				ret = enc_readbs_reset_buffer(enc, outinfo.bitstreamBuffer, outinfo.bitstreamSize);
				if (ret < 0)
				{
					err_msg("writing bitstream buffer failed\n");
					goto err2;
				}
			}
			else
			{
				enc_readbs_ring_buffer(handle, enc->cmdl, virt_bsbuf_start, virt_bsbuf_end, phy_bsbuf_start, 0);
				//printf("***\r\n");
			}
			frame_id++;
			enc_param.forceIPicture = 0;
			t = time(NULL);
			tm = *localtime(&t);
			if (((frame_id % GOP_SIZE) == 0) && (tm.tm_min != min_old))
			{
				break;
			}
			// if ((count != 0) && (frame_id >= count)){
			// 	quitflag =1;
			// 	break;
			// }
		}
		close(enc->cmdl->dst_scheme_file);
		enc->cmdl->dst_scheme_file = -1;
		nanosleep((const struct timespec[]){{0, 1000000L}}, NULL);
		if (quitflag)
		{
			close(enc->cmdl->dst_scheme_file);
			break;
		}
	}

	gettimeofday(&total_end, NULL);
	sec = total_end.tv_sec - total_start.tv_sec;
	usec = total_end.tv_usec - total_start.tv_usec;
	if (usec < 0)
	{
		sec--;
		usec = usec + 1000000;
	}
	total_time = (sec * 1000000) + usec;
	///ket thuc encode
	info_msg("Finished encoding: %d frames\n", frame_id);
	info_msg("enc fps = %.2f\n", (frame_id / (tenc_time / 1000000)));
	info_msg("total fps= %.2f \n", (frame_id / (total_time / 1000000)));

err2:
	/* Inform the other end that no more frames will be sent */
	if (enc->cmdl->dst_scheme == PATH_NET)
	{
		vpu_write(enc->cmdl, NULL, 0);
	}

	if (enc->mbInfo.addr)
		free(enc->mbInfo.addr);
	if (enc->mbInfo.addr)
		free(enc->mbInfo.addr);
	if (enc->sliceInfo.addr)
		free(enc->sliceInfo.addr);

	if (fpEncQpInfo)
	{
		fclose(fpEncQpInfo);
		fpEncQpInfo = NULL;
	}
	if (fpEncSliceBndInfo)
	{
		fclose(fpEncSliceBndInfo);
		fpEncSliceBndInfo = NULL;
	}
	if (fpEncMvInfo)
	{
		fclose(fpEncMvInfo);
		fpEncMvInfo = NULL;
	}
	if (fpEncSliceInfo)
	{
		fclose(fpEncSliceInfo);
		fpEncSliceInfo = NULL;
	}

err3:
	pthread_attr_destroy(&attr);
	info_msg("encoder_thread exit\n");
	vpu_running = 0;

	pthread_exit(0);
}

extern int device_video;
#define SAVE_IMAGE 0
void image_thread(void) // *arg)
{
	printf("Debug: image thread....\r\n");
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
#if SAVE_IMAGE
	char image_ppm[30];
	char img_jpg[30];
	FILE *fout;
	int count = 0;
	time_t t = time(NULL);
	struct tm tm;

	int Y0, Y1, Cb, Cr;				  /* gamma pre-corrected input [0;255] */
	int ER0, ER1, EG0, EG1, EB0, EB1; /* output [0;255] */
	double r0, r1, g0, g1, b0, b1;	  /* temporaries */
	double y0, y1, pb, pr;
#endif
	int index;

	///g_display_left, g_display_top, g_display_width, g_display_height
	// g_display_width = 960;
	// g_display_height = 540;
	g_display_width = screen_width / 2;
	g_display_height = screen_height / 2;
	g_in_width = 1280;
	g_in_height = 720;
	g_g2d_fmt = G2D_NV12;
	switch (device_video)
	{
	case 0:
		g_display_left = 0;
		g_display_top = 0;
		break;
	case 1:
		g_display_left = g_display_width;
		g_display_top = 0;
		break;
	case 2:
		g_display_left = 0;
		g_display_top = g_display_height;
		break;
	case 3:
		g_display_left = g_display_width;
		g_display_top = g_display_height;
		break;
	default:
		break;
	}

	while (1)
	{
		index = dequeue_dp_buf(&display_q); ///-- lay vi tri buffer trong queue
		if (index < 0)
		{
			wait_dp_queue(); ///-- doi queue neu chua co
			display_waiting = 0;
			index = dequeue_dp_buf(&display_q);
			if (index < 0)
			{
				printf("thread is going to finish\n");
				quitflag = 1;
				goto image_thread_exit;
				break;
			}
		}

		// draw_image_to_framebuffer_tvin(g2d_buffers[index], g_in_width, g_in_height, g_g2d_fmt, &g_screen_info, g_display_left, g_display_top, g_display_width, g_display_height, 0, G2D_ROTATION_0);
		draw_image_to_framebuffer(g2d_buffers[index], g_in_width, g_in_height, g_g2d_fmt, &g_screen_info, g_display_left, g_display_top, g_display_width, g_display_height, 0, G2D_ROTATION_0, g_display_base_phy);

#if SAVE_IMAGE
		// image_thread_wait = 0;
		// // usleep(100000);
		// nanosleep((const struct timespec[]){{0, 10000000L}}, NULL);
		// count++;
		// if (count == 500)
		// {
		// 	count = 0;
		// 	t = time(NULL);
		// 	tm = *localtime(&t);
		// 	sprintf(image_ppm, "image_%04d%02d%02d_%02d%02d%02d.ppm", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		// 	//sprintf(image_ppm, "video%d_%03d.ppm", device_video, i);
		// 	//sprintf(image_ppm, "out%03d.pgmyuv", i);
		// 	fout = fopen(image_ppm, "w");
		// 	if (!fout)
		// 	{
		// 		perror("Cannot open image");
		// 		exit(EXIT_FAILURE);
		// 	}
		// 	fprintf(fout, "P6\n%d %d 255\n", 1280, 720);
		// 	int ii = 0;

		// 	printf("\r\n");
		// 	ii=0;
		// 	for (int iii = 0; iii < 720; iii++)
		// 	{
		// 		int iii_temp = iii/2;
		// 		for (int jjj = 0; jjj < 1280/2; jjj++)
		// 		{
		// 			int C_temp = 2*jjj + 1280*(iii_temp);
		// 			int Y_temp = 2*jjj + 1280*iii;
		// 			Y0 = (char)(buff_Y[Y_temp]);// & 0xFF);
		// 			Y1 = (char)(buff_Y[Y_temp+1]);// & 0xFF);
		// 			Cb = (char)(buff_CrCb[C_temp]);// & 0xFF);
		// 			Cr = (char)(buff_CrCb[C_temp+1]);// & 0xFF);

		// 			// Strip sign values after shift (i.e. unsigned shift)
		// 			Y0 = Y0 & 0xFF;
		// 			Cb = (Cb) & 0xFF;
		// 			Y1 = Y1 & 0xFF;
		// 			Cr = (Cr) & 0xFF;

		// 			//fprintf( fp, "Value:%x Y0:%x Cb:%x Y1:%x Cr:%x ",packed_value,Y0,Cb,Y1,Cr);
		// 			y0 = (255.0 / 219.0) * (Y0 - 16);
		// 			y1 = (255.0 / 219.0) * (Y1 - 16);
		// 			pb = (255.0 / 224.0) * (Cb - 128);
		// 			pr = (255.0 / 224.0) * (Cr - 128);

		// 			// B = 1.164(Y - 16)                   + 2.018(U - 128)
		// 			// G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
		// 			// R = 1.164(Y - 16) + 1.596(V - 128)

		// 			// Generate first pixel
		// 			r0 = y0 + 		1.402 * pr;
		// 			g0 = y0 - 0.344 * pb - 0.714 * pr;
		// 			b0 = y0 + 1.772 * pb	;

		// 			// Generate next pixel - must reuse pb & pr as 4:2:2
		// 			r1 = y1 +		1.402 * pr;
		// 			g1 = y1 - 0.344 * pb - 0.714 * pr;
		// 			b1 = y1 + 1.772 * pb 	;

		// 			ER0 = clamp(r0);
		// 			ER1 = clamp(r1);
		// 			EG0 = clamp(g0);
		// 			EG1 = clamp(g1);
		// 			EB0 = clamp(b0);
		// 			EB1 = clamp(b1);
		// 			fprintf(fout, "%c%c%c%c%c%c", ER0, EG0, EB0, ER1, EG1, EB1); // Output two pixels
		// 			//fprintf( fp, "Memory:%p Pixel:%d R:%d G:%d B:%d     Pixel:%d R:%d G:%d B:%d \n",location,val,ER0,EG0,EB0,(val+1),ER1,EG1,EB1);
		// 			ii++;
		// 		}
		// 	}

		// 	fclose(fout);

		// 	int width, height, channels;
		// 	unsigned char *imgabc = stbi_load(image_ppm, &width, &height, &channels, 0);
		// 	printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
		// 	sprintf(img_jpg, "imagejpg_%04d%02d%02d_%02d%02d%02d.jpg", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		// 	stbi_write_jpg(img_jpg, width, height, channels, imgabc, 100);
		// 	//remove(image_ppm);
		// }
#endif

		if (quitflag)
		{
			goto image_thread_exit;
		}
	}
image_thread_exit:
	pthread_attr_destroy(&attr);
	pthread_exit(0);
}
////goi trong ham encoder test
static int encoder_start(struct encode *enc)
{
	struct v4l2_buffer v4l2_buf;
	pthread_t enc_thread_id;
	pthread_t image_thread_id;
	int src_scheme = enc->cmdl->src_scheme;
	int count = enc->cmdl->count;
	int index, frame_id = 0;
	RetCode ret = 0;

	/***************************************/

	pthread_mutex_init(&vpu_mutex, NULL);
	pthread_cond_init(&vpu_cond, NULL);

	printf("DEBUG:...Tao Thread encoder...\r\n");
	ret = pthread_create(&enc_thread_id, NULL, (void *)encoder_thread, (void *)enc);
	if (ret)
	{
		printf("Create encoder pthread error!\n");
		return -1;
	}

	sleep(1);
	pthread_create(&image_thread_id, NULL, (void *)image_thread, NULL);
	int img_size = 1280 * 720;

	if (src_scheme == PATH_V4L2)
	{
		ret = v4l_start_capturing();
		if (ret < 0)
		{
			return -1;
		}
		unsigned int count_db1 = 1;
		//// thread lay du lieu buffer v4l2
		while (1)
		{
			if ((frame_id % 1000) == 0)
			{
				printf("DEBUG:..encoder start camera: %d - so frame: %d\r\n", device_video, frame_id);
			}

			ret = v4l_get_capture_data(&v4l2_buf); ////.... lay du lieu capture tu camera dung ioctl
			if (ret < 0)
			{
				goto err2;
			}

			index = v4l2_buf.index;
			if (image_thread_wait == 0)
			{
				//printf("DEBUG: %d.....%d..%d\r\n", fb[src_fbid].bufY, fb[src_fbid].bufCb, fb[src_fbid].bufCr);
				// printf("DEBUG: %d.....%d--%d\r\n", cap_buffers[index].offset, cap_buffers[index].length, cap_buffers[index].start);
				memcpy(buff_Y, &(cap_buffers[index].start[0]), img_size);
				memcpy(buff_CrCb, &(cap_buffers[index].start[img_size]), img_size / 2);
				image_thread_wait = 1;
			}

			if ((index % 4) != 3)
			{							  ////..bo frame, giam size
				queue_buf(&vpu_q, index); ////..dua v4l2_buff.index vao list cua queue vpu_q
				wakeup_queue();			  //// mo khoa hang doi queue dung signal
			}
			// /////---------------------
			queue_dp_buf(&display_q, index); ////..dua v4l2_buff.index vao list cua queue vpu_q
			wakeup_dp_queue();				 //// mo khoa hang doi queue dung signal

			v4l_put_capture_data(&v4l2_buf); ////.. ioctl VIDIOC_QBUF

			// g2d_render_capture_data(index);

			if (quitflag)
				break;

			frame_id++;
			// if ((count != 0) && (frame_id >= count))
			// 	break;
		}

		while (vpu_running && ((queue_size(&vpu_q) > 0) || !vpu_waiting))
		{
			// usleep(10000);
			nanosleep((const struct timespec[]){{0, 10000000L}}, NULL);
		}
		if (vpu_running)
		{
			wakeup_queue();
			info_msg("Join vpu loop thread\n");
		}
	}

	pthread_join(enc_thread_id, NULL);
	pthread_mutex_destroy(&vpu_mutex);
	pthread_cond_destroy(&vpu_cond);

	pthread_join(image_thread_id, NULL);
	pthread_mutex_destroy(&display_mutex);
	pthread_cond_destroy(&display_cond);

err2:
	if (src_scheme == PATH_V4L2)
	{
		v4l_stop_capturing();
	}

	/* For automation of test case */
	if (ret > 0)
		ret = 0;
	printf("DEBUG:.FINISH camera: %d - so frame: %d\r\n", device_video, frame_id);
	return ret;
}

int encoder_configure(struct encode *enc)
{
	EncHandle handle = enc->handle;
	SearchRamParam search_pa = {0};
	EncInitialInfo initinfo = {0};
	RetCode ret;
	MirrorDirection mirror;

	printf("DEBUG:..enc_configure...111....\r\n");
	if (cpu_is_mx27())
	{
		search_pa.searchRamAddr = 0xFFFF4C00;
		ret = vpu_EncGiveCommand(handle, ENC_SET_SEARCHRAM_PARAM, &search_pa);
		if (ret != RETCODE_SUCCESS)
		{
			err_msg("Encoder SET_SEARCHRAM_PARAM failed\n");
			return -1;
		}
	}

	if (enc->cmdl->rot_en)
	{
		vpu_EncGiveCommand(handle, ENABLE_ROTATION, 0);
		vpu_EncGiveCommand(handle, ENABLE_MIRRORING, 0);
		vpu_EncGiveCommand(handle, SET_ROTATION_ANGLE,
						   &enc->cmdl->rot_angle);
		mirror = enc->cmdl->mirror;
		vpu_EncGiveCommand(handle, SET_MIRROR_DIRECTION, &mirror);
	}

	ret = vpu_EncGetInitialInfo(handle, &initinfo);
	if (ret != RETCODE_SUCCESS)
	{
		err_msg("Encoder GetInitialInfo failed\n");
		return -1;
	}
	printf("DEBUG:..enc_configure...222....\r\n");
	enc->minFrameBufferCount = initinfo.minFrameBufferCount;

	////-- khong thay vao dieu kienn nay
	if (enc->cmdl->save_enc_hdr)
	{
		if (enc->cmdl->format == STD_MPEG4)
		{
			SaveGetEncodeHeader(handle, ENC_GET_VOS_HEADER,
								"mp4_vos_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_VO_HEADER,
								"mp4_vo_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_VOL_HEADER,
								"mp4_vol_header.dat");
		}
		else if (enc->cmdl->format == STD_AVC)
		{
			SaveGetEncodeHeader(handle, ENC_GET_SPS_RBSP,
								"avc_sps_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_PPS_RBSP,
								"avc_pps_header.dat");
		}
	}

	enc->mbInfo.enable = 0;
	enc->mvInfo.enable = 0;
	enc->sliceInfo.enable = 0;

	if (enc->mbInfo.enable)
	{
		enc->mbInfo.addr = malloc(initinfo.reportBufSize.mbInfoBufSize);
		if (!enc->mbInfo.addr)
			err_msg("malloc_error\n");
	}
	if (enc->mvInfo.enable)
	{
		enc->mvInfo.addr = malloc(initinfo.reportBufSize.mvInfoBufSize);
		if (!enc->mvInfo.addr)
			err_msg("malloc_error\n");
	}
	if (enc->sliceInfo.enable)
	{
		enc->sliceInfo.addr = malloc(initinfo.reportBufSize.sliceInfoBufSize);
		if (!enc->sliceInfo.addr)
			err_msg("malloc_error\n");
	}
	printf("DEBUG:..enc_configure...333....\r\n");
	return 0;
}

void encoder_close(struct encode *enc)
{
	RetCode ret;

	ret = vpu_EncClose(enc->handle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE)
	{
		vpu_SWReset(enc->handle, 0);
		vpu_EncClose(enc->handle);
	}
}

int encoder_open(struct encode *enc)
{
	EncHandle handle = {0};
	EncOpenParam encop = {0};
	Uint8 *huffTable = enc->huffTable;
	Uint8 *qMatTable = enc->qMatTable;
	int i;
	RetCode ret;

	/* Fill up parameters for encoding */
	encop.bitstreamBuffer = enc->phy_bsbuf_addr;
	encop.bitstreamBufferSize = STREAM_BUF_SIZE;
	encop.bitstreamFormat = enc->cmdl->format;
	encop.mapType = enc->cmdl->mapType;
	encop.linear2TiledEnable = enc->linear2TiledEnable;
	/* width and height in command line means source image size */
	if (enc->cmdl->width && enc->cmdl->height)
	{
		enc->src_picwidth = enc->cmdl->width;
		enc->src_picheight = enc->cmdl->height;
	}

	/* enc_width and enc_height in command line means encoder output size */
	if (enc->cmdl->enc_width && enc->cmdl->enc_height)
	{
		enc->enc_picwidth = enc->cmdl->enc_width;
		enc->enc_picheight = enc->cmdl->enc_height;
	}
	else
	{
		enc->enc_picwidth = enc->src_picwidth;
		enc->enc_picheight = enc->src_picheight;
	}

	/* If rotation angle is 90 or 270, pic width and height are swapped */
	if (enc->cmdl->rot_angle == 90 || enc->cmdl->rot_angle == 270)
	{
		encop.picWidth = enc->enc_picheight;
		encop.picHeight = enc->enc_picwidth;
	}
	else
	{
		encop.picWidth = enc->enc_picwidth;
		encop.picHeight = enc->enc_picheight;
	}

	if (enc->cmdl->fps == 0)
		enc->cmdl->fps = 25; //30;

	info_msg("Capture/Encode fps will be %d\n", enc->cmdl->fps);

	/*Note: Frame rate cannot be less than 15fps per H.263 spec */
	encop.frameRateInfo = enc->cmdl->fps;
	encop.bitRate = 0; //enc->cmdl->bitrate;
	//encop.gopSize = enc->cmdl->gop;
	encop.gopSize = GOP_SIZE;
	encop.slicemode.sliceMode = 0;	   /* 0: 1 slice per picture; 1: Multiple slices per picture */
	encop.slicemode.sliceSizeMode = 0; /* 0: silceSize defined by bits; 1: sliceSize defined by MB number*/
	encop.slicemode.sliceSize = 4000;  /* Size of a slice in bits or MB numbers */

	encop.initialDelay = 0;
	encop.vbvBufferSize = 0; /* 0 = ignore 8 */
	encop.intraRefresh = 0;
	encop.sliceReport = 0;
	encop.mbReport = 0;
	encop.mbQpReport = 0;
	encop.rcIntraQp = -1;
	encop.userQpMax = 0;
	encop.userQpMin = 0;
	encop.userQpMinEnable = 0;
	encop.userQpMaxEnable = 0;

	encop.IntraCostWeight = 0;
	encop.MEUseZeroPmv = 0;
	/* (3: 16x16, 2:32x16, 1:64x32, 0:128x64, H.263(Short Header : always 3) */
	encop.MESearchRange = 3;

	encop.userGamma = (Uint32)(0.75 * 32768); /*  (0*32768 <= gamma <= 1*32768) */
	encop.RcIntervalMode = 1;				  /* 0:normal, 1:frame_level, 2:slice_level, 3: user defined Mb_level */
	encop.MbInterval = 0;
	encop.avcIntra16x16OnlyModeEnable = 0;

	encop.ringBufferEnable = enc->ringBufferEnable = 0;
	encop.dynamicAllocEnable = 0;
	encop.chromaInterleave = enc->cmdl->chromaInterleave;

	if (!cpu_is_mx6x() && enc->cmdl->format == STD_MJPG)
	{
		info_msg("encode open: not imx6 && stdmjpg \r\n");
		qMatTable = calloc(192, 1);
		if (qMatTable == NULL)
		{
			err_msg("Failed to allocate qMatTable\n");
			return -1;
		}
		huffTable = calloc(432, 1);
		if (huffTable == NULL)
		{
			free(qMatTable);
			err_msg("Failed to allocate huffTable\n");
			return -1;
		}

		/* Don't consider user defined hufftable this time */
		/* Rearrange and insert pre-defined Huffman table to deticated variable. */
		for (i = 0; i < 16; i += 4)
		{
			huffTable[i] = lumaDcBits[i + 3];
			huffTable[i + 1] = lumaDcBits[i + 2];
			huffTable[i + 2] = lumaDcBits[i + 1];
			huffTable[i + 3] = lumaDcBits[i];
		}
		for (i = 16; i < 32; i += 4)
		{
			huffTable[i] = lumaDcValue[i + 3 - 16];
			huffTable[i + 1] = lumaDcValue[i + 2 - 16];
			huffTable[i + 2] = lumaDcValue[i + 1 - 16];
			huffTable[i + 3] = lumaDcValue[i - 16];
		}
		for (i = 32; i < 48; i += 4)
		{
			huffTable[i] = lumaAcBits[i + 3 - 32];
			huffTable[i + 1] = lumaAcBits[i + 2 - 32];
			huffTable[i + 2] = lumaAcBits[i + 1 - 32];
			huffTable[i + 3] = lumaAcBits[i - 32];
		}
		for (i = 48; i < 216; i += 4)
		{
			huffTable[i] = lumaAcValue[i + 3 - 48];
			huffTable[i + 1] = lumaAcValue[i + 2 - 48];
			huffTable[i + 2] = lumaAcValue[i + 1 - 48];
			huffTable[i + 3] = lumaAcValue[i - 48];
		}
		for (i = 216; i < 232; i += 4)
		{
			huffTable[i] = chromaDcBits[i + 3 - 216];
			huffTable[i + 1] = chromaDcBits[i + 2 - 216];
			huffTable[i + 2] = chromaDcBits[i + 1 - 216];
			huffTable[i + 3] = chromaDcBits[i - 216];
		}
		for (i = 232; i < 248; i += 4)
		{
			huffTable[i] = chromaDcValue[i + 3 - 232];
			huffTable[i + 1] = chromaDcValue[i + 2 - 232];
			huffTable[i + 2] = chromaDcValue[i + 1 - 232];
			huffTable[i + 3] = chromaDcValue[i - 232];
		}
		for (i = 248; i < 264; i += 4)
		{
			huffTable[i] = chromaAcBits[i + 3 - 248];
			huffTable[i + 1] = chromaAcBits[i + 2 - 248];
			huffTable[i + 2] = chromaAcBits[i + 1 - 248];
			huffTable[i + 3] = chromaAcBits[i - 248];
		}
		for (i = 264; i < 432; i += 4)
		{
			huffTable[i] = chromaAcValue[i + 3 - 264];
			huffTable[i + 1] = chromaAcValue[i + 2 - 264];
			huffTable[i + 2] = chromaAcValue[i + 1 - 264];
			huffTable[i + 3] = chromaAcValue[i - 264];
		}

		/* Rearrange and insert pre-defined Q-matrix to deticated variable. */
		for (i = 0; i < 64; i += 4)
		{
			qMatTable[i] = lumaQ2[i + 3];
			qMatTable[i + 1] = lumaQ2[i + 2];
			qMatTable[i + 2] = lumaQ2[i + 1];
			qMatTable[i + 3] = lumaQ2[i];
		}
		for (i = 64; i < 128; i += 4)
		{
			qMatTable[i] = chromaBQ2[i + 3 - 64];
			qMatTable[i + 1] = chromaBQ2[i + 2 - 64];
			qMatTable[i + 2] = chromaBQ2[i + 1 - 64];
			qMatTable[i + 3] = chromaBQ2[i - 64];
		}
		for (i = 128; i < 192; i += 4)
		{
			qMatTable[i] = chromaRQ2[i + 3 - 128];
			qMatTable[i + 1] = chromaRQ2[i + 2 - 128];
			qMatTable[i + 2] = chromaRQ2[i + 1 - 128];
			qMatTable[i + 3] = chromaRQ2[i - 128];
		}
	}

	if (enc->cmdl->format == STD_MPEG4)
	{
		encop.EncStdParam.mp4Param.mp4_dataPartitionEnable = 0;
		enc->mp4_dataPartitionEnable =
			encop.EncStdParam.mp4Param.mp4_dataPartitionEnable;
		encop.EncStdParam.mp4Param.mp4_reversibleVlcEnable = 0;
		encop.EncStdParam.mp4Param.mp4_intraDcVlcThr = 0;
		encop.EncStdParam.mp4Param.mp4_hecEnable = 0;
		encop.EncStdParam.mp4Param.mp4_verid = 2;
	}
	else if (enc->cmdl->format == STD_H263)
	{
		encop.EncStdParam.h263Param.h263_annexIEnable = 0;
		encop.EncStdParam.h263Param.h263_annexJEnable = 1;
		encop.EncStdParam.h263Param.h263_annexKEnable = 0;
		encop.EncStdParam.h263Param.h263_annexTEnable = 0;
	}
	else if (enc->cmdl->format == STD_AVC)
	{
		encop.EncStdParam.avcParam.avc_constrainedIntraPredFlag = 0;
		encop.EncStdParam.avcParam.avc_disableDeblk = 0;
		encop.EncStdParam.avcParam.avc_deblkFilterOffsetAlpha = 6;
		encop.EncStdParam.avcParam.avc_deblkFilterOffsetBeta = 0;
		encop.EncStdParam.avcParam.avc_chromaQpOffset = 10;
		encop.EncStdParam.avcParam.avc_audEnable = 0;
		if (cpu_is_mx6x())
		{
			encop.EncStdParam.avcParam.interview_en = 0;
			encop.EncStdParam.avcParam.paraset_refresh_en = enc->mvc_paraset_refresh_en = 0;
			encop.EncStdParam.avcParam.prefix_nal_en = 0;
			encop.EncStdParam.avcParam.mvc_extension = enc->cmdl->mp4_h264Class;
			enc->mvc_extension = enc->cmdl->mp4_h264Class;
			encop.EncStdParam.avcParam.avc_frameCroppingFlag = 0;
			encop.EncStdParam.avcParam.avc_frameCropLeft = 0;
			encop.EncStdParam.avcParam.avc_frameCropRight = 0;
			encop.EncStdParam.avcParam.avc_frameCropTop = 0;
			encop.EncStdParam.avcParam.avc_frameCropBottom = 0;
			if (enc->cmdl->rot_angle != 90 &&
				enc->cmdl->rot_angle != 270 &&
				enc->enc_picheight == 1080)
			{
				/*
				 * In case of AVC encoder, when we want to use
				 * unaligned display width frameCroppingFlag
				 * parameters should be adjusted to displayable
				 * rectangle
				 */
				encop.EncStdParam.avcParam.avc_frameCroppingFlag = 1;
				encop.EncStdParam.avcParam.avc_frameCropBottom = 8;
			}
		}
		else
		{
			encop.EncStdParam.avcParam.avc_fmoEnable = 0;
			encop.EncStdParam.avcParam.avc_fmoType = 0;
			encop.EncStdParam.avcParam.avc_fmoSliceNum = 1;
			encop.EncStdParam.avcParam.avc_fmoSliceSaveBufSize = 32; /* FMO_SLICE_SAVE_BUF_SIZE */
		}
	}
	else if (enc->cmdl->format == STD_MJPG)
	{
		encop.EncStdParam.mjpgParam.mjpg_sourceFormat = enc->mjpg_fmt; /* encConfig.mjpgChromaFormat */
		encop.EncStdParam.mjpgParam.mjpg_restartInterval = 60;
		encop.EncStdParam.mjpgParam.mjpg_thumbNailEnable = 0;
		encop.EncStdParam.mjpgParam.mjpg_thumbNailWidth = 0;
		encop.EncStdParam.mjpgParam.mjpg_thumbNailHeight = 0;
		if (cpu_is_mx6x())
		{
			jpgGetHuffTable(&encop.EncStdParam.mjpgParam);
			jpgGetQMatrix(&encop.EncStdParam.mjpgParam);
			jpgGetCInfoTable(&encop.EncStdParam.mjpgParam);
		}
		else
		{
			encop.EncStdParam.mjpgParam.mjpg_hufTable = huffTable;
			encop.EncStdParam.mjpgParam.mjpg_qMatTable = qMatTable;
		}
	}

	ret = vpu_EncOpen(&handle, &encop);
	if (ret != RETCODE_SUCCESS)
	{
		if (enc->cmdl->format == STD_MJPG)
		{
			free(qMatTable);
			free(huffTable);
		}
		err_msg("Encoder open failed %d\n", ret);
		return -1;
	}

	enc->handle = handle;
	return 0;
}

///
extern int fd_fb;
int encode_test(void *arg)
{
	struct cmd_line *cmdl = (struct cmd_line *)arg;
	vpu_mem_desc mem_desc = {0};
	vpu_mem_desc scratch_mem_desc = {0};
	struct encode *enc;
	int ret = 0;

#ifndef COMMON_INIT
	vpu_versioninfo ver;
	ret = vpu_Init(NULL);
	if (ret)
	{
		err_msg("VPU Init Failure.\n");
		return -1;
	}

	ret = vpu_GetVersionInfo(&ver);
	if (ret)
	{
		err_msg("Cannot get version info, err:%d\n", ret);
		vpu_UnInit();
		return -1;
	}

	info_msg("VPU firmware version: %d.%d.%d_r%d\n", ver.fw_major, ver.fw_minor,
			 ver.fw_release, ver.fw_code);
	info_msg("VPU library version: %d.%d.%d\n", ver.lib_major, ver.lib_minor,
			 ver.lib_release);
#endif

	/* sleep some time so that we have time to start the server */
	if (cmdl->dst_scheme == PATH_NET)
	{
		sleep(3);
	}

	/* allocate memory for must remember stuff */
	enc = (struct encode *)calloc(1, sizeof(struct encode));
	if (enc == NULL)
	{
		err_msg("Failed to allocate encode structure\n");
		return -1;
	}

	/* get physical contigous bit stream buffer */
	mem_desc.size = STREAM_BUF_SIZE;
	ret = IOGetPhyMem(&mem_desc);
	if (ret)
	{
		err_msg("Unable to obtain physical memory\n");
		free(enc);
		return -1;
	}

	/* mmap that physical buffer */
	enc->virt_bsbuf_addr = IOGetVirtMem(&mem_desc);
	if (enc->virt_bsbuf_addr <= 0)
	{
		err_msg("Unable to map physical memory\n");
		IOFreePhyMem(&mem_desc);
		free(enc);
		return -1;
	}

	enc->phy_bsbuf_addr = mem_desc.phy_addr;
	enc->cmdl = cmdl;

	if (enc->cmdl->format == STD_MJPG)
		enc->mjpg_fmt = MODE420; /* Please change this per your needs */

	if (enc->cmdl->mapType)
	{
		enc->linear2TiledEnable = 1;
		enc->cmdl->chromaInterleave = 1; /* Must be CbCrInterleave for tiled */
		if (cmdl->format == STD_MJPG)
		{
			err_msg("MJPG encoder cannot support tiled format\n");
			return -1;
		}
	}
	else
		enc->linear2TiledEnable = 0;

	/* open the encoder */
	ret = encoder_open(enc);
	if (ret)
		goto err;

	/* configure the encoder */
	ret = encoder_configure(enc);
	if (ret)
		goto err1;

	/* allocate scratch buf */
	if (cpu_is_mx6x() && (cmdl->format == STD_MPEG4) && enc->mp4_dataPartitionEnable)
	{
		scratch_mem_desc.size = MPEG4_SCRATCH_SIZE;
		ret = IOGetPhyMem(&scratch_mem_desc);
		if (ret)
		{
			err_msg("Unable to obtain physical slice save mem\n");
			goto err1;
		}
		enc->scratchBuf.bufferBase = scratch_mem_desc.phy_addr;
		enc->scratchBuf.bufferSize = scratch_mem_desc.size;
	}

	/* allocate memory for the frame buffers */
	ret = encoder_allocate_framebuffer(enc);
	if (ret)
		goto err1;

	/* start encoding */
	ret = encoder_start(enc);

	/* free the allocated framebuffers */
	encoder_free_framebuffer(enc);
err1:
	/* close the encoder */
	encoder_close(enc);
err:
	if (cpu_is_mx6x() && cmdl->format == STD_MPEG4 && enc->mp4_dataPartitionEnable)
	{
		IOFreeVirtMem(&scratch_mem_desc);
		IOFreePhyMem(&scratch_mem_desc);
	}
	/* free the physical memory */
	IOFreeVirtMem(&mem_desc);
	IOFreePhyMem(&mem_desc);
	free(enc);
#ifndef COMMON_INIT
	vpu_UnInit();
#endif
	return ret;
}
