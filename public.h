#ifndef PUBLIC_H
#define PUBLIC_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <signal.h>

#define EARTH_RADIUS 6378.137
#define THR_INTERVAL 3 //interval for calculating interval

extern int pksize;
extern double thrput;
extern int count_thrput;

double cal_latency(long int sec_t, long int usec_t, long int sec_r, long int usec_r);

double cal_lossrate(int *head_num_p, int *good_num, int total_num);

void cal_throughput(int sig);

double radian(double d);

double cal_distan(double lat, double lon, double lat_r, double lon_r);
#endif

