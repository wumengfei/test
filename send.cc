/*
This is a complete sending and recieving program, 
It use PF_PACKET and RAW socket to transmit data.
It can calculate latency, loss rate, and throughput,
and all will be recorded in 'vanet_log' file.

All the variables and Units are listed
flag		% 't' for sending, 'r' for recieving
size		bytes
interval	ms
count_t		% number of packet for sending
count_r		% number of packet for recieving
sec_t		s
usec_t		us
good_num	good number of packet for recieving
total_num	total number of packet for recieving
latency		ms
loss rate	%
throughput	Mb per sec
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include "./GPS/GPS.h"
#include "public.h"

#define IF_NAME "wlan1" // if_name of ath5k network card
#define ETH_P_WSMP 0x88DC // WSMP protocol number
#define MTU 1500 // Maximum Transmission Unit


/*args from user input to pthread*/
struct	argument{
	int size;
	int interval;
};

/*custom packet struct*/
struct test_struct{
	char flag;
	int size;
	int interval;
	int count_t;
	int count_r;
	long int tv_sec;
	long int tv_usec;
	
    struct GPS newgps_info;
    double time;
    double latitude;
    double longitude;
    double speed;
    double direction;
    
//	char time[TIMELEN];
//	char lat[LATITUDELEN];
//	char lon[LONGITUDELEN];
//	float speed;
//	char direction[8];
	char extra[MTU];
};

/*You can deal with other things while communication through IEEE802.11p*/	
void deal_while_communication()
{
	return;
}

/*sending pthread*/
void *send(void *arg)
{
	struct argument *arg1 = (struct argument *)arg;
	/*socket number*/
	int rawfd;
	int count_t = 0;		
	struct timeval tv_send;
	struct ifreq ifstruct;
	/*socket address struct*/
	sockaddr_ll send_sll;
	test_struct data_t;
	char *send_buf = (char *)malloc(arg1->size);
	/*clear*/	
	memset(&ifstruct, 0, sizeof(ifstruct));
	memset(&send_sll, 0, sizeof(send_sll));
	memset(send_buf,0,sizeof(send_buf));

	/*create a socket*/
	if((rawfd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_WSMP)))==-1){
		perror("Create socket");
		free(send_buf);
		exit(1);
	}

	/*fill up sock address stuct*/
	strcpy(ifstruct.ifr_name, IF_NAME);
	ioctl(rawfd, SIOCGIFINDEX, &ifstruct);
	send_sll.sll_ifindex = ifstruct.ifr_ifindex;
	send_sll.sll_family = AF_PACKET;
	send_sll.sll_protocol = htons(ETH_P_WSMP);
	send_sll.sll_pkttype = PACKET_BROADCAST;
	send_sll.sll_halen = ETH_ALEN;
	memset(send_sll.sll_addr,0xFF,ETH_ALEN);	
	data_t.flag = 't';
	data_t.size = arg1->size;
	data_t.interval = arg1->interval;

	printf("Start to send message...\n\n");

	while (1){
		gettimeofday(&tv_send,NULL);
        data_t.newgps_info.time = time_v;
        data_t.newgps_info.latitude = lat_v;
        data_t.newgps_info.longitude = lon_v;
        data_t.newgps_info.speed = speed_v;
        data_t.newgps_info.direction = direction_v;
//		memcpy(data_t.newgps_info.time, time_v, sizeof(time_v));
//		memcpy(data_t.newgps_info.latitude, lat_v, sizeof(lat_v));
//		memcpy(data_t.newgps_info.longitude, lon_v, sizeof(lon_v));
//		data_t.newgps_info.speed = speed_v;
//		memcpy(data_t.direction, direction_v, sizeof(direction_v));	
		data_t.count_t = ++count_t;
		data_t.tv_sec = tv_send.tv_sec;
		data_t.tv_usec = tv_send.tv_usec;
        

		printf("T: flag=%c, size=%d, interval=%d, count_t=%d, sec_t=%ld, usec_t=%ld, time=%f, lat=%f, lon=%f, speed=%.2f, direction=%f\n", 
			data_t.flag,
			data_t.size,
			data_t.interval,
			data_t.count_t,
			data_t.tv_sec,
			data_t.tv_usec,
			data_t.newgps_info.time,
			data_t.newgps_info.latitude,
			data_t.newgps_info.longitude,
			data_t.newgps_info.speed,
			data_t.newgps_info.direction);
   		
		/*// transform
		data_t.size = htons(data_t.size);
		data_t.interval = htons(data_t.interval);
		data_t.count_t = htons(data_t.count_t);
		data_t.count_r = htons(data_t.count_r);
		data_t.tv_sec = htonl(data_t.tv_sec);
		data_t.tv_usec = htonl(data_t.tv_usec);
		*/	
		memcpy(send_buf,&data_t,sizeof(test_struct));
		//debug
		/*for (int i=0; i<50; i++){
			printf("%X-", send_buf[i]);
		}
		printf("\n");*/	
		/*send data to socket*/
		if(sendto(rawfd,send_buf,arg1->size,0,(struct sockaddr *)&send_sll,sizeof(send_sll))<0){
			perror("Send packet");
			close(rawfd);
			free(send_buf);
			exit(1);
		}
		
		fflush(stdout);	    		
		usleep(arg1->interval*1000);

		//debug gps
		//printf("GPS: %s\n",parsed_gps_info);
	}
	close(rawfd);
	free(send_buf);
	exit(1);

}

