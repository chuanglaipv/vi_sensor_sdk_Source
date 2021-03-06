#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <cv.h>
#include <highgui.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>

#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>
#include <thread>


#include "loitorusbcam.h"
#include "loitorimu.h"

using namespace std;
using namespace cv;

bool close_img_viewer=false;
bool visensor_Close_IMU_viewer=false;

// 当前左右图像的时间戳
timeval left_stamp,right_stamp;

// image viewer
void *opencv_showimg(void*)
{
	IplImage *cv_img1;
	IplImage *cv_img2;
	if(!visensor_resolution_status)
	{
		cv_img1=cvCreateImage(cvSize(IMG_WIDTH_VGA,IMG_HEIGHT_VGA),IPL_DEPTH_8U,1);
		cv_img2=cvCreateImage(cvSize(IMG_WIDTH_VGA,IMG_HEIGHT_VGA),IPL_DEPTH_8U,1);
	}
	else
	{
		cv_img1=cvCreateImage(cvSize(IMG_WIDTH_WVGA,IMG_HEIGHT_WVGA),IPL_DEPTH_8U,1);
		cv_img2=cvCreateImage(cvSize(IMG_WIDTH_WVGA,IMG_HEIGHT_WVGA),IPL_DEPTH_8U,1);
	}
	while(!close_img_viewer)
	{
		usleep(1000);
		//Cam1
		if(visensor_cam_selection==2)
		{
			visensor_imudata paired_imu=visensor_get_leftImg(cv_img1->imageData,left_stamp);

			//cout<<"left_time : "<<left_stamp.tv_usec<<endl;
			
			//cout<<"paired_imu time ===== "<<paired_imu.system_time.tv_usec<<endl<<endl;

			cvShowImage("Left",cv_img1);
			cvWaitKey(1);
		}
		//Cam2
		else if(visensor_cam_selection==1)
		{
			visensor_imudata paired_imu=visensor_get_rightImg(cv_img2->imageData,right_stamp);

			//cout<<"right_time : "<<right_stamp.tv_usec<<endl;
			
			//cout<<"paired_imu time ===== "<<paired_imu.system_time.tv_usec<<endl<<endl;

			cvShowImage("right",cv_img2);
			cvWaitKey(1);
		}
		// Cam1 && Cam2
		else if(visensor_cam_selection==0)
		{
			
			visensor_imudata paired_imu=visensor_get_stereoImg(cv_img1->imageData,cv_img2->imageData,left_stamp,right_stamp);

			//cout<<"left_time : "<<left_stamp.tv_usec<<endl;
			//cout<<"right_time : "<<right_stamp.tv_usec<<endl;
			
			//cout<<"paired_imu time ===== "<<paired_imu.system_time.tv_usec<<endl<<endl;

			cvShowImage("Left",cv_img1);
			cvShowImage("Right",cv_img2);
			cvWaitKey(1);
		}
	}
	pthread_exit(NULL);
}

void* show_imuData(void *)
{
	int counter=0;
	while(!visensor_Close_IMU_viewer)
	{
		if(visensor_imu_have_fresh_data())
           	{
			counter++;
			// 每隔20帧显示一次imu数据
			if(counter>=0)
			{
				float ax=visensor_imudata_pack.ax;
				float ay=visensor_imudata_pack.ay;
				float az=visensor_imudata_pack.az;
				cout<<"visensor_imudata_pack->a : "<<sqrt(ax*ax+ay*ay+az*az)<<endl;
				cout<<"visensor_imudata_pack->r : "<<visensor_imudata_pack.rx<<" , "<<visensor_imudata_pack.ry<<" , "<<visensor_imudata_pack.rz<<endl;
				//cout<<"imu_time ======================= "<<visensor_imudata_pack.system_time.tv_usec<<endl;
				counter=0;
			}
		}
		usleep(50);
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{

	/************************ Start Cameras ************************/
	visensor_load_settings("Loitor_VISensor_Setups.txt");

	// 手动设置相机参数
	//visensor_set_current_mode(5);
	//visensor_set_auto_EG(0);
	//visensor_set_exposure(50);
	//visensor_set_gain(200);
	//visensor_set_cam_selection_mode(2);
	//visensor_set_resolution(false);
	//visensor_set_fps_mode(true);
	// 保存相机参数到原配置文件
	//visensor_save_current_settings();

	int r = visensor_Start_Cameras();
	if(r<0)
	{
		printf("Opening cameras failed...\r\n");
		return r;
	}
	/************************** Start IMU **************************/
	int fd=visensor_Start_IMU();
	if(fd<0)
	{
		printf("visensor_open_port error...\r\n");
		return 0;
	}
	printf("visensor_open_port success...\r\n");
	/************************ ************ ************************/

	usleep(100000);

	//Create img_show thread
	pthread_t showimg_thread;
	int temp;
	if(temp = pthread_create(&showimg_thread, NULL, opencv_showimg, NULL))
	printf("Failed to create thread opencv_showimg\r\n");
	//Create show_imuData thread
	pthread_t showimu_thread;
	if(temp = pthread_create(&showimu_thread, NULL, show_imuData, NULL))
	printf("Failed to create thread show_imuData\r\n");

	
	
/**/
	while(1)
	{
		// Do - Nothing :)
		//cout<<visensor_get_imu_portname()<<endl;
		//cout<<"hardware_fps : "<<visensor_get_hardware_fps()<<endl;
		usleep(10000000);
	}

	/* shut-down viewers */
	close_img_viewer=true;
	visensor_Close_IMU_viewer=true;
	if(showimg_thread !=0)
	{
		pthread_join(showimg_thread,NULL);
	}
	if(showimu_thread !=0)
	{
		pthread_join(showimu_thread,NULL);
	}

	/* close cameras */
	visensor_Close_Cameras();
	/* close IMU */
	visensor_Close_IMU();

	return 0;
}
