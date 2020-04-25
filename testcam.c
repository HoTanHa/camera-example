#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define LEN 25
int main(int argc, char **argv){
	// system("gst-launch-1.0 -v imxv4l2src device=/dev/video0 ! video/x-raw, format=UYVY, width=1280, height=720, framerate=30/1 ! queue ! imxv4l2sink");

	char line[LEN];
	while (1){
	FILE *cmd = popen("gst-launch-1.0 -v imxv4l2src device=/dev/video0 ! video/x-raw, format=UYVY, width=1280, height=720, framerate=30/1 ! queue ! imxv4l2sink", "r");
	fgets(line, LEN, cmd);
	pid_t pid = strtoul(line, NULL, 10);
	sleep(20);
	sprintf(line, "kill -1 %d", pid);
	system("pkill -f \"gst-launch-1.0\"");
	pclose(cmd);

	cmd = popen("gst-launch-1.0 -v imxv4l2src device=/dev/video1 ! video/x-raw, format=UYVY, width=1280, height=720, framerate=30/1 ! queue ! imxv4l2sink", "r");
	fgets(line, LEN, cmd);
	pid_t pid1 = strtoul(line, NULL, 10);
	sleep(20);
	sprintf(line, "kill -1 %d", pid1);
	system("pkill -f \"gst-launch-1.0\"");
	pclose(cmd);

	cmd = popen("gst-launch-1.0 -v imxv4l2src device=/dev/video2 ! video/x-raw, format=UYVY, width=1280, height=720, framerate=30/1 ! queue ! imxv4l2sink", "r");
	fgets(line, LEN, cmd);
	pid_t pid2 = strtoul(line, NULL, 10);
	sleep(20);
	sprintf(line, "kill -1 %d", pid2);
	system("pkill -f \"gst-launch-1.0\"");
	pclose(cmd);

	cmd = popen("gst-launch-1.0 -v imxv4l2src device=/dev/video3 ! video/x-raw, format=UYVY, width=1280, height=720, framerate=30/1 ! queue ! imxv4l2sink", "r");
	fgets(line, LEN, cmd);
	pid_t pid3 = strtoul(line, NULL, 10);
	sleep(20);
	sprintf(line, "kill -1 %d", pid3);
	system("pkill -f \"gst-launch-1.0\"");
	pclose(cmd);
	}

	
}

