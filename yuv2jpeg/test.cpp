#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <memory>

#include <jpeglib.h>

using namespace std;
/// g++ test.cpp -o test -ljpeg
/**
 * converts a YUYV raw buffer to a JPEG buffer.
 * input is in YUYV (YUV 422). output is JPEG binary.
 * from https://linuxtv.org/downloads/v4l-dvb-apis/V4L2-PIX-FMT-YUYV.html:
 *      Each four bytes is two pixels.
 *      Each four bytes is two Y's, a Cb and a Cr.
 *      Each Y goes to one of the pixels, and the Cb and Cr belong to both pixels.
 *
 * inspired by: http://stackoverflow.com/questions/17029136/weird-image-while-trying-to-compress-yuv-image-to-jpeg-using-libjpeg
 */
void compressYUYVtoJPEG(const vector<uint8_t>& input, const int width, const int height, vector<uint8_t>& output) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_ptr[1];
    int row_stride;

    uint8_t* outbuffer = NULL;
    uint64_t outlen = 0;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &outbuffer, &outlen);

    // jrow is a libjpeg row of samples array of 1 row pointer
    cinfo.image_width = width & -1;
    cinfo.image_height = height & -1;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr; //libJPEG expects YUV 3bytes, 24bit

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 92, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    vector<uint8_t> tmprowbuf(width * 3);

    JSAMPROW row_pointer[1];
    row_pointer[0] = &tmprowbuf[0];
    while (cinfo.next_scanline < cinfo.image_height) {
        unsigned i, j;
        unsigned offset = cinfo.next_scanline * cinfo.image_width * 2; //offset to the correct row
        for (i = 0, j = 0; i < cinfo.image_width * 2; i += 4, j += 6) { //input strides by 4 bytes, output strides by 6 (2 pixels)
            tmprowbuf[j + 0] = input[offset + i + 0]; // Y (unique to this pixel)
            tmprowbuf[j + 1] = input[offset + i + 1]; // U (shared between pixels)
            tmprowbuf[j + 2] = input[offset + i + 3]; // V (shared between pixels)
            tmprowbuf[j + 3] = input[offset + i + 2]; // Y (unique to this pixel)
            tmprowbuf[j + 4] = input[offset + i + 1]; // U (shared between pixels)
            tmprowbuf[j + 5] = input[offset + i + 3]; // V (shared between pixels)
        }
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    cout << "libjpeg produced " << outlen << " bytes" << endl;

    output = vector<uint8_t>(outbuffer, outbuffer + outlen);
}

int main() {

    ifstream input("frame-3.raw", ios::binary);
    // copies all data into buffer
    vector <uint8_t> inbuf((istreambuf_iterator<char>(input)), (istreambuf_iterator<char>()));

    cout << "read " << inbuf.size() << " bytes" << endl;

    vector<uint8_t> output;
    compressYUYVtoJPEG(inbuf, 1280, 720, output);

    ofstream ofs("output.jpg", ios_base::binary);
    ofs.write((const char*) &output[0], output.size());
    ofs.close();

    return 1;
}