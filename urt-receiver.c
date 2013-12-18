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

void diep(char *a){
	perror(a);
	//exit(1);
	return ;
}

void decode_buffer(unsigned char *buffer);
unsigned int decode_int(unsigned char *buffer);
unsigned char decode_char(unsigned char *buffer);

int main(int argc, char** argv){

	struct sockaddr_in si_me, si_other;
	int s, i, slen = sizeof(si_other);
	unsigned char buf[BUFLEN];

	if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(s, &si_me, sizeof(si_me)) == -1)
		diep("bind");

	for(i = 0; i < NPACK; i++){

		if(recvfrom(s, buf, BUFLEN, 0, &si_other, &slen) == -1)
			diep("recvfrom");
		else
			decode_buffer(buf);


	}

	close(s);
	return 0;
	
}

void decode_buffer(unsigned char *buffer){

	printf("Start decoding the packet!!\n");
	unsigned char str = decode_char(buffer);
	buffer ++;
	unsigned int ans = decode_int(buffer);

	//char decodeString[1000];
	printf("decoded buffer is: %c, %d\n",  str, ans);
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

unsigned char decode_char(unsigned char *buffer){

	unsigned char c;
	c = buffer[0];
	return c;

}
