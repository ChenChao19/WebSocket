/**************************************
udp_ser.c: the source file of the server in udp transmission
**************************************/
#include "headsock.h"

int compare(const void *a, const void *b) {
	int int_a = *((int*)a);
	int int_b = *((int*)b);
	if(int_a == int_b) return 0;
	else if (int_a < int_b) return -1;
	else return 1;
}

int main(int argc, char *argv[]) { 
	int sockfd, ret;
        struct sockaddr_in my_addr;
	char buf[BUFSIZE];
        FILE *fp;
        char recvs[DATALEN];
        struct ack_so ack;
        int end, m, n = 0;
	int len = sizeof(struct sockaddr_in);
        long lseek = 0;

	//create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1) {//error handling code for socket
                printf("error in socket");
                exit(1);
        }

	//server address
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYUDP_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY
	bzero(&(my_addr.sin_zero), 8);
	
	//bind socket
	if(bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
		printf("error in binding");
		exit(1);
	}

	//start receiving and transmitting ack/nack
       	printf("receiving data!\n");

	//one packet is drop per RAND_DROP number of packets received
	//Range of rand_no is [0, RAND_DROP - 1]
	//Rate of dropping/Frame Error Probability is 100 / RAND_DROP %

	while(!end) {
		//Range of counter is also [0, RAND_DROP]

		//receive the data in the length of DATALEN
		m = recvfrom(sockfd, &recvs, DATALEN, 0, (struct sockaddr *)&my_addr, &len);
		if(m == -1) {//error handling code for receiving
			printf("error when receiving\n");
                        exit(1);
		}

		//randomly drop packets if rand
		double random = (rand() % 1000) / 1000.0;
		if(random < ERROR_PROB) { //drops the packet and send nack
			ack.num = 0;
			ack.len = 0;
		} else { //accepts the packet and send ack
			
			if(recvs[m-1] == '\0') {
                       		end = 1;
                        	m--;
                	}

			memcpy((buf+lseek), recvs, m);
			lseek += m;
			ack.num = 1;
			ack.len = 0;
		}

		//send ack/nack to client
		n = sendto(sockfd, &ack, 2, 0, (struct sockaddr *)&my_addr, len);
		if(n == -1) {//error handling code for sending
			printf("error when sending\n");
			exit(1);
		}
	}

	//write the data from buffer into a file
	fp = fopen("myUDPreceive.txt", "wt");
	if (fp == NULL) {
                printf("File doesn't exit\n");
                exit(0);
        }
	fwrite (buf , 1 , lseek , fp);

	//close the file
	fclose(fp);

	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);

	//close socket and exit
	close(sockfd);
        exit(0);
}
