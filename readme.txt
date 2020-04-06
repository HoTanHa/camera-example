WORKS (EXEC)
$ g++ -Wall -o webcam_capture.bin webcam_capture.cpp

gcc v4l1grab.c -o v4l1grab -Wall -ljpeg -DIO_READ -DIO_MMAP -DIO_USERPTR
gcc $CFLAGS -s -Wall -Wstrict-prototypes v4l1grab.c -o v4l1grab
gcc v4l1grab.c -o v4l1grab -Wall -ljpeg -DIO_READ -DIO_MMAP -DIO_USERPTR
./v4l2grab -o image.jpeg
./v4l2grab -q 100 -W 1280 -H 720 -o image.jpeg


cmd:
 streamer -c /dev/video0 -b 16 -o outfile.jpeg
 streamer -q -c /dev/video0 -f rgb24 -r 3 -t 00:30:00 -o test.avi

buidl v4l2 example from v4l2
gcc -o2 -Wall `pkg-config --cflags --libs libv4l2` v4l2-capture.c -o v4l2-capture
gcc -o2 -Wall `pkg-config --cflags --libs libv4l2` v4l2-grab.c -o v4l2-grab
gcc -o2 -Wall `pkg-config --cflags --libs libv4l2` v4l2-grab.c -lv4l1 -lv4l2 -o v4l2-grab

./capture -m -o abc.jpeg -f 640*480 -o -c 100 > output.raw
avconv -i output.raw -vcodec copy output.mp4
