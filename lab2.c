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
#include<pthread.h> //for threading , link with lpthread
 
//the thread function
void *connection_handler(void *);
int count = 0;
struct arg{
	int clisock;
	char argv3[];
};

int main(int argc , char *argv[])
{
        
	if (!strcmp(argv[1], "multithread")) {    
 
	int socket_desc , client_sock , c, numbytes;
    	struct sockaddr_in server , client;
	socklen_t clilen;
	FILE *fp;
	char buffer[1024];
	
     //Prepare the sockaddr_in structure
	bzero((char *) &server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[3]);
    server.sin_port = htons(atoi(argv[4]));
    bzero(buffer, 1024);

	//Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
		if(!strcmp(argv[2], "server")) {

	struct arg args;

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {   
        puts("Connection accepted");
         
		args.clisock = client_sock;
		strcpy(args.argv3, argv[5]);

        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &args) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
		count++;
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
	}//server
	
	else if(!strcmp(argv[2], "client")) {
		int recv_rec = 0, cfSize, numbytes;
		/* connect */
            if (connect(socket_desc,(struct sockaddr *) &server,sizeof(server)) < 0) 
                puts("ERROR connecting");
    	/* receive file name */
			read(socket_desc, buffer, 1024);
			printf("FileName received: %s.\n", buffer);
			fp = fopen(buffer, "wb");
			bzero(buffer, 1024);
			/* receive file size */
			read(socket_desc, buffer, 1024);
			cfSize = atoi(buffer);
			printf("FileSize received: %d bytes.\n", cfSize);
			bzero(buffer, 1024);
			/* receive */
			while(1){
				numbytes = read(socket_desc, buffer, sizeof(buffer));
				if(numbytes == 0) break;
				numbytes = fwrite(buffer, sizeof(char), numbytes, fp);
			}
			printf("Receiving finished!\n");
	}//client
	fclose(fp);
    }//mutithread
	else if(!strcmp(argv[1], "multicast")) {
	if(!strcmp(argv[2], "server")) {

	struct in_addr localInterface;
	struct sockaddr_in groupSock;
	int sd, numbytes;
	char buffer[1024];
	char *END_FLAG = "-----傳送完畢-----";
	FILE *fp;

	/* Create a datagram socket on which to send. */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
	  perror("Opening datagram socket error");
	  exit(1);
	}
	else
	  printf("Opening the datagram socket...OK.\n");
	 
	/* Initialize the group sockaddr structure with a */
	/* group address of 225.1.1.1 and port 5555. */
	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr("226.1.1.1");
	groupSock.sin_port = htons(atoi(argv[4]));
	 
	/* Disable loopback so you do not receive your own datagrams.
	{
	char loopch = 0;
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)
	{
	perror("Setting IP_MULTICAST_LOOP error");
	close(sd);
	exit(1);
	}
	else
	printf("Disabling the loopback...OK.\n");
	}
	*/
	 
	/* Set local interface for outbound multicast datagrams. */
	/* The IP address specified must be associated with a local, */
	/* multicast capable interface. */
	localInterface.s_addr = inet_addr(argv[3]);
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
	{
	  perror("Setting local interface error");
	  exit(1);
	}
	else
	  printf("Setting the local interface...OK\n");
	/* Send a message to the multicast group specified by the*/
	/* groupSock sockaddr structure. */
	/*int datalen = 1024;*/
	//send message	
	/*if(sendto(sd, databuf, datalen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0)
	{
		perror("Sending datagram message error");
	}
	else
	  printf("Sending datagram message...OK\n");*/

    		int send_rec = 0, sfSize;
			printf("Starting file transfer...\n"); 		
            /* prepare file to send */
			fp = fopen(argv[5], "rb");
			fseek(fp, 0, SEEK_END);
			sfSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			/* send file name */
			printf("FileName sent: %s.\n", argv[5]);
			sendto(sd, argv[5], strlen(argv[5]), 0, (struct sockaddr *) &groupSock, sizeof(groupSock));
			sleep(1);
			/* send file size */
			sprintf(buffer, "%d", sfSize);
			sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *) &groupSock, sizeof(groupSock));
			printf("FileSize sent: %s bytes.\n", buffer);
			bzero(buffer, 1024);
			sleep(1);
			while ((numbytes = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
        		sendto(sd, buffer, numbytes, 0, (struct sockaddr *) &groupSock, sizeof(groupSock));
        		//printf("Send %d bytes.\n", numbytes);
    		}
    		sleep(1);
    		sendto(sd, END_FLAG, strlen(END_FLAG), 0, (struct sockaddr *) &groupSock, sizeof(groupSock));
    		printf("Sending finished!\n");
    	//}
	 
	/* Try the re-read from the socket if the loopback is not disable
	if(read(sd, databuf, datalen) < 0)
	{
	perror("Reading datagram message error\n");
	close(sd);
	exit(1);
	}
	else
	{
	printf("Reading datagram message from client...OK\n");
	printf("The message is: %s\n", databuf);
	}
	*/
	return 0;
	}//server
	else if(!strcmp(argv[2], "client")) {
	struct sockaddr_in localSock;
	struct ip_mreq group;
	int sd, numbytes;
	int datalen;
	char buffer[1024];
	bzero(buffer, 1024);
	socklen_t clilen;
	FILE *fp;
	char *END_FLAG = "-----傳送完畢-----";

	/* Create a datagram socket on which to receive. */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
		perror("Opening datagram socket error");
		exit(1);
	}
	else
	printf("Opening datagram socket....OK.\n");
		 
	/* Enable SO_REUSEADDR to allow multiple instances of this */
	/* application to receive copies of the multicast datagrams. */
	{
		int reuse = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
	{
		perror("Setting SO_REUSEADDR error");
		close(sd);
		exit(1);
	}
	else
		printf("Setting SO_REUSEADDR...OK.\n");
	}
	 
	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
	memset((char *) &localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons(atoi(argv[4]));
	localSock.sin_addr.s_addr = INADDR_ANY;
	if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)))
	{
		perror("Binding datagram socket error");
		close(sd);
		exit(1);
	}
	else
		printf("Binding datagram socket...OK.\n");
	 
	/* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
	/* interface. Note that this IP_ADD_MEMBERSHIP option must be */
	/* called for each local interface over which the multicast */
	/* datagrams are to be received. */
	group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
	group.imr_interface.s_addr = inet_addr(argv[3]);
	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
	{
		perror("Adding multicast group error");
		close(sd);
		exit(1);
	}
	else
		printf("Adding multicast group...OK.\n");
	 
	/* Read from the socket. */
	/*datalen = sizeof(databuf);
	if(read(sd, databuf, datalen) < 0)
	{
		perror("Reading datagram message error");
		close(sd);
		exit(1);
	}
	else
	{
		printf("Reading datagram message...OK.\n");
		printf("The message from multicast server is: \"%s\"\n", databuf);
	}*/

    		int recv_rec = 0, cfSize;
    		
    		clilen = sizeof(localSock);
    		printf("Waitting for file transfer...\n");
    		/* receive file name */
    		recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&localSock, &clilen);
			printf("FileName received: %s.\n", buffer);
			fp = fopen(buffer, "wb");
			bzero(buffer, 1024);
			/* receive file size */
			recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&localSock, &clilen);
			cfSize = atoi(buffer);
			printf("FileSize received: %d bytes.\n", cfSize);
			bzero(buffer, 1024);
    		while (numbytes = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&localSock, &clilen)) {
        		if (!(strcmp(buffer, END_FLAG))) break;
        		recv_rec += numbytes;
				//printf("Receive %d bytes.\n", numbytes);
        		fwrite(buffer, sizeof(char), numbytes, fp);
        		bzero(buffer, numbytes);
    		}
    	printf("Receiving finished!\n");
    	printf("Packet Loss Rate is %.2f %%.\n", ((double) (cfSize-recv_rec)/cfSize)*100);
	return 0;
	}//client
	}//multicast
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *argument)
{
	struct arg *args = argument;
	FILE *fp;	
	int  send_rec = 0, sfSize, numbytes;
	int sock = (args -> clisock);
	char buffer[1024];
	while(count<3);
	/* prepare file to send */
			fp = fopen(args -> argv3, "rb");
			fseek(fp, 0, SEEK_END);
			sfSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			/* send file name */
			//printf("FileName sent: %s.\n", args -> argv3);
			write(sock, args -> argv3, strlen(args -> argv3));
			sleep(1); 
			/* send file size */
			sprintf(buffer, "%d", sfSize);
			write(sock, buffer, strlen(buffer));
			//printf("FileSize sent: %s bytes.\n", buffer);
			bzero(buffer, 1024);
			sleep(1);
			/* send */
			while(!feof(fp)){
				numbytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
				numbytes = write(sock, buffer, numbytes);
			}
	//close(sock);
    return 0;
} 
