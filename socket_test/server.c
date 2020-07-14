
// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 


#include <signal.h>

#define PORT     6001
#define MAXLINE 1024 
#define Portcam0 8010
  
// Driver code 
int sockfd;

int send_socket_buffer(int socket_fd, uint16_t port, char *buff, int len)
{
	if (socket_fd <= 0)
		return -1;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int n = 0;
	n = sendto(socket_fd, buff, len, MSG_CONFIRM, (struct sockaddr *)&addr,
			   sizeof(addr));
	if (len != n)
	{
		printf("send socket: error\n");
	}

	return n;
}



sigset_t sigset;
static int
signal_thread(void *arg)
{
	int sig;

	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	while (1)
	{
		 sigwait(&sigset, &sig);
		if (sig == SIGINT)
		{
			printf("Ctrl-C received\n");
			break;
		}
		else if (sig == SIGUSR1)
		{
			send_socket_buffer(sockfd, Portcam0, "stream:ON", 9);
		}
		else if (sig == SIGUSR2)
		{
			send_socket_buffer(sockfd, Portcam0, "stream:OFF", 10);
		}
		else if (sig == SIGHUP)
		{
			send_socket_buffer(sockfd, Portcam0, "memory:OK", 9);
		}
		else
		{
			printf("Unknown signal. Still exiting\n");
			break;
		}

	}

	// pthread_exit(0);
	exit(0);
}
int main() { 
     
    char buffer[MAXLINE]; 
    char *hello = "Hello from server"; 
    struct sockaddr_in servaddr, cliaddr; 

	pthread_t sigtid;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);
	sigaddset(&sigset, SIGHUP);

	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	pthread_create(&sigtid, NULL, (void *)&signal_thread, NULL);
      
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    int len, n; 
  
    len = sizeof(cliaddr);  //len is value/resuslt 
    while(1) {
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                &len); 
    buffer[n] = '\0'; 
    printf("Client : %s\n", buffer); 

    sendto(sockfd, (const char *)hello, strlen(hello),  
        MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
            len); 
    }

    sendto(sockfd, (const char *)hello, strlen(hello),  
        MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
            len); 
    printf("Hello message sent.\n");  
      
    return 0; 
} 