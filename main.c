#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <errno.h>
#include <string.h>
#include <termio.h>
#include <termios.h>
#include <pthread.h>
#include <sys/sem.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define DEBUG
#define msleep(n)		usleep(1000*n)
#define ERROR			-1
#define BACKLOG			20

#define		R2000_INVENTORYINFINITE		0X88
#define 	R2000_STOPINVENTORY		0x40

static unsigned char wBuf[1024],rBuf[8192];

static int speed_arr[] = {B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300,
		   B38400, B19200, B9600, B4800, B2400, B1200, B300};

static int name_arr[] = {230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300,
		  38400, 19200, 9600, 4800, 2400, 1200, 300};
static int server_fd;
static int client_fd;
static int serial_port_fd;

int openSerialPort(const char *devName,unsigned int baudrate)
{
	int i,serial_fd = -1;
	serial_fd = open(devName, O_RDWR|O_NONBLOCK);
	if (serial_fd < 0) 
	{
		return ERROR;
	}
	struct termios serialAttr;
	memset(&serialAttr, 0, sizeof serialAttr);
	serialAttr.c_cflag = HUPCL | CREAD | CLOCAL;
	for(i = 0; i < sizeof(speed_arr)/sizeof(int); i++)
	{
		if(baudrate == name_arr[i])
		{
			cfsetispeed(&serialAttr,speed_arr[i]);
			cfsetospeed(&serialAttr,speed_arr[i]);
		}
	}
	serialAttr.c_cflag &= ~CSIZE; 
  	serialAttr.c_cflag |= CS8;                                   /* 数据位 8 */
  	serialAttr.c_cflag &= ~PARENB;                               /* Clear parity enable */
  	serialAttr.c_cflag &= ~CSTOPB;                               /* 停止位 1 */
  	serialAttr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  	serialAttr.c_oflag  &= ~OPOST;                                  /* Update the options and do it NOW */
      
        tcflush(serial_fd, TCIOFLUSH);
	if (tcsetattr(serial_fd, TCSANOW, &serialAttr) != 0) 
	{
		return ERROR;
	}
	return serial_fd;

}

void closeSerialPort(int fd)
{

	if(fd >= 0)
	{
		close(fd);
		fd = -1;	
	}
}

void R2000_Send(int fd,unsigned char *Uart_Send_Buf,unsigned int length)
{
//	tcflush(fd,TCIOFLUSH);
	int w = write(fd,Uart_Send_Buf,length);
#if 0
	printf("client_fd = %d, fd = %d\n",client_fd,fd);
	if(client_fd > 0)
	{
		write(client_fd,Uart_Send_Buf,length);
	}
#endif	
#if 0
	printf("R2000_Send w = %d\n",w);
	int i;
	printf("R2000_Send ");
	for(i = 0; i < length; i++)
	{
//		write(fd,Uart_Send_Buf[i],1);
		printf("%02X ",Uart_Send_Buf[i]);
	}
	printf("\n");
#endif
}

void AppEntry_R2000WriteRegister(int fd,unsigned char AddressLow,unsigned char AddressHigh,
unsigned char Data1,unsigned char Data2,unsigned char Data3,unsigned char Data4)/*写入寄存器操作*/
{
	printf(" AppEntry_R2000WriteRegister \n");
	unsigned char Uart_Send_Buf[8]={0};
	Uart_Send_Buf[0]=0x01;
	Uart_Send_Buf[1]=0x00;
	Uart_Send_Buf[2]=AddressLow;
	Uart_Send_Buf[3]=AddressHigh;
	Uart_Send_Buf[4]=Data1;
	Uart_Send_Buf[5]=Data2;
	Uart_Send_Buf[6]=Data3;
	Uart_Send_Buf[7]=Data4; 

	R2000_Send(fd,Uart_Send_Buf,8);

}

void AppEntry_StopInventory(int fd)/*停止读卡*/
{
	printf(" AppEntry_StopInventory \n");
	unsigned char Uart_Send_Buf[8]={0};
	Uart_Send_Buf[0]=0x40;
	Uart_Send_Buf[1]=0x01;
	Uart_Send_Buf[2]=0x00;
	Uart_Send_Buf[3]=0x00;
	Uart_Send_Buf[4]=0x00;
	Uart_Send_Buf[5]=0x00;
	Uart_Send_Buf[6]=0x00;
	Uart_Send_Buf[7]=0x00;
	
	R2000_Send(fd,Uart_Send_Buf,8);

}



