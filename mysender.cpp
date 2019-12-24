#include "Myudp_socket.h"
#define SENDER_PORT 5000

using namespace std;
using namespace cv;

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


	VideoCapture cap;
	bool VideoOpen = cap.open(file_path);
	if(VideoOpen == false){
		fprintf(stderr,"Video open failed\n");
		exit(0);
	}
	int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	Mat imgSender = Mat::zeros(height,width, CV_8UC3);
	
	//printf("width == %d height == %d\n",width,height);
	//imshow("Test",imgSender);
	//waitKey(0);

    // Open file
	/*
	FILE *fp = fopen(file_path, "r");
	if(fp == NULL){
		fprintf(stderr,"file open failed\n");
		exit(0);
	}
	*/

    // 建好UDP socket 並且bind 到 INADDR_ANY 上
	int socket_fd = create_UDP_socket(SENDER_PORT);
    // 將 agent 的 ip/port/family 以及 sin_zero 設定好
	struct sockaddr_in agent;
	set_addr(&agent, agent_ip, agent_port);
	
	// Initialize window
	int threshold = 16, winsize = 1, acked_seq_num = 0, result, end = 0;
	int send_max_seq = 0, last_seq = -1;

	cap >> imgSender;
	int imgSize = imgSender.total()*imgSender.elemSize();
	printf("width==%d height==%d\n",width,height);
	uchar VideoImagebuffer[imgSize];
	memcpy(VideoImagebuffer,imgSender.data, imgSize);
	int imgPointer = 0;
	while(1){
		//send packets
		int window_lower_bound = acked_seq_num + 1;
		//acked_seq_num 表示已經得到ack(被acked完畢)的number
		//window lower 要從acked完的下一個(尚未被acked的)開始送
		int window_upper_bound = acked_seq_num + winsize;
		//[acked_seq_num + 1, acked_seq_num + winsize] 總共 winsize 個
		segment s_tmp;
		for(int i = window_lower_bound; i <= window_upper_bound && !end; i++){
			//send winsize 個 packet
			//每個packet都從file裏面讀出 1KB 的內容

			/*fseek(fp, (i-1)*KiloByte, SEEK_SET);*/
			imgPointer = (i-1)*KiloByte;
			printf("imgSize == %d\n",imgSize);
			printf("imgPointer == %d\n",imgPointer);
			//必須要有這個seek。因為packet loss時可能會需要回來重新傳這段sequence number

			/*result = fread(s_tmp.data, 1, KiloByte, fp);*/
			int result = imgSize - imgPointer;
			if(result >= KiloByte){
				result = KiloByte;
				memcpy(s_tmp.data,VideoImagebuffer+imgPointer, result);
			}
			else if(result < KiloByte && result > 0){
				//result = result
				memcpy(s_tmp.data,VideoImagebuffer+imgPointer, result);
			}
			else if(result <= 0){
				result = 0;
			}
			printf("result == %d\n",result);
			if(result < 0){
				perror("file read error\n");
				exit(1);
			}
			if(result == 0){		// End of reading file
				last_seq = i-1; //最後的sequence number只到第(i-1)格window cell而已 (因為最後一個read沒有內容不會送出)
				
				//end = 1; 假設讀完file就end (其實還要 1.再收回最後一批ack 2.送出fin 3.收回finack)
				break;
			}
			s_tmp.head.length = result;
			s_tmp.head.seqNumber = i;
			s_tmp.head.ackNumber = 0;
			s_tmp.head.fin = 0;
			s_tmp.head.syn = 0;
			s_tmp.head.ack = 0;

			//printf("data == %s\n",s_tmp.data);

			if(send_packet(socket_fd, &s_tmp, &agent)){
				//如果 i 被迫向回跳(小於曾經到達過的最大處)，就表示正在進行retransmission
				if(i > send_max_seq)
					printf("send\tdata\t#%d,\twinSize = %d\n", s_tmp.head.seqNumber, winsize);
				else
					printf("resnd\tdata\t#%d,\twinSize = %d\n", s_tmp.head.seqNumber, winsize);
			}
		}
		//break; 
		//暫時跳出測試
	
		//更新已經完成送出的sequence number到哪裡了
		send_max_seq = max(window_upper_bound, send_max_seq);
		//更新window size
		int old_winsize = winsize;
		if(winsize < threshold)
			winsize *= 2;
		else
			winsize++;

		//Receive Ack from receiver
		while(acked_seq_num != window_upper_bound){
			//假設全部都會收到並回覆ack
			//acked_seq_num++;
			if(last_seq != -1){//如果last_seq有反應，表示出現了沒有讀到滿的情況
				if(acked_seq_num == last_seq){//如果最末一個seq_number被acked了，就結束
					end = 1;
					s_tmp.head.fin = 1;
					s_tmp.head.ack = 0;
					if(send_packet(socket_fd, &s_tmp, &agent))
						printf("send\tfin\n");
					if(recv_packet(socket_fd, &s_tmp, &agent))
						printf("recv\tfinack\n");
					break;
				}
			}

			fd_set input_set;
			FD_ZERO(&input_set);		  //先清0所有 file descriptor
			FD_SET(socket_fd, &input_set);//把這唯一的一支 socket fd 加入監控

			// 監控在timeout時間內，[0,socket_fd) 可不可讀
			struct timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 100;
			int ready_for_reading = select(socket_fd+1, &input_set, NULL, NULL, &timeout);
			// select回傳-1表示error，回傳0表示timeout，回傳正數表示ready的fd數量
			if(ready_for_reading == -1){
				//error
				fprintf(stderr,"select reading failed\n");
				exit(0);
			}
			else if(ready_for_reading == 0){
				//timeout, 處理window size
				threshold = max(old_winsize/2, 1);
				winsize = 1;
				printf("time\tout\t\tthreshold = %d\n", threshold);
				break;
			}
			else{
				//有東西可以讀
				//只從socket讀走一個 segment 的大小(sizeof(s_tmp))
				if(recv_packet(socket_fd, &s_tmp, &agent)){
					if(s_tmp.head.ack == 1){//if not ack, just ignore it (wrong data)
						printf("recv\tack\t#%d\n", s_tmp.head.ackNumber);
						if(s_tmp.head.ackNumber == acked_seq_num + 1){//what we are waiting for
							acked_seq_num++;//已經被Acked的數量增長
						}
					}
				}
			}
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

	return 0;
}