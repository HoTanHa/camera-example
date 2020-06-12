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
// and sigquit() signal functions
#include <signal.h>
#include <sys/wait.h>

#define BUF_SIZE_SHM 1024
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
	char buf[BUF_SIZE_SHM];
	char ip_address[16];
	int port1;
	int port2;
	int port3;
	int port4;
};
int fill_buffer(char *bufptr, int size);
void fill_ip(char *bufpip);
int count = 0;
int sigint = 0;
int sigusr1_p=0;
int sigusr2_p=0;
void sigint_parent()
{
	if (count < 3)
		signal(SIGINT, sigint_parent);
	else
		exit(0);
	count++;
	sigint=1;
}
void sigusr1_parent(){
	sigusr1_p=1;
}
void sigusr2_parent(){
	sigusr2_p=1;
}
void sigchild(int signum)
{
	int status;
	pid_t pid_child;
	pid_child = wait(&status);
	printf("SIG_Child: %d ---- %d\r\n", pid_child, status);
}

int main(int argc, char *argv[])
{

	int pid1;
	signal(SIGCHLD, sigchild); //nhan tin hieu process con

	if ((pid1 = fork()) < 0)
	{
		perror("fork");
		exit(1);
	}
	if (pid1 == 0)
	{ /* child */
		// execl("child_read.out", "child_process", (char *)NULL);
		execl("mxc_vpu_test.out", "process2", "-E", "-x 0 -w 1280 -h 720 -c 10000 -f 2", (char *)0);
	}

	signal(SIGINT, sigint_parent);
	signal(SIGUSR1, sigusr1_parent);
	signal(SIGUSR2, sigusr2_parent);
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
	spaceavailable = BUF_SIZE_SHM;
	
	while (1){
		info_msg("PARRENT pid: %d\r\n", getpid());
		if (sigusr1_p){
			shmp->cnt = fill_buffer(bufptr, spaceavailable);
			fill_ip(buffip);
			shmp->complete = 0;
			info_msg("Writing Process: Shared Memory Write: Wrote %d bytes\n", shmp->cnt);
			bufptr = shmp->buf;
			buffip = shmp->ip_address;
			// shmp->port1 += 1;
			shmp->port2 += 2;
			shmp->port3 += 3;
			shmp->port4 += 4;
			spaceavailable = BUF_SIZE_SHM;
			kill(pid1, SIGUSR1);
			sigusr1_p=0;
		}
		if (sigusr2_p){
			kill(pid1, SIGUSR2);
			sigusr2_p=0;
		}
		if (sigint){
			if (count==1){
				// kill(pid1, SIGHUP);
			}
			if (count==2){
				kill(pid1, SIGINT);
			}
			if (count==3){
				kill(pid1, SIGQUIT);
				sigint = 0;
				break;
			}
			sigint=0;
		}
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

void fill_ip(char *bufpip)
{
	static unsigned char a = 40;
	static unsigned char b = 100;
	char *str_wtire = (char *)malloc(16 + 1);
	sprintf(str_wtire, "192.168.%d.%d", a, b);
	memcpy(bufpip, str_wtire, strlen(str_wtire));
	// a += 2;
	// b += 10;
	free(str_wtire);
}