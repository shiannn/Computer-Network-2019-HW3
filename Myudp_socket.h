#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include "opencv2/opencv.hpp"

#define maxIpLength 100
#define KiloByte 1000

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
} header;

typedef struct{
	header head;
	char data[1000];
} segment;

int max(int a, int b){
	return a > b? a : b;
}

int create_UDP_socket(int port){
	int socket_fd;
	struct sockaddr_in server_addr;
	if((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) <0) {
		perror("socket failed\n");
		exit(EXIT_FAILURE);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);
	if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <0) {
		perror("bind failed\n");
		exit(1);
	}
	return socket_fd;
}
void set_addr(struct sockaddr_in* addr, char* ip, int port){
	//memset(addr, 0, sizeof(*addr));
    memset(addr->sin_zero, '\0', sizeof(addr->sin_zero));    
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    addr->sin_port = htons(port);
}

int recv_packet(int fd, segment* s_tmp, struct sockaddr_in* addr){
	int nbytes, length = sizeof(*addr);
	if((nbytes = recvfrom(fd, s_tmp, sizeof(*s_tmp), 0, (struct sockaddr*)addr, (socklen_t*)&length)) < 0) {
		perror("packet receiving failed\n");
	}
	return nbytes;
}
int send_packet(int fd, segment* s_tmp, struct sockaddr_in* addr){
	int nbytes, length = sizeof(*addr);
	if((nbytes = sendto(fd, s_tmp, sizeof(*s_tmp), 0, (struct sockaddr*)addr, *(socklen_t*)&length)) < 0){
		perror("packet sending failed\n");
	}
	return nbytes;
}

void setIP(char *dst, char *src) {
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0
            || strcmp(src, "localhost")) {
        sscanf("127.0.0.1", "%s", dst);
    } else {
        sscanf(src, "%s", dst);
    }
}