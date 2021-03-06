#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

//#include "ipv6_cast.h"
#include "gps.h"
const char *gps_freq_cmd[3] = { "$PMTK000*32\r\n",
				"$PMTK251,115200*1F\r\n",
				"$PMTK300,100,0,0,0,0*2C\r\n" };
char parsed_gps_info[BUFLEN];
char time_v[TIMELEN], lat_v[LATITUDELEN], lon_v[LONGITUDELEN];
char direction_v[8];
float speed_v;

void warning(const char *msg)
{
	printf("%s",msg);
}
void error(const char *msg)
{
	printf("%s",msg);
}

int set_port_speed(const int fd, const int speed)
{
	struct termios newtio,oldtio;

	if (tcgetattr(fd,&oldtio) != 0) { //tcgetattr get current termal in oldtio
		warning("Setup serial");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL|CREAD;
	newtio.c_cflag &=~CSIZE;
	newtio.c_cflag |= CS8;
	newtio.c_cflag &= ~PARENB;

	switch(speed) {
	case 9600:
		cfsetispeed(&newtio,B9600);
		cfsetospeed(&newtio,B9600);
		break;
	case 38400:
		cfsetispeed(&newtio,B38400);
		cfsetospeed(&newtio,B38400);
		break;
	case 115200:
		cfsetispeed(&newtio,B115200);
		cfsetospeed(&newtio,B115200);
		break;
	}
	newtio.c_cflag &= ~CSTOPB;
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 6;
	tcflush(fd, TCIFLUSH);
	if((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
		warning("Com set");
		return -1;
	}
	return 0;
}

int gps_open_port(const char *dev)
{
	int fd;

	fd = open(dev, O_RDWR);
	if(-1 == fd)
		warning("Can not open serial");

	return fd;
}

int gps_write_port(const int fd, const char *cmd, const int len)
{
	int nwrite;

	nwrite= write(fd, cmd, len);
	if (nwrite < 0)
		warning("Write gps port failed");

	return nwrite;
}

int gps_read_port(const int fd, char *buf, const int len)
{
	int nread;

	memset(buf, 0, len);
	nread = read(fd, buf, len);
	if (nread < 0)
		warning("Read gps port failed");

	return nread;
}

int gps_set_freq(const int fd)
{
	int i;
	int key = 0;
	char buf[BUFLEN_SHORT];

	if (set_port_speed(fd, 115200) < 0) {
		printf("set_port_speed error\n");
		return -1;
	}
	for (i=0; i<3; i++) {
		if (key == 0) {
			gps_write_port(fd, gps_freq_cmd[0], strlen(gps_freq_cmd[0]));
			gps_read_port(fd, buf, sizeof(buf));
			gps_read_port(fd, buf, sizeof(buf));
			key++;
		}
		if (key == 1) {
			gps_write_port(fd, gps_freq_cmd[1], strlen(gps_freq_cmd[1]));
			gps_read_port(fd, buf, sizeof(buf));
			gps_read_port(fd, buf, sizeof(buf));
			gps_read_port(fd, buf, sizeof(buf));
			if (set_port_speed(fd, 115200) < 0) {
				printf("set_port_spedd error\n");
				return -1;
			}
			gps_read_port(fd, buf, sizeof(buf));
			gps_read_port(fd, buf, sizeof(buf));
			key++;
		}
		if (key == 2) {
			gps_write_port(fd, gps_freq_cmd[2], strlen(gps_freq_cmd[2]));
			usleep(50);
			gps_read_port(fd, buf, sizeof(buf));
			gps_read_port(fd, buf, sizeof(buf));
		}
	}

	return 0;
}

void *gpsinfo(void *m)
{
	char gpsinfo[BUFLEN], time[TIMELEN], lat[LATITUDELEN], lon[LONGITUDELEN];
	char ss[12], direction[8];
	float speed;

	double tmpf;
	int tmpi;
	int n, index, i, valid;
	int mark, pos;
	int gps_fd;
	FILE *fp;

	/* GPS initial stuff */
	if ((gps_fd = gps_open_port(GPSDEVICE)) < 0)
		error("Open GPS device path failed");
	if (set_port_speed(gps_fd, 115200) < 0) { // GPS initial
		fprintf(stderr, "GPS device initial failed\n");
		return ((void *)0);
	}
	close(gps_fd); // temperarily close GPS device
	/**
	 * Change GPS's default update frequency 1Hz to 5Hz.
	 */
//	if (gps_set_freq(gps_fd) != 0)
//		error("Set GPS update frequency failed");

	fp = fopen(GPSDEVICE, "r");
	if (fp == NULL)
		error("Open GPS device as file failed");

	while(1) {
		bzero(gpsinfo, sizeof(gpsinfo));
		if (fgets(gpsinfo, sizeof(gpsinfo), fp) != NULL) {
			mark = 0;
			pos = 0;
			for (i=0; gpsinfo[i] != '\0'; i++) {
				if (gpsinfo[i] == '$') {
					if (memcmp(gpsinfo+i, "$GPRMC", 6) == 0) {
						mark = 1;
						pos = i;
						break;
					} else {
						i += 6;
						continue;
					}
				} else {
					continue;
				}
			}

			if (mark == 0)
				continue;

			for (n=pos, index=0; gpsinfo[n]!='\n'; n++) {
				if (gpsinfo[n] == ',') {
					index++;
					switch(index) {
					case 1:
						memcpy(time, gpsinfo+n+1, TIMELEN-1);
						time[TIMELEN-1] = '\0';
						break;
					case 2:
						if (gpsinfo[n+1] == 'A') valid=1;
						else valid=0;
						break;
					case 3:
						memcpy(lat, gpsinfo+n+1, LATITUDELEN-3);
						lat[LATITUDELEN-3] = '\0';
						tmpf = strtod(lat, (char **) NULL);
						tmpi = ((int)tmpf)/100;
						tmpf = tmpf - tmpi*100;
						tmpf = tmpf/60;
						tmpf += tmpi;
						break;
					case 4:
						sprintf(lat, "%9.6f %c", tmpf, gpsinfo[n+1]);
						tmpf = 0;
						tmpi = 0;
						break;
					case 5:
						memcpy(lon, gpsinfo+n+1, LONGITUDELEN-3);
						lon[LONGITUDELEN-3] = '\0';
						tmpf = strtod(lon, (char **) NULL);
						tmpi = ((int)tmpf)/100;
						tmpf = tmpf - tmpi*100;
						tmpf = tmpf/60;
						tmpf += tmpi;
						break;
					case 6:
						sprintf(lon, "%10.6f %c", tmpf, gpsinfo[n+1]);
						break;
					case 7:
						for (i=n+1; gpsinfo[i]!=','; i++)
							;
						memcpy(ss, gpsinfo+n+1, i-n-1);
						ss[i-n-1] = '\0';
						#define KNTOKM 1.852
						speed = KNTOKM * strtof(ss, (char **) NULL);
						break;
					case 8:
						for (i=n+1; gpsinfo[i]!=','; i++)
							;
						memcpy(direction, gpsinfo+n+1, i-n-1);
						direction[i-n-n] = '\0';
					default:
						break;
					}
				}
			}
			
			snprintf(parsed_gps_info,BUFLEN, "V %s %s %s %.2f %s\n", time, lat, lon, speed, direction);
			memcpy(time_v,time,sizeof(time));
			memcpy(lat_v,lat,sizeof(lat));
			memcpy(lon_v,lon,sizeof(lon));
			speed_v = speed;
			memcpy(direction_v,direction,sizeof(direction));
			
			//printf("GPS: %s\n",parsed_gps_info);
			/*
			sem_wait (&gps_empty_sem);
			pthread_mutex_lock(&parsed_gps_info_lock);

			if (!valid) {
				snprintf(parsed_gps_info,BUFLEN, "N %s\n", time); // have no self-location info.
			} else {
				snprintf(parsed_gps_info,BUFLEN, "V %s %s %s %.2f %s\n", time, lat, lon, speed, direction);
			}
			
			pthread_mutex_unlock(&parsed_gps_info_lock);
			sem_post (&gps_full_sem);
			usleep (1);

			if (valid) {
				sem_wait (&CD_empty_sem);
				pthread_mutex_lock(&CD_sendbuf_lock);

				sprintf(CD_sendbuf, "V %s %s %s %.2f %s\n", time, lat, lon, speed, direction);

				pthread_mutex_unlock(&CD_sendbuf_lock);
				sem_post (&CD_full_sem);
				usleep (1);
			}*/
		} /*else {
			warning("GPS information read error, retrying...\n");
			sem_wait (&gps_empty_sem);
			pthread_mutex_lock(&parsed_gps_info_lock);

			strcpy(parsed_gps_info, VANET_DEFAULT_MSG);

			pthread_mutex_unlock(&parsed_gps_info_lock);
			sem_post (&gps_full_sem);
			//usleep (1);
			sleep(GPS_REOPEN_TIME); //wait for GPS device
		}*/
	}

	fclose(fp);
	return ((void *)0);
}
/*int main(int argc, char *argv[])
{
	return -1;
}*/
