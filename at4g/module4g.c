#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h> /* POSIX Terminal Control Definitions */
#include <unistd.h>	 /* UNIX Standard Definitions 	   */
#include <errno.h>	 /* ERROR Number Definitions           */
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
//#include <usb-wwan.h>


#define TIMESLEEP 1000000

int setup_Serial(int fd_serial){
	struct termios SerialPortSettings; 
	tcgetattr(fd_serial, &SerialPortSettings); 
	cfsetispeed(&SerialPortSettings, B115200);
	cfsetospeed(&SerialPortSettings, B115200);
	/* 8N1 Mode */
	SerialPortSettings.c_cflag &= ~PARENB; 
	SerialPortSettings.c_cflag &= ~CSTOPB;
	SerialPortSettings.c_cflag &= ~CSIZE; 
	SerialPortSettings.c_cflag |= CS8;

	//SerialPortSettings.c_cflag &= ~CRTSCTS;
	SerialPortSettings.c_cflag |= CRTSCTS;
	SerialPortSettings.c_cflag |= CREAD | CLOCAL;

	SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);
	SerialPortSettings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	SerialPortSettings.c_oflag &= ~OPOST; 
	/* Setting Time outs */
	SerialPortSettings.c_cc[VMIN] = 10;
	SerialPortSettings.c_cc[VTIME] = 0;

	if ((tcsetattr(fd_serial, TCSANOW, &SerialPortSettings)) != 0){
		printf("ERROR ! in Setting attributes\r\n");
		return 1;
	}
	else {
		printf("Setup Serial Port Terminal. \r\n");
		printf("BaudRate = 115200__StopBits = 1__Parity = none\r\n");
	}
	tcflush(fd_serial, TCIFLUSH);
	return 0;
}

int write_Serial_4g(int fd_serial, char *at_cmd, int length){
	int bytes_written;
	printf("Send: %s", at_cmd);
	bytes_written = write(fd_serial, at_cmd, length);
	if (bytes_written==length) 
		return 0;
	return 1;
}

char recieve[256];
int  idx=0;
int recv_complete=0;
static void *read_Serial_4g(void *argv){
	int bytes_read;
	char read_buffer[10];
	int fd = *(int*)argv;
	
	while (1) {
		bytes_read = read(fd, &read_buffer, 1);
		if (bytes_read > 0)
		{
			if (read_buffer[0]=='\n' && recieve[idx]=='\r'){
				recv_complete = 1;
				recieve[idx] = '\0';
			}
			else {
				idx++;
				recieve[idx] = read_buffer[0];
			}
			// printf("%c", read_buffer[0]);
		}
		if (recv_complete){
			if (idx>1)
				printf("Recieve: %f\r\n");
			idx=0;
		}
		bytes_read =0;
	}

}

struct timeval tenc_begin, tenc_end, total_start, total_end;
int sec, usec;
int total_time =0;

void fnc_slp(int a){
	// for (int i=0; i<=a*TIMESLEEP; i++){
	// 	int j=0;
	// 	while (j<10000000){
	// 		j++;
	// 	}
	// }
	gettimeofday(&total_start, NULL);
	int i=0;
	while(1){
		gettimeofday(&total_end, NULL);
		sec = total_end.tv_sec - total_start.tv_sec;
		usec = total_end.tv_usec - total_start.tv_usec;
		if (usec < 0)
		{
			sec--;
			usec = usec + 1000000;
		}
		total_time = (sec * 1000000) + usec;
		if (total_time>(a*TIMESLEEP)) break;
	}
}

void main(int argc, char *argv[]){
	int fd_serial_4g;
	pthread_t thread_read_serial;
	char *file_name = "/dev/ttyUSB3";
//	fd_serial_4g = open(file_name, O_RDWR, 0 );
	fd_serial_4g = open(argv[1], O_RDWR, 0 );
	if (fd_serial_4g==-1){
		printf("Open ttyUSB 4g fail\r\n");
		exit(EXIT_FAILURE);
	}
	if (setup_Serial(fd_serial_4g)){
		exit(EXIT_FAILURE);
	}
	
	int ret;
	int *arg = malloc(sizeof(*arg));
	*arg = fd_serial_4g;
	ret = pthread_create(&thread_read_serial, NULL, &read_Serial_4g, arg);
	char at_cmd[40];

	for (int ii=0; ii<20; ii++){
		sprintf(at_cmd, "AT\r\n");
		if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
			exit(EXIT_FAILURE);
		}
		// fnc_slp(1);
	}

	sprintf(at_cmd, "AT+IPR=115200\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(1);

	sprintf(at_cmd, "ATE0\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(1);

	sprintf(at_cmd, "AT+CMEE=2\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(3);

	sprintf(at_cmd, "AT&W\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(3);

	sprintf(at_cmd, "ATI\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(3);

	sprintf(at_cmd, "AT+CPIN?\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(3);

	sprintf(at_cmd, "AT+COPS?\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(3);

	sprintf(at_cmd,"AT+QCFG=\"usbnet\"\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(3);

	sprintf(at_cmd,"AT+CREG=2\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(5);

	sprintf(at_cmd,"AT+QSPN\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(5);

	sprintf(at_cmd,"AT+CGACT=1,1\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(5);

	sprintf(at_cmd,"AT+CGDCONT=1,\"IP\"\r\n");
	if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
		exit(EXIT_FAILURE);
	}
	fnc_slp(5);

//	sprintf(at_cmd,"AT+QICSGP=1,1,\"E-connect\",\"\",\"\",1"); 
	// sprintf(at_cmd,"AT+QICSGP=1,1,\"mwap\",\"mms\",\"mms\",1\r\n"); 
	// if (write_Serial_4g(fd_serial_4g, at_cmd, strlen(at_cmd))){
	// 	exit(EXIT_FAILURE);
	// }
	fnc_slp(5);
//	sleep(10);

//	while (1)
	// {
	// 	/* code */
	// }
	
	// pthread_exit(NULL);
	close(fd_serial_4g);
	return 0;
}
