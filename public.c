#include "public.h"

int pksize = 0;
double thrput = 0;
int count_thrput = 0;
/*calculate latency*/
double cal_latency(long int sec_t, long int usec_t, long int sec_r, long int usec_r)
{
	double time_inval;
	time_inval = ( (double) (sec_r - sec_t) * 1000 + (double) (usec_r - usec_t) / 1000) / 2;
	printf("transmition latency is %.3f \n",time_inval);
	return time_inval;
}

/*calculate loss rate*/
double cal_lossrate(int *head_num_p, int *good_num, int total_num)
{
	double lossrate = -1;
	if (*head_num_p == -1)
		*head_num_p = total_num - 1;
	total_num = total_num - *head_num_p;

	if (total_num < *good_num){
		*head_num_p = -1;
		*good_num = 0;
	}
	lossrate = (double) (total_num - *good_num) / (double) total_num;
	printf("total_num = %d , good_num = %d , lossrate = %.2f\n", total_num, *good_num, lossrate);
	return lossrate;
}

/*calculate throughput*/
void cal_throughput(int sig)
{
	thrput = (double)count_thrput * (double)pksize * 8 / (1000000 * THR_INTERVAL);
	count_thrput = 0;
	alarm(THR_INTERVAL);
	return;
}
double radian(double d)
    {
       return d * M_PI / 180.0;
    }

double cal_distan(double lat, double lon, double lat_r, double lon_r)
{
    double dst;
    double radLat1 = radian(lat);
    double radLat2 = radian(lat_r);
    double a = radLat1 - radLat2;
    double b = radian(lon) - radian(lon_r);
    dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2) )));
    dst = dst * EARTH_RADIUS;
    dst= round(dst * 10000) / 10000;
    printf("the distance is %.0f\n",dst*1000);
    return dst*1000;
}
