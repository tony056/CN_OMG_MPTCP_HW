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
#define NAMELENGTH 100

struct sockaddr_in si_me, si_other;
int slen = 0, s, connection = -1, end = -1, expectNum = 1;

void diep(char *a){
	perror(a);
	//exit(1);
	return ;
}

void decode_buffer(unsigned char *buffer);
void decode_fileContent(unsigned char *buffer);
unsigned int decode_int(unsigned char *buffer);
char* decode_char(unsigned char *buffer);
int send_ack(int type, int sequenceNum);

int main(int argc, char** argv){

	
	int i;
	slen = sizeof(si_other);
	unsigned char buf[BUFLEN];

	if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(s, &si_me, sizeof(si_me)) == -1)
		diep("bind");

	while(end < 0){

		if(recvfrom(s, buf, BUFLEN, 0, &si_other, &slen) == -1)
			diep("recvfrom");
		if(connection < 0)
			decode_buffer(buf);
		else{

			decode_fileContent(buf);

		}

	}

	close(s);
	return 0;
	
}

void decode_buffer(unsigned char *buffer){

	printf("Start decoding the packet!!\n");
	
	unsigned int ans = decode_int(buffer);
	buffer +=4;
	//unsigned int langth = decode_int(buffer);
	unsigned char *str = malloc(sizeof(char) * NAMELENGTH);
	str = decode_char(buffer); 
	
	printf("decoded buffer is: %s, %d\n",  str, ans);

	while(send_ack(1, -1) < 0){

		printf("failed ack\n");

	}
	connection = 1;
	
	return;

}

unsigned int decode_int(unsigned char *buffer){

	unsigned int ans = 0;
	int i = 0;

	for(i = 0; i < 4;i++){
		ans += buffer[i];
		if(i != 3) ans = ans << 8;
	}
	 
	return ans;

}

char* decode_char(unsigned char *buffer){

	char str[NAMELENGTH];
	int i = 0;

	while(buffer[i] != NULL){
		str[i] = buffer[i];
		i++;
	}
	str[i] = '\0';
	
	return str;
	
}

int send_ack(int type, int sequenceNum){

	if(type == 1){ //recv connection packet
		unsigned char ack[2];
		ack[0] = '1';
		if(sendto(s, ack, sizeof(char) * 1, 0, &si_other, slen) == -1)
			return -1;
	}else if(type == 2){

	}

	printf("ack\n");
	return 1;

}

void decode_fileContent(unsigned char *buffer){
	char fuck[2];
	fuck[0] = '1';
	fuck[1] = '\0';
	if(strlen(buffer) == 1){ // send back fin

		while(sendto(s, fuck, sizeof(char) * 1, 0, &si_other, slen) < 0){}
		end = 1;
	}else{
		int sequence = decode_int(buffer);
		if(sequence == expectNum){
			buffer += 4;
			int length = decode_int(buffer);
			buffer += 4;
			char data[length];
			data = decode_char(buffer);
			printf("Sequence: %d, %s\n",sequence, data);
			send_ack(2, expectNum);
			expectNum += length;
		}else{
			send_ack(2, expectNum);
		}

	}
	printf("end the connection\n");
}
