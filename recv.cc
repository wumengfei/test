#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <sys/time.h>
#include "./GPS/GPS.h"
#include "public.h"

#define IF_NAME "wlan1" // if_name of ath5k network card
#define ETH_P_WSMP 0x88DC // WSMP protocol number
#define MTU 1500 // Maximum Transmission Unit

int main(int argc, char * argv[])
{
	/*check if it's legal for the input*/
	if (argc > 1){
		printf("error: this is an error format input!\
			\nrecv , please input a right format!\n");
		return -1;
	}
	pthread_t gps_thread;
	//create pthread for gps
    //这里用到了gps.c文件中的*gpsinfo(*m)，需要替换
	if(pthread_create(&gps_thread,NULL,reading_gps(),NULL) == -1){
		perror("Create gps thread");
		return -1;
	}
	/*custom packet struct*/
	struct test_struct{
		char flag;
		int size;
		int interval;
		int count_t;
		int count_r;
		long int tv_sec;
		long int tv_usec;
	
		char time[TIMELEN];
		char lat[LATITUDELEN];
		char lon[LONGITUDELEN];
		float speed;
		char direction[8];
		char extra[MTU];	
	};
     
	struct ifreq ifstruct;
	/*socket address struct*/	
	sockaddr_ll rcv_sll;
	test_struct data_r;
	test_struct data_t;
	/*socket number*/
	int rawfd;
	FILE *fp;
	int count = 0;
	int fromlen = sizeof(rcv_sll);
	int rx_head_num = -1;
	int rx_good_num = 0;	
	char *rcv_buf = (char *)malloc(MTU);	
	
	/*clear*/
	memset(rcv_buf,0,sizeof(rcv_buf));
	memset(&rcv_sll, 0, sizeof(rcv_sll));
	memset(&ifstruct,0, sizeof(ifstruct));
	memset(&data_r,0, sizeof(test_struct));
	memset(&data_t,0, sizeof(test_struct));

	/*create a socket*/
	if ((rawfd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_WSMP))) == -1){
		perror("Create socket");
		free(rcv_buf);
		return -1;
	}

	/*open a log file with an append mode*/
	if ((fp = fopen("vanet_log","a")) == NULL){
		perror("Create file");
		close(rawfd);
		free(rcv_buf);
		return -1;
	}
	/*fill up socket address struct*/
	strcpy(ifstruct.ifr_name,IF_NAME);//上面有ifreq结构体
	ioctl(rawfd, SIOCGIFINDEX, &ifstruct);//问题？
	rcv_sll.sll_family = AF_PACKET;
	rcv_sll.sll_protocol = htons(ETH_P_WSMP);
	rcv_sll.sll_ifindex = ifstruct.ifr_ifindex;
	rcv_sll.sll_pkttype = PACKET_BROADCAST;
	rcv_sll.sll_halen = ETH_ALEN;
	memset(rcv_sll.sll_addr,0xFF,ETH_ALEN);
	
	// write a starting in file
	if( fprintf(fp,"Start to write ...\n") <0){
		perror("fprintf");
		close(rawfd);
		fclose(fp);
		free(rcv_buf);
		return -1;
	}
	printf("Wait for 11p packet...\n");
	while(1){
		/*listen and recieve packet*/
		if(recvfrom(rawfd,rcv_buf,MTU,0,(struct sockaddr *)&rcv_sll, (socklen_t*)(&fromlen))>0){
			memcpy(&data_r,rcv_buf,sizeof(test_struct));
			memcpy(&data_t,rcv_buf,sizeof(test_struct));
			//debug
			/*for (int i=0; i<50; i++){
				printf("%X-", rcv_buf[i]);
			}
			printf("\n");*/		
			//transform
			/*data_r.size = ntohs(data_r.size);
			data_r.interval = ntohs(data_r.interval);
			data_r.count_t = ntohs(data_r.count_t);
			data_r.count_r = ntohs(data_r.count_r);
			data_r.tv_sec = ntohl(data_r.tv_sec);
			data_r.tv_usec = ntohl(data_r.tv_usec);*/
			//
			printf("recv!\n");//debug
			/*if packet from sender*/
			if (data_r.flag == 't'){	
				printf("R: flag=%c, size=%d, interval=%d, count_t=%d, count_r=%d, sec_t=%ld, usec_t=%ld, time=%s, lat=%s, lon=%s, speed=%.2f, direction=%s\n", 
					data_r.flag,
					data_r.size,
					data_r.interval,
					data_r.count_t,
					data_r.count_r,
					data_r.tv_sec,
					data_r.tv_usec,
					data_r.time,
					data_r.lat,
					data_r.lon,
					data_r.speed,
					data_r.direction);
				
				data_t.flag = 'r';
				data_t.count_r = ++count;
				if(data_t.count_t < data_t.count_r){
					count = data_t.count_t;
					data_t.count_r = data_t.count_t;
					rx_good_num = data_t.count_t;
					rx_head_num = -1;
				}
				rx_good_num++;
				memcpy(data_t.time, time_v, sizeof(time_v));
				memcpy(data_t.lat, lat_v, sizeof(lat_v));
				memcpy(data_t.lon, lon_v, sizeof(lon_v));
				data_t.speed = speed_v;
				memcpy(data_t.direction, direction_v, sizeof(direction_v));
				// transform
				/*data_r.size = htons(data_r.size);
				data_r.interval = htons(data_r.interval);
				data_r.count_t = htons(data_r.count_t);
				data_r.count_r = htons(data_r.count_r);
				data_r.tv_sec = htonl(data_r.tv_sec);
				data_r.tv_usec = htonl(data_r.tv_usec);*/
				//
				memcpy(rcv_buf,&data_t,sizeof(test_struct));
				
				/*resend*/
				if(sendto(rawfd,rcv_buf,data_t.size,0,(struct sockaddr *)&rcv_sll,sizeof(rcv_sll))<0){
					perror("Send packet");
					close(rawfd);
					free(rcv_buf);
					return -1;
				}
				if(fprintf(fp,"size=%d, interval=%d, count_t=%d, count_r=%d, sec=%ld, usec=%ld, thrput=%.3f, loss rate=%.2f, dis=%f\n",
						data_r.size,
						data_r.interval,
						data_r.count_t,
						data_r.count_r,
						data_r.tv_sec,
						data_r.tv_usec,
						thrput,
						cal_lossrate(&rx_head_num,&rx_good_num,data_r.count_t),
						cal_distan(atof(data_r.lat), atof(data_r.lon), atof(lat_v), atof(lon_v)))<0){
							perror("fprintf");
							close(rawfd);
							fclose(fp);
							free(rcv_buf);
							return -1;
				}
				printf("Resend the packet...\n");	
				fflush(stdout);
			}
		}				
	}
	close(rawfd);
	free(rcv_buf);
	return 0;
}
