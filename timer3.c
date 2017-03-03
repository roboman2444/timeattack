#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <limits.h>

struct sockaddr_in serv_addr;
int error = 0;

int thesock = 0;


/*
Welcome to Secure Signed Shell
1) sign command
2) execute command
>_ 2
what command do you want to run?
>_ sh
gimme signature:
>_ 93c8c70ed1a1ef4c7420a3c229411e96
wrong signature for sh

*/

char readbuff[1024];
int readplace = 0;




int pushdata(int n, long long int time){
	fprintf(stderr, "%i\t%lli\n", n, time);
}



int readendswith(char * str){
	int b1, b2;
	//find end of str
	for(b1 = 0; str[b1]; b1++);
	//find end of read
	for(b2=0; readbuff[b2]; b2++);
	//compare

	while(b1 && b2 && readbuff[b2--] == str[b1--]);
	return (!b1);
}
int readendswithfast(char * str, int strend, int readend){
	int b1 = strend, b2 = readend;
	while(b1 && b2 && readbuff[b2--] == str[b1--]);
	return (!b1);
}

int resetread(void){
	readbuff[0] = 0;
	readplace = 0;
}


int readtill1(void){
	int n;
	while ( (n = read(thesock, readbuff+readplace, 1)) > 0){
//		printf("%s\n", readbuff);
		readbuff[n+readplace] = 0;
		readplace+=n;
		if(readendswithfast(": ", 2, readplace)) break;
	}
//	printf("d %s\n", readbuff);
}
int readtill2(void){
	int n;
	while ( (n = read(thesock, readbuff+readplace, 1)) > 0){
		readbuff[n+readplace] = 0;
		readplace+=n;
		if(readendswithfast("? ", 2, readplace)) break;
	}
//	printf("d %s\n", readbuff);
}







int reconnect(int extra){
	printf("Reconnecting %i\n", extra);
	if(thesock) close(thesock);
	thesock = 0;
	if((thesock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("\n Error : Could not create socket \n");
		return 0;
	}
	if( connect(thesock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		printf("\n Error : Connect Failed \n");
		return 0;
	}
	error = 0;
	return 1;

}




struct timespec diff(struct timespec start, struct timespec end){
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}



char * signbuffer = 0;// "00000000000000000000000000000000";
int signplace = 0;
long long signtime(int len){
	struct timespec start_time;
	struct timespec end_time;
	sprintf(signbuffer, "%i", len);
	int l = strlen(signbuffer);
	if(send(thesock, signbuffer, l, MSG_NOSIGNAL) != l) error = 1;

//	usleep(200);
	clock_gettime(CLOCK_REALTIME, &start_time);
	//HERE WE GO
	if(send(thesock, "\n", 1, MSG_NOSIGNAL) < 0 ){ error = 1;}
	read(thesock, readbuff, 1);
	//done
	clock_gettime(CLOCK_REALTIME, &end_time);
	struct timespec d = diff(start_time, end_time);
//	printf("Duration %i\n", diffInNanos);
	return d.tv_sec * 1000000000 + d.tv_nsec;
}

int doshit(void){
		int i, j;
		for(i = 1; i < 16; i++){
		for(j = 0; j < 1000; j++){
				resetread();
				readtill1();
				if(error){
					reconnect(j);
					i--;
					continue;
				}
				resetread();
				if(send(thesock, "a\n", 2, MSG_NOSIGNAL) != 2) error = 1;
				if(error){
					reconnect(j);
					i--;
					continue;
				}
				readtill2();
				if(error){
					reconnect(j);
					i--;
					continue;
				}
				resetread();
				long long signres = signtime(1000 * (2 << i));
				pushdata(1000 * (2 << i), signres);
//				/*if(signres >= 0)*/ tracker[i].run += signres;
//				else error = 1;
				if(error){
					reconnect(j);
					i--;
					continue;
				}
				readtill2();
				if(error){
					reconnect(j);
					i--;
					continue;
				}
				resetread();
				if(send(thesock, "y\n", 2, MSG_NOSIGNAL) != 2) error = 1;
				if(error){
					reconnect(j);
					i--;
					continue;
				}

			}
		reconnect(-i);
		}
/*
		//find the smallest
		long long int large = 0;
		int lgindex;
		for(i =0; i < 16; i++){
			signbuffer[signplace] = tracker[i].c;
//			printf("Run for %s was %i\n", signbuffer, tracker[i].win);
			printf("Run for %s was %i\n", signbuffer, tracker[i].run);
			if(tracker[i].run > large){
				large = tracker[i].run;
				lgindex = i;
			}
			tracker[i].run = 0;
			tracker[i].win = 0;
		}
		signbuffer[signplace] = tracker[lgindex].c;
		printf("winner was %s with %f\n", signbuffer, (double)large/(double)j);
*/

	printf("done\n");
	return 1;
}


int main(int argc, char *argv[]){
	printf("fuck\n");
	signbuffer = malloc(100);
//	ranshuf();
	sprintf(signbuffer, "%s", "1000");
//	sprintf(signbuffer, "%s", "0");
	int sockfd = 0, n = 0;
	if(argc != 3){
		printf("\n Usage: %s <ip of server> <port> \n",argv[0]);
		return 1;
	}
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("\n Error : Could not create socket \n");
		return 1;
	}
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoll(argv[2]));
	if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0){
		printf("\n inet_pton error occured\n");
		return 1;
	}
	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		printf("\n Error : Connect Failed \n");
		return 1;
	}
	thesock = sockfd;
	doshit();
	return 0;
}
