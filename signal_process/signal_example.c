// C program to implement sighup(), sigint() 
// and sigquit() signal functions 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/wait.h>
  
#define info_msg(fmt, ...) do { printf("[INFO]\t%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__);} while(0)
#define IGNORE_SIGCHILD 0
int count=0;
// function declaration 
void sighup(); 
void sigint(); 
void sigquit(); 
void sigint_parent(); 
void sigchild(int signum){
	int status;
	pid_t pid_child;
	pid_child = wait(&status);
	printf("SIG_Child: %d ---- %d\r\n", pid_child, status);
}
// driver code 
void main() 
{ 
    int pid1, pid2; 
#if IGNORE_SIGCHILD
	signal(SIGCHLD, SIG_IGN); //bo qua tin hieu process con
#else
	signal(SIGCHLD, sigchild); //nhan tin hieu process con
#endif
    /* get child process */
    if ((pid1 = fork()) < 0) { 
        perror("fork"); 
        exit(1); 
    } 
  
    if (pid1 == 0) { /* child */
        signal(SIGHUP, sighup); 
        signal(SIGINT, sigint); 
        signal(SIGQUIT, sigquit); 
        for (;;) 
            ; /* loop for ever */
    } 
    else /* parent */
    { /* pid hold id of child */
		// signal(SIGINT, SIG_DFL);
        
        printf("\nPARENT: sending SIGHUP\n\n"); 
        kill(pid1, SIGHUP); 
  
        sleep(2); /* pause for 3 secs */
        printf("\nPARENT: sending SIGINT\n\n"); 
        kill(pid1, SIGINT); 
  
        sleep(2); /* pause for 3 secs */
        printf("\nPARENT: sending SIGQUIT\n\n"); 
        kill(pid1, SIGQUIT); 
        sleep(1); 
    } 

	if ((pid2 = fork()) < 0) { 
        perror("fork"); 
        exit(1); 
    } 
    if (pid2 == 0) { /* child */
#ifndef IMX
        execl("child_prs.o", (char*)NULL);
#else
         execl("child_process.out", "child_process", (char*)NULL);
#endif
    } 
    else {
		signal(SIGINT, sigint_parent); 
        while (count==0) {
			sleep(1);
			printf("aaaaa111\r\n");
		}
        kill(pid2, SIGHUP); 
		while (count==1) {

			sleep(1);
			info_msg("aaa222\r\n");
		}
        kill(pid2, SIGINT);
		while (count==2) {

			sleep(1);
			info_msg("aaaa333\r\n");
		}
        kill(pid2, SIGQUIT);
    }
	while (1){
		sleep(1);
			info_msg("aaaa44\r\n");
	}
    
} 
  
// sighup() function definition 
void sighup() { 
    signal(SIGHUP, sighup); /* reset signal */
    printf("CHILD: I have received a SIGHUP\n"); 
} 
  
// sigint() function definition 
void sigint() { 
    signal(SIGINT, sigint); /* reset signal */
    printf("CHILD: I have received a SIGINT\n"); 
} 
  
// sigquit() function definition 
void sigquit() { 
    printf("My DADDY has Killed me!!!\n"); 
    exit(0); 
} 

void sigint_parent(){
    if (count<3) signal(SIGINT, sigint_parent);
	else exit(0);
    count++;
}