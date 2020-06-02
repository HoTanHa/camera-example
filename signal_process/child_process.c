
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define info_msg(fmt, ...) do { printf("[INFO]\t%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__);} while(0)


void sighup();
void sigint();
void sigquit();

int main(void)
{
	int i=1;
	signal(SIGHUP, sighup);
	signal(SIGINT, sigint);
	signal(SIGQUIT, sigquit);
	while (1)
	{
		sleep(1);
		printf("Chill process %d\r\n", i);
		i+=12;
	}
	
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