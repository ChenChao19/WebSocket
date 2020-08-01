/*******************************
udp_client.c: the source file of the client in udp transmission 
********************************/

#include "headsock.h"

//calculate time interval between out and in
void tv_sub(struct timeval *out, struct timeval *in)
{
        if ((out->tv_usec -= in->tv_usec) <0)
        {
                --out ->tv_sec;
                out ->tv_usec += 1000000;
        }
        out->tv_sec -= in->tv_sec;
}

//transmission and receive function
float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len) {	
	char *buf;
        long lsize, ci;
        char sends[DATALEN];
        struct ack_so ack;
        int m, n, slen;
        float time_inv = 0.0;
        struct timeval sendt, recvt;
        ci = 0;

	fseek (fp , 0 , SEEK_END);
        lsize = ftell (fp);
        rewind (fp);
        printf("The file length is %d bytes\n", (int)lsize);
        printf("the packet length is %d bytes\n",DATALEN);

	 // allocate memory to contain the whole file.
        buf = (char *) malloc (lsize);
        if (buf == NULL) exit (2);
        fread (buf,1,lsize,fp);//copy the file into the buffer
        buf[lsize] ='\0';//append the end byte with '\0'

	//get the current time, this is the start of the trasnmission
        gettimeofday(&sendt, NULL);

	//transmission process with stop and wait protocal
	while(ci <= lsize) {
		//packet sent are in size of DATALEN except for the last packet
		if(lsize + 1 - ci <= DATALEN) {
			slen = lsize + 1 - ci;
		} else {
			slen = DATALEN;
		}
		memcpy(sends, (buf+ci), slen);

		//send the data and return the number of bytes being sent
		m = sendto(sockfd, &sends, slen, 0, addr, addrlen);
		if(m == -1) {//error handling code for sending
                        printf("error! when sending");
                        exit(1);
                }

		//client wait for ack/nack
		n = recvfrom(sockfd, &ack, 2, 0, addr, &addrlen);
		if(n == -1) {//error handling code for receiving
			printf("error when receiving\n");
			exit(1);
		}

		//nack handler
		if(ack.num == 0 && ack.len == 0) {
			//printf("retransmission required\n");
			continue; //ci += slen not executed, retransmission
		}

		//error in transmission
		if (ack.num != 1 || ack.len != 0) {
			printf("error in transmission\n");
	
		}
		ci += slen;
	}

	//get the current time, this is the end of the transmission
	gettimeofday(&recvt, NULL);
	*len = ci;
	//calculate total transmission time
        tv_sub(&recvt, &sendt);
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
        return(time_inv);
}

int main(int argc, char **argv)
{
        int sockfd, ret;
        float ti, rt;
        long len;
        struct sockaddr_in ser_addr;
        char ** pptr;
        struct hostent *sh;
        struct in_addr **addrs;
        FILE *fp;

	//error handling code for command line parameter
        if (argc != 2) {
                printf("parameters not match");
        }

	//get host's information
        sh = gethostbyname(argv[1]);
        if (sh == NULL) { //error handling code for no host found
                printf("error when gethostby name");
                exit(0);
        }

	//print the remote hosts's information
	printf("canonical name: %s\n", sh->h_name);
        for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
                printf("the aliases name is: %s\n", *pptr);
        switch(sh->h_addrtype)
        {
                case AF_INET:
                        printf("AF_INET\n");
                break;
                default:
                        printf("unknown addrtype\n");
                break;
        }

	//create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) { //error handling code for socket
                printf("error in socket");
                exit(1);
        }

	//get the server address
	addrs = (struct in_addr **)sh->h_addr_list;
	ser_addr.sin_family = AF_INET;
        ser_addr.sin_port = htons(MYUDP_PORT);
        memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
        bzero(&(ser_addr.sin_zero), 8);

	//open the file
	fp = fopen("myfile.txt", "r+t");
	if(fp == NULL) {//error handling code for file opening
                printf("File doesn't exit\n");
                exit(0);
        }
	
	//Transmit and receive
	ti = str_cli(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len); //return total time taken for transission
	rt = (len/(float)ti) / 1000;//caculate the average transmission rate
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Mbytes/s)\n", ti, (int)len, rt);

	//close socket, file and exit
	close(sockfd);
	fclose(fp);
	exit(0);
}
