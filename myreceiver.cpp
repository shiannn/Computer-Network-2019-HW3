#include "Myudp_socket.h"
#include <unistd.h>

//#define imgSize 1920000
//#define height 800
//#define width 800

#define imgSize 1555200
#define height 540
#define width 960

#define RECEIVER_PORT 5002
#define BUFSIZE 32

using namespace std;
using namespace cv;

int main(int argc, char *argv[]){ // agent_ip, agent_port, file_path
    // Initialize
	if(argc != 4){
		fprintf(stderr,"wrong arguments\n");
		exit(0);
	}
    char agent_ip[maxIpLength], file_path[maxIpLength];
    //strcpy(agent_ip, argv[1]);
    setIP(agent_ip, argv[1]);
	int agent_port = atoi(argv[2]);
	strcpy(file_path, argv[3]);

    // Open file
	/*
	FILE *fp = fopen(file_path, "w");
	if(fp == NULL){
		fprintf(stderr,"file open failed\n");
		exit(0);
	}
	*/

    // Create UDP socket
	int socket_fd = create_UDP_socket(RECEIVER_PORT);
	struct sockaddr_in agent, client;
	set_addr(&agent, agent_ip, agent_port);

    // Initialization
	segment buf[BUFSIZE];
	int num = 0, acked_seq_num = 0;
	segment s_tmp;

    printf("starting receiving\n");

	Mat imgReceiver = Mat::zeros(height,width, CV_8UC3);
	uchar VideoImagebuffer[imgSize];
	int offset = 0;
    while(1){
        if(recv_packet(socket_fd, &s_tmp, &agent)){
			printf("num==%d\n",num);
            if(num == BUFSIZE){
                //buffer滿了，drop掉 (buffer handling)
                for(int i = 0; i < BUFSIZE; i++){
					//fwrite(buf[i].data, 1, buf[i].head.length, fp);
					memcpy(VideoImagebuffer+offset, buf[i].data, buf[i].head.length);
					offset += buf[i].head.length;
					printf("offset == %d\n",offset);
					if(offset == imgSize){
						memcpy(imgReceiver.data,VideoImagebuffer,imgSize);
						imshow("Video", imgReceiver);
						waitKey(3.333);
						offset = 0;
					}
				}
				if(s_tmp.head.fin == 1){
					//don't drop fin
					//buffer滿了 且 是final
					s_tmp.head.ack = 1;
					s_tmp.head.fin = 1;
					s_tmp.head.syn = 0;
					if(send_packet(socket_fd, &s_tmp, &agent)){
						printf("recv\tfin\n");
						printf("send\tfinack\n");
						break;
					}
				}	
				else{
					printf("drop\tdata\t#%d\n", s_tmp.head.seqNumber);
					//p.type = ACK;
					s_tmp.head.ack = 1;
					//p.seq_num = acked_seq_num;
					s_tmp.head.ackNumber = acked_seq_num;//drop 掉之後維持acked的number 傳回去

					if(send_packet(socket_fd, &s_tmp, &agent))
						printf("send\tack\t#%d\n", s_tmp.head.ackNumber);
					printf("flush\n");
					num = 0;
				}
            }
            else if(s_tmp.head.fin == 0){
                //buffer沒滿且 只是data
                //直接回推原本的 seq_Number 表示收到
                s_tmp.head.ack = 1;
				printf("[check] sequence number==%d\n",s_tmp.head.seqNumber);
				printf("[check] acked number==%d\n",acked_seq_num);
				if(s_tmp.head.seqNumber == acked_seq_num + 1){
					acked_seq_num++;
                    s_tmp.head.ackNumber = acked_seq_num;

					printf("recv\tdata\t#%d\n", s_tmp.head.seqNumber);
					for(int i = 0; i < s_tmp.head.length; i++)
						buf[num].data[i] = s_tmp.data[i];
					buf[num].head.length = s_tmp.head.length;
					num++;
				}
				else{
					printf("drop\tdata\t#%d\n", s_tmp.head.seqNumber);
					s_tmp.head.ackNumber = acked_seq_num;
				}
				if(send_packet(socket_fd, &s_tmp, &agent))
					printf("send\tack\t#%d\n", s_tmp.head.ackNumber);
            }
            else if(s_tmp.head.fin == 1){
                //buffer沒滿且 是final
                s_tmp.head.ack = 1;
                s_tmp.head.fin = 1;
                s_tmp.head.syn = 0;
				if(send_packet(socket_fd, &s_tmp, &agent)){
					printf("recv\tfin\n");
					printf("send\tfinack\n");
					break;
				}
            }
        }
    }
	
    //flush into file
	for(int i = 0; i < num; i++){
		memcpy(VideoImagebuffer+offset, buf[i].data, buf[i].head.length);
		offset += buf[i].head.length;
		printf("offset == %d\n",offset);
		if(offset == imgSize){
			memcpy(imgReceiver.data,VideoImagebuffer,imgSize);
			imshow("Video", imgReceiver);
			waitKey(0);
		}
	}
	/*
    printf("flush\n");
	for(int i = 0; i < num; i++)
		fwrite(buf[i].data, 1, buf[i].head.length, fp);

	fclose(fp);
	*/

    return 0;
}