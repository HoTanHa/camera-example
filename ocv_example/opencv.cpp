#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

int main(int argc, char **argv)
{
    VideoCapture cap(3); // open the default camera
    if(!cap.isOpened())  // check if we succeeded
        return -1;
    Mat edges;
    namedWindow("frame",1);
    for(;;)
    {
        Mat frame;
        cap >> frame; 
        imshow("frame", frame);
        if(waitKey(30) >= 0) break;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}
