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
	
	// Initialize window
	int threshold = 16, winsize = 1, acked_seq_num = 0, result, end = 0;
	int send_max_seq = 0, last_seq = -1;

	while(1){
		//send packets
		int window_lower_bound = acked_seq_num + 1;
		//acked_seq_num 表示已經得到ack(被acked完畢)的number
		//window lower 要從acked完的下一個(尚未被acked的)開始送
		int window_upper_bound = acked_seq_num + winsize;
		//[acked_seq_num + 1, acked_seq_num + winsize] 總共 winsize 個
		for(int i = window_lower_bound; i <= window_upper_bound && !end; i++){
			//send winsize 個 packet
			//每個packet都從file裏面讀出 1KB 的內容
			fseek(fp, (i-1)*KiloByte, SEEK_SET);
			//必須要有這個seek。因為packet loss時可能會需要回來重新傳這段sequence number
			segment s_tmp;
			if((result = fread(s_tmp.data, 1, KiloByte, fp)) < 0){
				perror("file read error\n");
				exit(1);
			}
			if(result == 0){		// End of reading file
				last_seq = i-1; //最後的sequence number只到第(i-1)格window cell而已 (因為最後一個read沒有內容不會送出)
				
				end = 1; //假設讀完file就end (其實還要 1.再收回最後一批ack 2.送出fin 3.收回finack)
				break;
			}
			s_tmp.head.length = result;
			s_tmp.head.seqNumber = i;
			s_tmp.head.ackNumber = 0;
			s_tmp.head.fin = 0;
			s_tmp.head.syn = 0;
			s_tmp.head.ack = 0;

			printf("data == %s\n",s_tmp.data);

			if(send_packet(socket_fd, &s_tmp, &agent)){
				//如果 i 被迫向回跳(小於曾經到達過的最大處)，就表示正在進行retransmission
				if(i > send_max_seq)
					printf("send\tdata\t#%d,\twinSize = %d\n", s_tmp.head.seqNumber, winsize);
				else
					printf("resnd\tdata\t#%d,\twinSize = %d\n", s_tmp.head.seqNumber, winsize);
			}
		}
		while(acked_seq_num != window_upper_bound){
			//假設全部都會收到並回覆ack
			acked_seq_num++;
		}

		if(end){
			break;
		}
	}
	
	//each time read (window size) from file and send
	//but the last time only last_seq and send
	//also only receive last_seq ack from receiver
	//and then sender send a fin to receiver
	//receive finack from receiver
	//window size means how many packet one time
}