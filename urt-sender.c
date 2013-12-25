#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>

#define BUFLEN 512
#define NPACK 10
#define PORT 9930
#define SRV_IP "127.0.0.1"
#define LENGTH 1000

// header : add ip addr + port + filelength + flag;
// agent: change destination 
struct sockaddr_in si_other;
int slen = 0, nameLength = 0, fin = -1, ackType = -1, expectNum = 0, resend = 0;

void diep(char *a){
	perror(a);
	//exit(1);
	return ;
}
void sighandler(int signo){
	resend = 1;
}
unsigned char * encode_int(unsigned char *buffer, int value);
unsigned char * encode_char(unsigned char *buffer, char *value);
unsigned char * encode_struct(unsigned char *buffer, int a, char *b);
unsigned char * encode_data_struct(unsigned char *buffer, int length, int sequence, char* data);
void send_connection_packet(char *filename, int s);
void send_data_packet(char *fileName, int s);
void send_end_packet(int s);
int decode_ack(char *ack);
unsigned int decode_int(unsigned char *buffer);


int main(int argc, char** argv){

	
	int s, i;
	slen = sizeof(si_other);
	char fileName[1000];
	struct sigaction act;
	act.sa_handler = sighandler;
	act.sa_flags = 0;


	strcpy(fileName, argv[1]);
	nameLength = strlen(fileName);
	if(argc != 2)
		diep("please enter filename");
	if(sigaction(SIGALRM, &act, 0) == -1)
		perror("sigaction\n");

	if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	if(inet_aton(SRV_IP, &si_other.sin_addr) == 0){
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	while(fin < 0){
		if(ackType == -1)
			send_connection_packet(fileName, s);
		else if(ackType == 1)
			send_data_packet(fileName, s);
		else
			send_end_packet(s);
			
	}
	printf("end the connection\n");
	close(s);
	return 0;
}

unsigned char * encode_int(unsigned char *buffer, int value){

	printf("encode int\n");
	int i = 0;
	for(i = 0;i < 4;i++){
		buffer[i] = value >> (24 - 8 * i);
	}
	return buffer + 4;

}

unsigned char * encode_char(unsigned char *buffer, char *value){

	printf("encode char:\n");
	int i = 0;

	for(i = 0;i < LENGTH; i++){
		buffer[i] = value[i];
	}

	return buffer + LENGTH;

}

/*void send_packet(int s, int a, char b){

	unsigned char buf[32];
	unsigned char *ptr;
	char ack[4];
	ack[3] = '\0';

	ptr = encode_strcut(buf, a, b);
	
	if(sendto(s, buf, ptr - buf, 0, &si_other, slen) == -1)
		diep("sendto()");
	if(recvfrom(s, ack, 3, 0, &si_other, &slen) == -1)
		diep("recvfrom");

	printf("ack: %s\n", ack);

}*/

unsigned char * encode_struct(unsigned char *buffer, int a, char *b){

	buffer = encode_int(buffer, a);
	buffer = encode_char(buffer, b);

	
	return buffer;

}

unsigned char * encode_data_struct(unsigned char *buffer, int length, int sequence, char *data){

	buffer = encode_int(buffer, sequence);
	buffer = encode_int(buffer, length);
	buffer = encode_char(buffer, data);

	return buffer;

}

void send_connection_packet(char *fileName, int s){

	// packet style: file name + file size
	// ack style: num
	char readBuffer[1000],ack[2];
	int f, fileLength = 0, i, j;
	unsigned char buf[LENGTH + 8], *ptr;
	ack[1] = '\0';


	if((f = open(fileName, O_RDONLY)) == -1)	
		diep("open file failed");
	while((i = read(f, readBuffer, 1000)) > 0){
		fileLength += i;
	}
	for(j = 0;j < strlen(fileName); j++)
		readBuffer[j] = fileName[j];
	
	ptr = encode_struct(buf, fileLength, readBuffer);

	if(sendto(s, buf, ptr - buf, 0, &si_other, slen) == -1)
		diep("connection packet error\n");
	while(recvfrom(s, ack, 1, 0, &si_other, &slen) < 0){
		sendto(s, buf, ptr - buf, 0, &si_other, slen);
	}
	printf("the recv ack is : %s\n", ack);
	ackType = 1;
	expectNum = 1;
	
}

void send_end_packet(int s){

	printf("send_end_packet\n");
	char end[2], rec[2];
	end[0] = '1';
	end[1] = '\0';
	rec[1] = '\0';
	if(sendto(s, end, 1, 0, &si_other, slen) == -1)
			diep("end packet error\n");
	while(recvfrom(s, rec, 1, 0, &si_other, &slen) < 0){
		if(sendto(s, end, 1, 0, &si_other, slen) == -1)
			diep("end packet error\n");
	}

	if(rec[0] != '1')
		diep("wrong fin\n");
	fin = 1;

}

void send_data_packet(char *fileName, int s){
	//packet style: sequence num + length + data
	printf("send_data_packet\n");
	int f, i, fileLength = 0;
	char readBuffer[LENGTH + 1], ack[5];
	unsigned char buf[LENGTH + 8], *ptr;
	ack[4] = '\0';
	if(expectNum < 1) diep("can not start sending data packet \n");
	if((f = open(fileName, O_RDONLY)) == -1) diep("can not open the file when sending data packets\n");
	while((i = read(f, readBuffer, LENGTH)) > 0){
		readBuffer[i] = '\0';
		printf("reading: %s\n", readBuffer);
		ptr = encode_data_struct(buf, i, expectNum, readBuffer);
		expectNum += i;
		fileLength += i;
		//printf("ready recv\n");
		while(1){ // small loop to do congestion control
			if(sendto(s, buf, ptr - buf, 0, &si_other, slen) < 0)
				diep("send error\n");
			alarm(5);
			//recvfrom(s, ack, sizeof(char) * 4, 0, &si_other, &slen) < 0 || expectNum != decode_ack(ack)
			if(recvfrom(s, ack, sizeof(char) * 4, 0, &si_other, &slen) >= 0){
				if(expectNum == decode_ack(ack)){
					alarm(0);
					break;
				}
			}
			printf("recv error\n");
		}
	}
	if(expectNum - 1 != fileLength) diep("data sending error\n");
	else ackType = 2;

}

int decode_ack(char *ack){
	//printf("decode_ack:%d\n", strlen(ack));

	if(ack != NULL){
		printf("expect sequence: %d\n", decode_int(ack));
		return decode_int(ack);
	}
	return -1;

}

unsigned int decode_int(unsigned char *buffer){

	unsigned int ans = 0;
	int i = 0;
	printf("%s\n", "decode int");

	for(i = 0; i < 4;i++){
		ans += buffer[i];
		if(i != 3) ans = ans << 8;
	}
	return ans;

}