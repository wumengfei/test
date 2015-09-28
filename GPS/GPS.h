#ifndef GPS_H
#define GPS_H

#include <pthread.h>

#define BUF_SIZE 1024
#define GPS_INTERVAL 10//Interval of reading(ms)

struct GPS
{
    int					time_hh;
    int					time_min;
    int					time_sec;
    double				time;
    double  			latitude;
    double				longitude;
    double				speed;
    double  			direction;
    unsigned int		date;
    char				lat_N_S;
    char				lng_E_W;
    char				status;

};

/*
 * "-------------------GPS-------------------------\n   \
     This is current GPS information:\n                 \
     Time     : %f\n                                    \
     Status   : %c (A stands for Available)\n           \
     Latitude : %f\n                                    \
     N or S   : %c\n                                    \
     Longitude: %f\n                                    \
     E or W   : %c\n                                    \
     Speed    : %f\n                                    \
     Direction: %f (degree, compared with North)\n      \
     Date     : %u\n                                    \
    -----------------------------------------------\n"
*Paras:
* Cgps.time, Cgps.status, Cgps.latitude, Cgps.lat_N_S, Cgps.longitude, Cgps.lng_E_W, Cgps.speed, Cgps.direction, Cgps.date
*/
extern pthread_t pgps_t;
extern int fd_gps;//File descriptor
extern struct GPS gps_info;//Global GPS information
extern const char *dev_path;//GPS device file path

void cthread_gps();//Create thread to read GPS continously
void display_gps();
void test();//A set of test GPS data(The same format as reading from device)
void test2();
void modify();//Modify the GPS data in debug stage
struct GPS reading_gps();//Reading GPS from GPS device
int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop);
void *thread_reading(void *arg);


int set_para();//Set GPS device parameters


#endif // GPS_H
