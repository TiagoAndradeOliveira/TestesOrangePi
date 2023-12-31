/*
	THIS CODE WAS TAKEN FROM:
	MPU6050 Interfacing with Raspberry Pi
	http://www.electronicwings.com
	AND ADAPTED TO WORK WITH AN ORANGE PI, IN THE i2c bus 5. (pins 3 and 5)
*/

/*
	In this KalmanFilter Test, it is needed to:
	1 - Plot in real-time the data got from the MPU6050
	2 - Them compare the old data with an Kalman Filter to see how the graph changes
	Note: if its not possible/too hard to implement real-time graphing in C, try to do 30s tests or something like it

	UNFORTUNELY, the pb plots library only suports graphs that have a bigger difference between values. I tried to shake 
	the IMU during the tests and it generated a graph every time. I will check if the sensitivity of the graph
	can be changed
*/

#include "pbPlots.h"
#include "supportLib.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>

#define Device_Address 0x68	//Device Address/Identifier for MPU6050, when AD0 is connected to ground

#define PWR_MGMT_1   0x6B
#define SMPLRT_DIV   0x19
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define INT_ENABLE   0x38
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H  0x43
#define GYRO_YOUT_H  0x45
#define GYRO_ZOUT_H  0x47

int fd;
int cont; 	  //Variable that counts how many cycles happened
#define N 1500 //Number of cycles
#define DelayMiliSeconds 10 //Delay between samples

_Bool PrintGraphic(double xaxis[N], double yaxis[N]){

	_Bool success;

	StartArenaAllocator();

	RGBABitmapImageReference *canvasReference = CreateRGBABitmapImageReference();
	StringReference *errorMessage = CreateStringReference(L"", 0);
	success = DrawScatterPlot(canvasReference, 1920, 1080, xaxis, N, yaxis, N, errorMessage);

	if(success){
		size_t length;
		ByteArray *pngdata = ConvertToPNG(canvasReference->image);
		WriteToFile(pngdata, "example1.png");
		DeleteImage(canvasReference->image);
	}else{
		fprintf(stderr, "Error: ");
		for(int i = 0; i < errorMessage->stringLength; i++){
			fprintf(stderr, "%c", errorMessage->string[i]);
		}
		fprintf(stderr, "\n");
	}

	FreeAllocations();

	return success ? 0 : 1;
}

void PrintString3Acelerations(double Ax[N], double Ay[N], double Az[N]){
	int i;
	for(i=0;i<N;i++){
		printf("| %.5f | %.5f | %.5f |\n",Ax[i],Ay[i],Az[i]);};
}
void Print1String(double L[N]){
	int i;
	for(i=0;i<N;i++){
		printf("| %.5f |\n",L[i]);
	};
}

void MPU6050_Init(){
	//To better understand this registers, see the MPU6050 registers sheet
	wiringPiI2CWriteReg8 (fd, SMPLRT_DIV, 0x07);	// Write to sample rate register 
	wiringPiI2CWriteReg8 (fd, PWR_MGMT_1, 0x01);	// Write to power management register 
	wiringPiI2CWriteReg8 (fd, CONFIG, 0);		    // Write to Configuration register 
	wiringPiI2CWriteReg8 (fd, GYRO_CONFIG, 24);	    // Write to Gyro Configuration register 
	wiringPiI2CWriteReg8 (fd, INT_ENABLE, 0x01);	// Write to interrupt enable register 
} 

short read_raw_data(int addr){
	short high_byte,low_byte,value;
	high_byte = wiringPiI2CReadReg8(fd, addr);
	low_byte = wiringPiI2CReadReg8(fd, addr+1);
	value = (high_byte << 8) | low_byte;
	return value;
}

void ms_delay(int val){
	int i,j;
	for(i=0;i<=val;i++)
		for(j=0;j<1200;j++);
}

int main(void){

	wiringPiSetup();

	//Me trying to make an list of raw AccelValues over time.
	//float RawAccelString[N] = {};
	//float RawAccelValue;

	double AxAccelString[N];
	double AyAccelString[N];
	double AzAccelString[N];
	double TimeLineInMiliseconds[N];
	
	float Acc_x,Acc_y,Acc_z;
	float Gyro_x,Gyro_y,Gyro_z;
	float Ax=0, Ay=0, Az=0;
	float Gx=0, Gy=0, Gz=0;
	// fd = wiringPiI2CSetup(Device_Address);   /*Initializes I2C with device Address*/
	fd = wiringPiI2CSetupInterface("/dev/i2c-5", Device_Address); 
	/*
		IMPORTANTÍSSIMO, É NECESSÁRIO ESPECIFICAR QUE INTERFACE I2-C VAI SER UTILIZAR NA ORANGE, NESSE
		CASO UTILIZAMOS A INTERFACE 5, PINOS 3,5.
	*/

	MPU6050_Init();		                 // Initializes MPU6050 
	
	for(cont=0; cont<N; cont++) //It's not a while for the purpose of implementing KalmanFilter
	{
		// Read raw value of Accelerometer and gyroscope from MPU6050
		Acc_x = read_raw_data(ACCEL_XOUT_H);
		Acc_y = read_raw_data(ACCEL_YOUT_H);
		Acc_z = read_raw_data(ACCEL_ZOUT_H);
		
		Gyro_x = read_raw_data(GYRO_XOUT_H);
		Gyro_y = read_raw_data(GYRO_YOUT_H);
		Gyro_z = read_raw_data(GYRO_ZOUT_H);
		
		// Divide raw value by sensitivity scale factor 
		Ax = Acc_x/16384.0;
		Ay = Acc_y/16384.0;
		Az = Acc_z/16384.0;

		//RawAccelValue = (((Ax*Ax)+(Ay*Ay)+(Az*Az)));
		
		Gx = Gyro_x/131;
		Gy = Gyro_y/131;
		Gz = Gyro_z/131;
		
		//printf("\n Gx=%.3f °/s\tGy=%.3f °/s\tGz=%.3f °/s\tAx=%.3f g\tAy=%.3f g\tAz=%.3f g\n",Gx,Gy,Gz,Ax,Ay,Az);
		//printf("RawAccelValue = %.5f\n",RawAccelValue);
		//RawAccelString[cont] = RawAccelValue;
		AxAccelString[cont] = Ax;
		AyAccelString[cont] = Ay;
		AzAccelString[cont] = Az;
		TimeLineInMiliseconds[cont] = (DelayMiliSeconds*cont);
		//printf("Time of the sample: %f\n", DelayMiliSeconds*cont);
		delay(DelayMiliSeconds);
		
	}

	//printf("\nString of Values Stored\n");

	// PrintString3Acelerations(AxAccelString,AyAccelString,AzAccelString);
	// printf("\nNow printing the TimeLine in ms\n");
	// Print1String(TimeLineInMiliseconds);

	//NOW IT'S THE PART TO TRY TO DRAWN THE GRAPH

	PrintGraphic(TimeLineInMiliseconds,AxAccelString);



}