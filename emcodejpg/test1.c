#include <stdio.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

int main()
{
int width, height, channels;
//unsigned char *img = stbi_load("abc.jpg", &width, &height, &channels, 0);
unsigned char *img = stbi_load("video0_000.ppm", &width, &height, &channels, 0);
if(img ==NULL)
{
printf("Error in loading the image\n");
exit(1);
}
printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
stbi_write_png("aas.png", width, height, channels, img, width * channels);
stbi_write_jpg("sky2.jpg", width, height, channels, img, 100);


return 0;

}
//gcc -std=c11 -Wall -pedantic my_program.c -lm -o my_program