/*recieving pthread*/
void *rcv(void *arg)
{
	struct argument *arg1 = (struct argument *)arg;
	/*record first r_count*/
        int rx_head_num = -1;
	/*socket number*/
	int rawfd;
	int fromlen;
	/*record good packets recieved*/
	int rx_good_num = 0;
	char *rcv_buf = (char *)malloc(arg1->size);
	FILE *fp;
	struct timeval time_r;
	struct ifreq ifstruct;
	/*socket address struct*/
	sockaddr_ll rcv_sll;
	test_struct data_r;
	
	/*clear*/
	memset(rcv_buf,0, sizeof(rcv_buf));
	memset(&ifstruct, 0, sizeof(ifstruct));
	memset(&rcv_sll, 0, sizeof(rcv_sll));

	/*create a socket*/
	if ((rawfd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_WSMP))) == -1){
		perror("Create socket");
		free(rcv_buf);
		exit(1);
	}	
        /*open a log file with an append mode*/
	if ((fp = fopen("vanet_log","a")) == NULL){
		perror("Create file");
		close(rawfd);
		free(rcv_buf);
		exit (1);
	}

	/*fill up socket address struct*/
	strcpy(ifstruct.ifr_name, IF_NAME);
	ioctl(rawfd, SIOCGIFINDEX, &ifstruct);
	rcv_sll.sll_family = AF_PACKET;
	rcv_sll.sll_protocol = htons(ETH_P_WSMP);	
	rcv_sll.sll_ifindex = ifstruct.ifr_ifindex;
	rcv_sll.sll_pkttype = PACKET_BROADCAST;
	rcv_sll.sll_halen = ETH_ALEN;
	memset(rcv_sll.sll_addr,0xFF,ETH_ALEN);
	fromlen = sizeof(rcv_sll);

	// write a starting in file
	if( fprintf(fp,"Start to write ...\n") <0){
		perror("fprintf");
		close(rawfd);
		fclose(fp);
		free(rcv_buf);
		exit(1);
	}
	
	while(1){
		/*listen and recieve packet*/
		if (recvfrom(rawfd,rcv_buf,arg1->size,0,(struct sockaddr *)&rcv_sll, (socklen_t*)(&fromlen))>0){
			
			memcpy(&data_r,rcv_buf,sizeof(test_struct));
			//network to host byte order
			/*data_r.size = ntohs(data_r.size);
			data_r.interval = ntohs(data_r.interval);
			data_r.count_t = ntohs(data_r.count_t);
			data_r.count_r = ntohs(data_r.count_r);
			data_r.tv_sec = ntohl(data_r.tv_sec);
			data_r.tv_usec = ntohl(data_r.tv_usec);*/
			/*if packet from resender*/
			if(data_r.flag == 'r'){	
				//debug
				/*for (int i=0; i<50; i++){
					printf("%X-", rcv_buf[i]);
				}
				printf("\n");*/	

				gettimeofday(&time_r,NULL);
				//rx_good_num ++;
				/*calculate throughput*/
				count_thrput ++;	
				printf("R: flag=%c, size=%d, interval=%d, count_t=%d, count_r=%d,sec_t=%ld, usec_t=%ld, sec_r=%ld, usec_r=%ld, time=%f, lat=%f, lon=%f, speed=%.2f, direction=%f\n", 
						data_r.flag,
						data_r.size,
						data_r.interval,
						data_r.count_t,
						data_r.count_r,
						data_r.tv_sec,
						data_r.tv_usec,
						time_r.tv_sec,
						time_r.tv_usec,
						data_r.newgps_info.time,
						data_r.newgps_info.latitude,
						data_r.newgps_info.longitude,
						data_r.newgps_info.speed,
						data_r.newgps_info.direction);
				printf("throughtput is %.3f\n",thrput);

				if(fprintf(fp,"size=%d, interval=%d, count_t=%d, count_r=%d, sec_t=%ld, usec_t=%ld, sec_r=%ld, usec_r=%ld, thrput=%.3f, RTT=%.3f, loss rate=%.2f, dis=%f\n",
						data_r.size,
						data_r.interval,
						data_r.count_t,
						data_r.count_r,
						data_r.tv_sec,
						data_r.tv_usec,
						time_r.tv_sec,
						time_r.tv_usec,
						thrput,
						cal_latency(data_r.tv_sec,data_r.tv_usec,time_r.tv_sec,time_r.tv_usec),
						cal_lossrate(&rx_head_num,&data_r.count_r,data_r.count_t),
						cal_distan(data_r.newgps_info.latitude, data_r.newgps_info.longitude, lat_v, lon_v))<0){
							perror("fprintf");
							close(rawfd);
							fclose(fp);
							free(rcv_buf);
							exit(1);
				}
				printf("\n<=============================================================================>\n");
				fflush(fp);				
				fflush(stdout);					
			}
		}	
	}
	close(rawfd);
	fclose(fp);
	free(rcv_buf);
	exit(1);
}

