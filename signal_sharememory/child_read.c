/* Filename: shm_read.c */
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

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
struct shmseg *shmp;
void sighup();
void sigint();
void sigquit();
void sigusr1();
void sigusr2();

int main(int argc, char *argv[])
{

	signal(SIGHUP, sighup);
	signal(SIGINT, sigint);
	signal(SIGQUIT, sigquit);
	signal(SIGUSR1, sigusr1);
	signal(SIGUSR2, sigusr2);
	int shmid;
	
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

	/* Transfer blocks of data from shared memory to stdout*/
	while (1)
	{
		info_msg("Child process pid: %d -- ppid: %d\r\n", getpid(), getppid());
		sleep(3);
	}
	info_msg("Reading Process: Reading Done, Detaching Shared Memory\r\n");
	if (shmdt(shmp) == -1)
	{
		perror("shmdt");
		return 1;
	}
	info_msg("Reading Process: Complete\r\n");
	return 0;
}

// sighup() function definition
void sighup()
{
	signal(SIGHUP, sighup); /* reset signal */
	info_msg("CHILD: I have received a SIGHUP\n");
}

// sigint() function definition
void sigint()
{
	signal(SIGINT, sigint); /* reset signal */
	info_msg("CHILD: I have received a SIGINT\n");
}

// sigquit() function definition
void sigquit()
{
	info_msg("My DADDY has Killed me!!!\n");
	exit(1);
}

void sigusr1(){
	printf("segment contains : \r\n\"%s\"\r\n", shmp->buf);
	printf("IP Address: %s - port: %d _ %d _ %d _ %d .\r\n", shmp->ip_address, shmp->port1, shmp->port2, shmp->port3, shmp->port4);
	info_msg("Reading Process: Shared Memory: Read %d bytes\r\n", shmp->cnt);
}

void sigusr2(){

}