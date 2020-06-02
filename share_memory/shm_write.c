/* Filename: shm_write.c */
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 1024
#define SHM_KEY 0x1234

#define info_msg(fmt, ...)                                               \
	do                                                                   \
	{                                                                    \
		printf("[INFO]\t%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
	} while (0)

struct shmseg
{
	int cnt;
	int complete;
	char buf[BUF_SIZE];
	char ip_address[16];
	int port1;
	int port2;
	int port3;
	int port4;
};
int fill_buffer(char *bufptr, int size);
void fill_ip(char *bufpip);
int main(int argc, char *argv[])
{
	int shmid, numtimes;
	struct shmseg *shmp;
	char *bufptr;
	char *buffip;
	int spaceavailable;
	shmid = shmget(SHM_KEY, sizeof(struct shmseg), 0644 | IPC_CREAT);
	if (shmid == -1)
	{
		perror("Shared memory");
		return 1;
	}

	// Attach to the segment to get a pointer to it.
	shmp = shmat(shmid, NULL, 0);
	if (shmp == (void *)-1)
	{
		perror("Shared memory attach");
		return 1;
	}

	/* Transfer blocks of data from buffer to shared memory */
	bufptr = shmp->buf;
	buffip = shmp->ip_address;
	shmp->port1 = 3000;
	shmp->port2 = 4000;
	shmp->port3 = 5000;
	shmp->port4 = 6000;
	spaceavailable = BUF_SIZE;
	for (numtimes = 0; numtimes < 5; numtimes++)
	{
		shmp->cnt = fill_buffer(bufptr, spaceavailable);
		fill_ip(buffip);
		shmp->complete = 0;
		info_msg("Writing Process: Shared Memory Write: Wrote %d bytes\n", shmp->cnt);
		bufptr = shmp->buf;
		buffip = shmp->ip_address;
		shmp->port1+=1;
		shmp->port2+=2;
		shmp->port3+=3;
		shmp->port4+=4;
		spaceavailable = BUF_SIZE;
		sleep(3);
	}
	info_msg("Writing Process: Wrote %d times\r\n", numtimes);
	shmp->complete = 1;

	if (shmdt(shmp) == -1)
	{
		perror("shmdt");
		return 1;
	}

	if (shmctl(shmid, IPC_RMID, 0) == -1)
	{
		perror("shmctl");
		return 1;
	}
	info_msg("Writing Process: Complete\r\n");
	return 0;
}

int fill_buffer(char *bufptr, int size)
{
#if 0
	static char ch = 'A';
	int filled_count;

	//printf("size is %d\n", size);
	memset(bufptr, ch, size - 1);
	bufptr[size - 1] = '\0';
	if (ch > 122)
		ch = 65;
	if ((ch >= 65) && (ch <= 122))
	{
		if ((ch >= 91) && (ch <= 96))
		{
			ch = 65;
		}
	}
	filled_count = strlen(bufptr);

	//printf("buffer count is: %d\n", filled_count);
	//printf("buffer filled is:%s\n", bufptr);
	ch++;
	return filled_count;
#else
	int filled_count = 0;
	char *str_wtire = (char *)malloc(20 + 1);
	static int a = 1;
	sprintf(str_wtire, "aaaa_%d", a);
	int length = strlen(str_wtire);
	for (int i = 0; i < 4; i++)
	{
		memcpy(bufptr + i * length, str_wtire, length);
		filled_count += length;
	}
	bufptr[filled_count] = '\0';
	filled_count = strlen(bufptr);
	a *= 11;
	free(str_wtire);
	return filled_count;
#endif
}

void fill_ip(char *bufpip){
	static unsigned char a = 1;
	static unsigned char b = 100;
	char *str_wtire = (char *)malloc(16+ 1);
	sprintf(str_wtire, "192.168.%d.%d", a, b);
	memcpy(bufpip, str_wtire, strlen(str_wtire));
	a+=2; b+=10;
	free(str_wtire);
}