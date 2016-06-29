#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h> 
#include <string.h>
#include <stdint.h> //int8_t

#include <unistd.h> //sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h> // serial ports

/* baudrate settings are defined in <asm/termbits.h>, which is included by <termios.h> */
#define BAUDRATE 115200         
/* change this definition for the correct port */
#define DEVICEPORT "/dev/ttyACM0"

/******************************************************
  Prepared by Avi Klausner, Naved Ansari, Adarsh Mammen
  
  Usage:
    ./Controller
	 
Run 4 pre-compiled commands for car
******************************************************/


int main(int argc, char **argv) 
{
	char command[128];
	char response[2560];
	char start[3];
	char dataRequest[7];
	int cFlag = 0;
	int r, c;

	int8_t x=0, y=0, z=0;
	int8_t filter_x=0, filter_y=0, filter_z=0;

	start[0] = 0xFF;
	start[1] = 0x07;
	start[2] = 0x03; 

	dataRequest[0] = 0xFF;
	dataRequest[1] = 0x08;
	dataRequest[2] = 0x07; 
	dataRequest[3] = 0x00;
	dataRequest[4] = 0x00; 	
	dataRequest[5] = 0x00;
	dataRequest[6] = 0x00; 
	
	// open port for watch RF Communication
	struct termios  config;
	int wFile;
	wFile = open(DEVICEPORT, O_RDWR | O_NOCTTY | O_NDELAY); //open port (NON BLOCKING)
	if(wFile == -1)
	{
		printf( "failed to open port\n" );
		return -1;
	}

	// open car device driver file
	FILE * cFile;
	cFile = fopen("/dev/mycar", "r+"); 
	if (cFile==NULL) {
		fputs("mycar module isn't loaded\n",stderr);
		return -1;
	}

	bzero(&config, sizeof(config));
	config.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD; //set baud rate, no. of bits = 8
	config.c_oflag = 0;
	tcflush(wFile, TCIFLUSH);
	tcsetattr(wFile,TCSANOW,&config);

	//start sequence: 0xFF 0x07 0x03;
	write(wFile,start,sizeof(start)); //send start sequence to device. 
	printf("press start button on watch now!!!!\n");	
	sleep(1);
	printf("hope you already pressed button!\n");

	int d = 0;
	int zeroCounter = 0;
	while(1)
	{
		fclose(cFile);
		usleep(50000); //sleep between every subsequent requests. 
		strcpy(response,"");

		//data request sequence: 0xFF 0x08 0x07 0x00 0x00 0x00 0x00;
		write(wFile,dataRequest,sizeof(dataRequest));
		//printf("\nRequest sent!\n");
	
		read(wFile,response,255);		
	
		x = response[4];
		y = response[5];
		z = response[6];

		if ((x + y + z) ==0) //ignore zeros
		{
			zeroCounter++;
		}
		else
		{
		
			zeroCounter = 0;
			filter_x = x;
			filter_y = y;
			filter_z = z;
		}
		printf("x = %hhd, y = %hhd, z = %hhd\n",x,y,z);
		
		cFlag = 0;
		///////////////////////////////////////
		//set direction based on data. 
		if (filter_x>15) //right
		{
			if (filter_x>30)
				strcpy(command,"rH");
			else
				strcpy(command,"rM");
			cFlag++;
		}
		/////////////////////////////////////
		if (filter_x<-20) //left
		{
			if (filter_x<-35)
				strcpy(command,"lH");
			else
				strcpy(command,"lM");
			cFlag++;
		}
		/////////////////////////////////////////
		if (filter_y>20)
		{
			if (filter_y>30)
			{	
				if (filter_y>40)
					strcpy(command,"fH");
				else
					strcpy(command,"fM");
			}
			else
				strcpy(command,"fL");
			cFlag++;
		}
		////////////////////////////////////////////////
		if (filter_y<-20)
		{
			if (filter_y<-30)
			{	
				if (filter_y<-40)
					strcpy(command,"bH");
				else
					strcpy(command,"bM");
			}
			else
				strcpy(command,"bL");
			cFlag++;
		}
		

		if (cFlag > 1)
		{
			strcpy(command,"error");
		}
		else if (cFlag == 0)
		{
			strcpy(command,"s");
		}
		
		printf("command = %s\n\n",command);

		cFile = fopen("/dev/mycar", "r+");
		if (zeroCounter > 20)
		{
			zeroCounter = 33;
			strcpy(command,"s");
			fputs(command, cFile);
			//break;
		}
		else
			fputs(command, cFile); //put the command in the car controller device file. 
	}

	//fputs("s", cFile);
	close(wFile);
	fclose(cFile);
	return 0;
}