void AppEntry_R2000CommandExcute(int fd,unsigned char R2000_Command)/*执行命令函数*/
{
	printf(" AppEntry_R2000CommandExcute \n");
	unsigned char Uart_Send_Buf[8]={0};
	
	Uart_Send_Buf[0]=0x01;
	Uart_Send_Buf[1]=0x00;
	Uart_Send_Buf[2]=0x00;
	Uart_Send_Buf[3]=0xF0;
	Uart_Send_Buf[4]=R2000_Command;
	Uart_Send_Buf[5]=0x00;
	Uart_Send_Buf[6]=0x00;
	Uart_Send_Buf[7]=0x00;
	
	R2000_Send(fd,Uart_Send_Buf,8);
}

void AppEntry_R2000(int fd ,unsigned char entry)
{
	switch(entry)
		{
			case R2000_STOPINVENTORY:/*Stop Inventory A0 02 40 1E*/
				AppEntry_StopInventory(fd);
				break;
			case R2000_INVENTORYINFINITE:/*Inventory Infinite Cycle*/
				//AppEntry_R2000WriteRegister(0x0D,0x01,0x00,0x00,0x00,0x00);
				//AppEntry_R2000WriteRegister(0x0E,0x01,0x00,0x00,0x00,0x00);
				AppEntry_R2000WriteRegister(fd,0x00,0x07,0xFF,0xFF,0x00,0x00);
				AppEntry_R2000CommandExcute(fd,0x0F);
				break;

		}
}

int readPortConf()
{
	int port = 5000;
	int p_fd = open("/etc/R2000.conf",O_RDONLY | O_CREAT,755);
	if(p_fd > 0)
	{
		unsigned char read_buff[5];
		read(p_fd,read_buff,5);
		port = atoi(read_buff);
	}
	if(port < 1024 || port > 65535)
	{
		system("echo 5000 > /etc/R2000.conf");
		printf("PORT: 1024 < port < 65535 \n");
		port = 5000;
	}
	return port;
}

void *thread_uart() 
{
	int bufLen = 0;
	int retval;
	fd_set rfds;
	struct timeval tv;
	printf("in uart thread serial_port_fd = %d\n",serial_port_fd);
	while(1)
	{
		memset(rBuf,0,8192);
		tv.tv_sec = 1;//set the rcv wait time
		tv.tv_usec =5000;//100000us = 0.1s
		FD_ZERO(&rfds);
	   	FD_SET(serial_port_fd,&rfds);
	  	retval = select(serial_port_fd+1,&rfds,NULL,NULL,&tv);
		printf("in uart thread retval = %d\n",retval);
		if(retval)
		{	
			int rs;		
			rs = read(serial_port_fd,rBuf,1);
			if(rs == 1)
			{
				if(rBuf[0] == 0x01)
				{
					usleep(1000*20);
					rs = read(serial_port_fd,rBuf+1,7);
					if(rs == 7)
					{
						bufLen = (rBuf[4]+rBuf[5]*0xff)*4;
						printf("bufLen is %d \n",bufLen);
						usleep(1000*20);
						rs = read(serial_port_fd,rBuf+8,bufLen);
						if(rs == bufLen)
						{
							if(client_fd > 0)
							write(client_fd,rBuf,bufLen + 8);
						}
						else
						{
							printf("in uart thread read bufLen bytes failed! rs = %d\n",rs);
						}
					}
					else
					{
						printf("in uart thread read 7 bytes failed! rs = %d\n",rs);
					}
				}
				else
				{
//					tcflush(serial_port_fd,TCIFLUSH);
				}
			}
			else
			{
				printf("in uart thread read 1 byte failed! rs = %d\n",rs);
			}

			retval = 0;	
#ifdef DEBUG1			
			int i;
			for(i = 0; i < bufLen + 8; i++)
			printf("%02X ",rBuf[i]);
			printf("\n");
			bufLen = 0;
#endif
		}
		else 
		{
//			if(client_fd > 0)
//				write(client_fd,"123 ",sizeof("123"));
		}
	}
	printf("exit uart thread\n");
}

