#include "Myudp_socket.h"
#define SENDER_PORT 5000

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

void setIP(char *dst, char *src) {
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0
            || strcmp(src, "localhost")) {
        sscanf("127.0.0.1", "%s", dst);
    } else {
        sscanf(src, "%s", dst);
    }
}

int main(int argc, char *argv[]){ // agent_ip, agent_port, file_path
    // Initialize
	if(argc != 4){
		fprintf(stderr,"用法: %s <agent IP> <agent port> <file path>\n", argv[0]);
		exit(0);
	}
    char agent_ip[maxIpLength], file_path[maxIpLength];
	//strcpy(agent_ip, argv[1]);
    setIP(agent_ip, argv[1]);
	int agent_port = atoi(argv[2]);
	strcpy(file_path, argv[3]);

    // Open file
	FILE *fp = fopen(file_path, "r");
	if(fp == NULL){
		fprintf(stderr,"file open failed\n");
		exit(0);
	}

    // 建好UDP socket 並且bind 到 INADDR_ANY 上
	int socket_fd = create_UDP_socket(SENDER_PORT);
    // 將 agent 的 ip/port/family 以及 sin_zero 設定好
	struct sockaddr_in agent;
	set_addr(&agent, agent_ip, agent_port);
}