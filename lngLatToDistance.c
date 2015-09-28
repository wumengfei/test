//平面坐标系
typedef struct tagCRDCARTESIAN{
	double x;
	double y;
	//double z;
}CRDCARTESIAN,*PCRDCARTESIAN;


//大地坐标系
typedef struct tagCRDGEODETIC{
	double longitude; //经度
	double latitude; //纬度 
	//double height;    //大地高,可设为0
}CRDGEODETIC;
typedef CRDGEODETIC *PCRDGEODETIC;

//将地理坐标转化为平面坐标
void CGPS::GeodeticToCartesian (PCRDCARTESIAN pcc, PCRDGEODETIC pcg)
{
	double L,B;//大地经度，纬度
	double l,L0;//投影带中央线起算的经差，投影带中央子午线
	double X,Y;//直角坐标
	double f,e2,a,b,e12;////参考椭球体扁率,第一偏心率，长半轴,短半轴，第二偏心率
	double t;//保存tan B
	double m;//保存lcosB
	double q2;//卯酉圈的分量 的平方
	double M,N;//子午，卯酉圈曲半径
	double S;//子午线弧长
	double A0,B0,C0,D0,E0;
	double n;//投影带号
	double a_1_e2;//a*(1-e*e)的值
	int zonewide=3;//3度带比较准，可以用6度带
#define BJ54 54//北京54基准面为
#define WGS84 84//WGS84基准面为
#define XIAN80 80//西安80基准面为
	int base_type=WGS84;    //投影基准面类型：北京54基准面为54，西安80基准面为80，WGS84基准面
	const double PI = 3.1415926535897932;
	//----------初始化已知参数-----------
	L = pcg->longitude;//经度
	B = pcg->latitude;//纬度
	if(WGS84 == base_type)
	{
		a = 6378137;//单位是“米”
		b = 6356752.3142;
		f = 1/298.257223563;
		e2 = 0.0066943799013;//--第一偏心率
		e12 = 0.00673949674227;//--第二偏心率
	}
	else if(BJ54 == base_type)
	{
		a = 6378245;
		b = 6356863.0187730473;
		f = 1/298.3;
		e2 = 0.006693421622966;//--第一偏心率
		e12 = 0.006738525414683;//--第二偏心率
	}
	else if(XIAN80 == base_type)
	{
		a = 6378140;
		b = 6356755.2881575287;
		f = 1/298.257;
		e2 = 0.006694384999588;//--第一偏心率
		e12 = 0.006739501819473;//--第二偏心率
	}
	//---基本计算公式 a 长半轴，b 短半轴--------
	//f = (a-b)/a;//--椭球体扁率
	//e2 = (a*a-b*b)/(a*a);//--第一偏心率
	//e12 = (a*a-b*b)/(b*b);//--第二偏心率
	a_1_e2 = a*(1-e2);
	//-----简化计算参数
	if (zonewide==6) 
	{
		n=(int)(L/6)+1;
		L0=n*zonewide-3;
	}
	else
	{
		n=(int)((L-1.5)/3)+1;
		L0=n*3;
	}

	//---转化为弧度,sin,cos 等运算都是以弧度为基础的
	L0=L0*PI/180;
	L=L*PI/180;
	B=B*PI/180;
	l = L - L0;
	t = tan(B);
	q2 = e12*(cos(B)*cos(B));
	N = a/sqrt(1-e2*sin(B)*sin(B));////卯酉圈的曲率半径
	m = l*cos(B);
	//---子午线弧长
	//--以下算法是对原始公式的多项式进行了合并处理，保存计算更准
	double m0 = a_1_e2;
	double m2 = 3.0 * e2 * m0 / 2.0;
	double m4 = 5.0 * e2 * m2 / 4.0;
	double m6 = 7.0 * e2 * m4 / 6.0;
	double m8 = 9.0 * e2 * m6 / 8.0;
	double a0 = m0 + m2 / 2.0 + 3.0 * m4 / 8.0 + 5.0 * m6 / 16.0 + 35.0 * m8 / 128;
	double a2 = m2 / 2.0 + m4 / 2.0 + 15.0 * m6 / 32.0 + 7.0 * m8 / 16.0;
	double a4 = m4 / 8.0 + 3.0 * m6 / 16.0 + 7.0 * m8 / 32.0;
	double a6 = m6 / 32.0 + m8 / 16.0;
	double a8 = m8 / 128.0;
	A0 = a0;
	B0 = a2;
	C0 = a4;
	D0 = a6;
	E0 = a8;
	S = (A0 * B - B0* sin(2.0*B)/2.0 + C0 * sin(4.0*B)/4.0 - D0 *sin(6.0*B)/6.0 + E0*sin(8.0*B)/8.0);

	//----高斯投影公式-------
	X = S + N * t *((0.5 + ((5.0 - t * t + 9 * q2 +4 * q2 * q2)/24 + (61.0 - 58.0 * t * t + pow(t,4)) * m * m /720.0) * m * m) * m * m);//pow(t,4)为t的4次方
	Y = N * ((1 + ((1 - t * t +q2)/6.0 +(5.0 - 18.0 * t * t + pow(t,4) + 14 * q2 - 58 * q2 * t * t ) * m * m / 120.0) * m * m ) * m);
	//----根据中国国情坐标纵轴西移500公里当作起始轴
	Y = 1000000 * n + 500000 + Y;
	pcc->x = Y;//因为是以赤道为Y轴的，与我们南北为Y轴是相反的，所以Y赋给x;
	pcc->y = X;/*未经过测试*/
	//TRACE("x=%d , y=%d",pcc->x,pcc->y);
}