void *thread_socket()
{
	char buffers[1024];
	struct sockaddr_in server_addr,client_addr;
	int sin_size,recvbytes;
	int port = readPortConf();

//creat
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("creat socket failed \n");
		exit(1);
	}
	printf("socket success ! server_fd = %d\n",server_fd);

//set server_addr
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero),8);

	int on=1;  
	if((setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)  
	{  
		perror("setsockopt failed");  
		exit(EXIT_FAILURE);  
	}  
 
//bind
	if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
	{
		printf("bind failed \n");
		exit(EXIT_FAILURE);
	}
	printf("bind success !\n");

//listen
	if(listen(server_fd,BACKLOG) == -1)
	{
		printf("listen failed \n");
		exit(EXIT_FAILURE);
	}
	printf("listen ....\n");

//accept
	while(1)
	{
		if((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sin_size)) == -1)
		{
			printf("accept failed \n");
			exit(1);
		}
		printf("client_fd = %d\n",client_fd);
		int i, len;
		char socket_buf[100];
		while(1)
		{
			len = read( client_fd, socket_buf, 100);
			if(len <= 0)
			{
				printf("recive data from socket fail !\n");
				perror("recvfrom:");
				close(client_fd);
				client_fd = 0;
				break;
//				exit(EXIT_FAILURE);
			}
			else
			{
				printf("\nrecieve data from socket sueccess, data_len= %d\n",len);
				for(i=0;i<len;i++)
				{
					printf("%c",socket_buf[i]);
				}
				printf("\n");
				if(memcmp(socket_buf,"start",5) == 0)
				{
					printf("socket : start\n");
					AppEntry_R2000(serial_port_fd,R2000_INVENTORYINFINITE);
				}else if(memcmp(socket_buf,"stop",4) == 0)
				{
					printf("socket :stop\n");
					AppEntry_R2000(serial_port_fd,R2000_STOPINVENTORY);
				}
#if 0
				len = write(fd_uart, socket_buf, len);    //send the data recived from socket to uart1
				if(len < 0)
				{
					printf("\nwrite data to uart fail\n");
				}
				else
				{
					printf("\nwrite data to uart success,data_len=%d\n",len);
				}
#endif
			}
		}
	}
	close(server_fd);
	printf("exit uart thread\n");
}

int main()
{
	int res;
	pthread_t socket_thread, uart_thread;
	void *thread_result;

	serial_port_fd = openSerialPort("/dev/ttySAC3",115200);
	if (serial_port_fd < 0) 
	{
		perror("open serialport failed !\n");
		exit(EXIT_FAILURE);
	}
	printf("serial_port_fd  = %d \n",serial_port_fd );

	res=pthread_create( &socket_thread, NULL, thread_socket, 0 );
	if(res != 0){
		perror("Thread_socket creation failed");
		exit(EXIT_FAILURE);
	}

	res=pthread_create( &uart_thread, NULL, thread_uart, 0 );
	if(res != 0){
		perror("Thread_uart creation failed");
		exit(EXIT_FAILURE);
	}

	res=pthread_join( socket_thread, &thread_result );
	if(res != 0){
		perror("Thread_socket join failed");
		exit(EXIT_FAILURE);
	}

	res=pthread_join( uart_thread, &thread_result );
	if(res != 0){
		perror("Thread_uart join failed");
		exit(EXIT_FAILURE);
	}

#if 0
	fd = openSerialPort("/dev/ttySAC3",115200);
	if (fd < 0) 
	{
		printf("open serialport failed !\n");
		exit(1);
	}

	
	AppEntry_R2000(fd,R2000_INVENTORYINFINITE);
	msleep(500);

	while(1)
	{
		readLength = read(fd,rBuf,1024);
		if(readLength > 0)
		{
			printf("readLength is %d \n",readLength);
			for(i = 0; i < readLength; i++)
			printf("%02X ",rBuf[i]);
			printf("\n");
		}
		msleep(500);
		j++;
		if(j > 20) break;
	}


//	readData(fd);
	msleep(5000);
	AppEntry_R2000(fd,R2000_STOPINVENTORY);	
//	readData(fd);
	closeSerialPort(fd);
#endif
	closeSerialPort(serial_port_fd);
	printf("main exit\n");
	return 0;
}
