#include "main.h"

unsigned char CheckSum(unsigned char *uBuff, int uBuffLen)
{

     unsigned char i,uSum=0;
     for(i=0;i<uBuffLen;i++)
     {
          uSum = uSum + uBuff[i];
     }
     uSum = (~uSum) + 1;
     return uSum;
}

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
	while(1)
	{
		memset(rBuf,0,MAXSIZE);
		tv.tv_sec = 1;//set the rcv wait time
		tv.tv_usec =5000;//100000us = 0.1s
		FD_ZERO(&rfds);
	   	FD_SET(serial_port_fd,&rfds);
	  	retval = select(serial_port_fd+1,&rfds,NULL,NULL,&tv);
		if(retval)
		{	
			int rs;		
			rs = read(serial_port_fd,rBuf,1);
			if(rs == 1)
			{
				if(rBuf[0] == 0x01)
				{
//					usleep(1000*20);
					rs = read(serial_port_fd,rBuf+1,7);
					if(rs == 7)
					{
						bufLen = (rBuf[4]+rBuf[5]*0xff)*4;
						printf("bufLen is %d \n",bufLen);
						usleep(1000);
						rs = read(serial_port_fd,rBuf+8,bufLen);
						if((rs == bufLen) && (bufLen < MAXSIZE - 8))
						{
							if(client_fd > 0)
							{
							//write(client_fd,rBuf,bufLen + 8);
								Packets_Dat(client_fd,rBuf);
							}
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
			}
			else
			{
				printf("in uart thread read 1 byte failed! rs = %d\n",rs);
			}

			retval = 0;	
#if	0		
			int i;
			for(i = 0; i < bufLen + 8; i++)
			printf("%02X ",rBuf[i]);
			printf("\n");
			bufLen = 0;
#endif
		}
		else 
		{
			printf("## in uart thread no data ,retval = %d\n",retval);
		}
	}
	printf("exit uart thread\n");
}

void *thread_socket()
{
	char buffers[MAXSIZE];
	struct sockaddr_in server_addr,client_addr;
	int sin_size,recvbytes;
	int port = readPortConf();

//creat
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("creat socket failed");
		exit(EXIT_FAILURE);
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
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("bind success !\n");

//listen
	if(listen(server_fd,BACKLOG) == -1)
	{
		perror("listen failed");
		exit(EXIT_FAILURE);
	}
	printf("listen ....\n");

//accept
	while(1)
	{
		if((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sin_size)) == -1)
		{
			perror("accept failed");
			exit(1);
		}
//		printf("client_fd = %d\n",client_fd);
		int i, len;
		char socket_buf[100];
		while(1)
		{
			len = read( client_fd, socket_buf, MAXSIZE);
			if(len <= 0)
			{
				perror("recvfrom:");
				close(client_fd);
				client_fd = 0;
				break;
			}
			else
			{
				printf("\nrecieve data from socket sueccess, data_len= %d\n",len);
				for(i=0;i<len;i++)
				{
					printf("%02X ",socket_buf[i]);
				}
				printf("\n");
				if(check_from_socket_data(socket_buf,len) == 0)
				{
					switch(socket_buf[2])
					{
						case R2000_INVENTORYINFINITE :
							printf(" CMD : R2000_INVENTORYINFINITE\n");
							AppEntry_R2000(serial_port_fd,R2000_INVENTORYINFINITE);
							break;
						case R2000_STOPINVENTORY :
							printf(" CMD : R2000_STOPINVENTORY\n");
							AppEntry_R2000(serial_port_fd,R2000_STOPINVENTORY);
							break;
						default:
							printf("Other CMD \n");
					}
				}
				else
				{
					printf("\nsocket : recieve data of invalid !\n ");
				}
#if 0
				if(memcmp(socket_buf,"start",5) == 0)
				{
					printf("socket : start\n");
					AppEntry_R2000(serial_port_fd,R2000_INVENTORYINFINITE);
				}else if(memcmp(socket_buf,"stop",4) == 0)
				{
					printf("socket :stop\n");
					AppEntry_R2000(serial_port_fd,R2000_STOPINVENTORY);
				}
#endif
			}
		}
	}
	close(server_fd);
	printf("exit uart thread\n");
}

int check_from_socket_data(unsigned char *buff , unsigned int length)
{
	if((buff[0] == 0xA0) && (buff[length - 1] == CheckSum(buff,length - 1)))
		return 0;
	else	return -1;	
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
//	printf("serial_port_fd  = %d \n",serial_port_fd );

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

	closeSerialPort(serial_port_fd);
	printf("main exit\n");
	return 0;
}
