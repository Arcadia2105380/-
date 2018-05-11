#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#define MAXLEN 1024

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void delay(unsigned int secs) 
{
	unsigned int retTime = time(0) + secs;   
	while (time(0) < retTime);               
}

void timelog(int *n)
{
	time_t timep;
    struct tm *p;
	time(&timep);
    p = gmtime(&timep);
    if((p->tm_hour)+8 >= 24)
    	p->tm_hour -= 24;
	printf("%d%%  %d/%d/%d %d:%d:%d\n", *n, (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, (p->tm_hour)+8, p->tm_min, p->tm_sec);
	*n += 5;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, numbytes, n = 5;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[MAXLEN];
    char *END_FLAG = "===END===";
    FILE *fp;
	/* server info */ 
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[3]);
    serv_addr.sin_port = htons(atoi(argv[4]));
    bzero(buffer, MAXLEN);
    
    if (!strcmp(argv[1], "tcp")) {
    	/* open socket */    	
    	sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");
		if (!strcmp(argv[2], "send")) {
			int send_rec = 0, sfSize;            
            /* bind */ 
            if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
                error("ERROR on binding");
            /* listen & accept*/
            listen(sockfd,5);
            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0)
                error("ERROR on accept");
            printf("Starting file transfer...\n");
            /* prepare file to send */
			fp = fopen(argv[5], "rb");
			fseek(fp, 0, SEEK_END);
			sfSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			/* send file name */
			printf("FileName sent: %s.\n", argv[5]);
			write(newsockfd, argv[5], strlen(argv[5]));
			delay(1); 
			/* send file size */
			sprintf(buffer, "%d", sfSize);
			write(newsockfd, buffer, strlen(buffer));
			printf("FileSize sent: %s bytes.\n", buffer);
			bzero(buffer, MAXLEN);
			delay(1);
			/* send */
			while(!feof(fp)){
				numbytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
				numbytes = write(newsockfd, buffer, numbytes);
				send_rec += numbytes;
				while(send_rec >= sfSize/20) {
					printf("Sending ");
					timelog(&n);
					send_rec -= (sfSize/20);
				}
			}
			printf("Sending finished!\n");
            close(newsockfd);         	
		}
		else if (!strcmp(argv[2], "recv")) {
			int recv_rec = 0, cfSize;
            /* connect */
            if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
                error("ERROR connecting");
            printf("Waitting for file transfer...\n");
            /* receive file name */
			read(sockfd, buffer, MAXLEN);
			printf("FileName received: %s.\n", buffer);
			fp = fopen(buffer, "wb");
			bzero(buffer, MAXLEN);
			/* receive file size */
			read(sockfd, buffer, MAXLEN);
			cfSize = atoi(buffer);
			printf("FileSize received: %d bytes.\n", cfSize);
			bzero(buffer, MAXLEN);
			/* receive */
			while(1){
				numbytes = read(sockfd, buffer, sizeof(buffer));
				if(numbytes == 0) break;
				recv_rec += numbytes;
				while(recv_rec >= cfSize/20) {
					printf("Receiving ");
					timelog(&n);
					recv_rec -= (cfSize/20);
				}
				numbytes = fwrite(buffer, sizeof(char), numbytes, fp);
			}
			printf("Receiving finished!\n");
		}
		fclose(fp);
	}
	else if (!strcmp(argv[1], "udp")) {
		/* UDP setting */
    	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");
    	if (!strcmp(argv[2], "send")) {
    		int send_rec = 0, sfSize;
			printf("Starting file transfer...\n"); 		
            /* prepare file to send */
			fp = fopen(argv[5], "rb");
			fseek(fp, 0, SEEK_END);
			sfSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			/* send file name */
			printf("FileName sent: %s.\n", argv[5]);
			sendto(sockfd, argv[5], strlen(argv[5]), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
			delay(1);
			/* send file size */
			sprintf(buffer, "%d", sfSize);
			sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
			printf("FileSize sent: %s bytes.\n", buffer);
			bzero(buffer, MAXLEN);
			delay(1);
			while ((numbytes = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
        		sendto(sockfd, buffer, numbytes, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        		printf("Send %d bytes.\n", numbytes);
    		}
    		delay(1);
    		sendto(sockfd, END_FLAG, strlen(END_FLAG), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    		printf("Sending finished!\n");
    	}
    	else if (!strcmp(argv[2], "recv")) {
    		int recv_rec = 0, cfSize;
    		/* bind */
    		if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        		error("bind error");
    		clilen = sizeof(cli_addr);
    		printf("Waitting for file transfer...\n");
    		/* receive file name */
    		recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen);
			printf("FileName received: %s.\n", buffer);
			fp = fopen(buffer, "wb");
			bzero(buffer, MAXLEN);
			/* receive file size */
			recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen);
			cfSize = atoi(buffer);
			printf("FileSize received: %d bytes.\n", cfSize);
			bzero(buffer, MAXLEN);
    		while (numbytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen)) {
        		if (!(strcmp(buffer, END_FLAG))) break;
        		recv_rec += numbytes;
				printf("Receive %d bytes.\n", numbytes);
        		fwrite(buffer, sizeof(char), numbytes, fp);
        		bzero(buffer, numbytes);
    		}
    	printf("Receiving finished!\n");
    	printf("Packet Loss Rate is %.2f %%.\n", ((double) (cfSize-recv_rec)/cfSize)*100);
		}	
	}
	close(sockfd);
	return 0;
}

