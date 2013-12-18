#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>


#define BUFLEN 512
#define NPACK 10
#define PORT 9930
#define SRV_IP "127.0.0.1"


struct sockaddr_in si_other;
int slen = 0;

void diep(char *a){
	perror(a);
	//exit(1);
	return ;
}
unsigned char * encode_int(unsigned char *buffer, int value);
unsigned char * encode_char(unsigned char *buffer, char value);
unsigned char * encode_strcut(unsigned char *buffer, int a, char b);
void send_packet(int s, int a, char b);

int main(int argc, char** argv){

	
	int s, i;
	slen = sizeof(si_other);
	

	if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	if(inet_aton(SRV_IP, &si_other.sin_addr) == 0){
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	for(i = 0;i < NPACK; i++){
		//printf("Sending packet %d\n", i);
		unsigned char str = 'i';
		unsigned int num = 1000 + i;
		
		send_packet(s, num, str);
		
	}
	
	close(s);
	return 0;
}

unsigned char * encode_int(unsigned char *buffer, int value){

	int i = 0;
	for(i = 0;i < 4;i++){
		buffer[i] = value >> (24 - 8 * i);
	}
	return buffer + 4;

}

unsigned char * encode_char(unsigned char *buffer, char value){

	buffer[0] = value;
	return buffer + 1;

}

void send_packet(int s, int a, char b){

	unsigned char buf[32];
	unsigned char *ptr;

	ptr = encode_strcut(buf, a, b);
	
	if(sendto(s, buf, ptr - buf, 0, &si_other, slen) == -1)
		diep("sendto()");

}

unsigned char * encode_strcut(unsigned char *buffer, int a, char b){


	buffer = encode_char(buffer, b);
	buffer = encode_int(buffer, a);
	return buffer;

}