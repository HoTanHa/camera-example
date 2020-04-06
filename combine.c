//
// combine.c - Join multiple MP4 videos and PNG images into one video
// Written by Ted Burke - last updated 12-2-2017
//
// To compile:
// 
//    gcc combine.c
// 
 
#include <stdio.h>
 
// Video resolution
#define W 1280
#define H 720
 
// Allocate a buffer to store one frame
unsigned char frame[H][W][3] = {0};
 
void main()
{
    int count, n;
    FILE *pipein;
    FILE *pipeout;
     
    // Open output pipe
    pipeout = popen("ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 combined.mp4", "w");
     
    // Write first 50 frames using original video title image from title_original.png
    pipein = popen("ffmpeg -i title_original.png -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "r");
    count = fread(frame, 1, H*W*3, pipein);
    for (n=0 ; n<50 ; ++n)
    {
        fwrite(frame, 1, H*W*3, pipeout);
        fflush(pipeout);
    }
    fflush(pipein);
    pclose(pipein);
     
    // Copy all frames from teapot.mp4 to output pipe
    pipein = popen("ffmpeg -i teapot.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "r");
    while(1)
    {
        count = fread(frame, 1, H*W*3, pipein);
        if (count != H*W*3) break;
        fwrite(frame, 1, H*W*3, pipeout);
        fflush(pipeout);
    }
    fflush(pipein);
    pclose(pipein);
 
    // Write next 50 frames using modified video title image from title_modified.png
    pipein = popen("ffmpeg -i title_modified.png -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "r");
    count = fread(frame, 1, H*W*3, pipein);
    for (n=0 ; n<50 ; ++n)
    {
        fwrite(frame, 1, H*W*3, pipeout);
        fflush(pipeout);
    }
    fflush(pipein);
    pclose(pipein);
     
    // Copy all frames from output.mp4 to output pipe
    pipein = popen("ffmpeg -i output.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "r");
    while(1)
    {
        count = fread(frame, 1, H*W*3, pipein);
        if (count != H*W*3) break;
        fwrite(frame, 1, H*W*3, pipeout);
        fflush(pipeout);
    }
    fflush(pipein);
    pclose(pipein);
     
    // Flush and close output pipe
    fflush(pipeout);
    pclose(pipeout);
}