int main(int argc, char *argv[])
{       
	/* defind the packet size and interval for sending*/
	argument arg;
	/* defind the threads for send and recieve*/
	pthread_t rcv_thread; 	
	pthread_t send_thread;
	pthread_t gps_thread;
	/* check if it's legal for the input*/ 	
	if (argc != 3){
		printf("error: this is an error format input!\
			\nsend [size (100-1500 bytes)] [interval (1-1000 ms)] ,\
			\nplease input a right format!\n");
		return -1;
	}
	if (((arg.size = atoi(argv[1])) < 100)
		|| (arg.size > 1500)
		|| ((arg.interval = atoi(argv[2])) < 1)
		|| (arg.interval > 1000))
	{
		printf("error: this is an error format input!\
			\nsend [size (100-1500 bytes)] [interval (1-1000 ms)] ,\
			\nplease input a right format!\n");
		return -1;	
	}
	//create pthread for recieving
	if(pthread_create(&rcv_thread,NULL,rcv,(void *)&arg)==-1){
		perror("Create rcv thread");
		return -1;
	}
	//create pthread for sending
	if(pthread_create(&send_thread,NULL,send,(void *)&arg)==-1){
		perror("Create send thread");
		return -1;
	}
	//create signal for calculating throughput
	signal(SIGALRM, cal_throughput);
	pksize = arg.size;	
	//timer for 3s
	alarm(THR_INTERVAL);
	//create pthread for gps
        set_para();//开启gps线程前有一步骤设置参数
	if(pthread_create(&gps_thread,NULL,(void *)reading_gps,NULL) == -1){
		perror("Create gps thread");
		return -1;
	}
	while(1){
		deal_while_communication();
	}
	return 0;
}

