#include "main.h"

void R2000_Send(int fd,unsigned char *Uart_Send_Buf,unsigned int length)
{
	write(fd,Uart_Send_Buf,length);
}

void AppEntry_R2000WriteRegister(int fd,unsigned char AddressLow,unsigned char AddressHigh,
unsigned char Data1,unsigned char Data2,unsigned char Data3,unsigned char Data4)/*写入寄存器操作*/
{
//	printf(" AppEntry_R2000WriteRegister \n");
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
//	printf(" AppEntry_StopInventory \n");
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
//	printf(" AppEntry_R2000CommandExcute \n");
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
				EPCCommand = R2000_INVENTORYINFINITE;
				break;

		}
}

static unsigned char recvbuffer[MAXSIZE];
void Packets_Dat(int fd,unsigned char *databuf)/*写的不够严谨，需要重新改写，在下面一些赋值数据中需要先判断接收的字节是否足够0x0006*/
{
	unsigned int x=databuf[2]+databuf[3]*256;
	//unsigned int datalength=databuf[5]+databuf[6]*0xff;/*Data 长度单位字节*/
	unsigned int datalength=databuf[4]+databuf[5]*0xff;
	unsigned char ExcuteCommd;
	unsigned int j;
	long ErrorCode;
	unsigned char CheckS;
	unsigned char value[2]={0};
	
	switch(x)
	{
		case 0x0000://Uart0_Send("开始包:\r\n", 9);
			if(wm==0)/*是否是开始包*/
			{
				if((EPCCommand!=R2000_INVENTORYSINGLE) && (EPCCommand!=R2000_INVENTORYINFINITE) && (EPCCommand!=R2000_OPTIONONETAG) && (EPCCommand!=R2000_CALIBRATIONGROSSGAIN))/*读单卡或多卡不发送'   头'形式为标签个数+ 标签*/
				{
					recvbuffer[wm++]=0xE0;
					recvbuffer[wm++]=0x00;
					recvbuffer[wm++]=EPCCommand;
				}
			}
			break;
		case 0x0001://Uart0_Send("结束包:\r\n", 9);
			ErrorCode=databuf[13]*256+databuf[12];
			if(ErrorCode==0x00)/*判断是否报错，这个错是指在执行命令过程中，返回的错误*/
			{
				if((EPCCommand!=R2000_INVENTORYSINGLE) && (EPCCommand!=R2000_INVENTORYINFINITE) && (EPCCommand!=R2000_OPTIONONETAG) && (EPCCommand!=R2000_CALIBRATIONGROSSGAIN))/*如果是读单卡或多卡则不需要尾部*/
				{
					if(wm!=3)/*返回数据信息E0 Length EPCCommand Data1 Data2....Datan CheckSum*/
					{
						recvbuffer[1]=wm-1;
						CheckS = CheckSum(recvbuffer, wm) ;
						recvbuffer[wm]=CheckS;
						TCP_Send_PC(fd,recvbuffer, wm+1);
					}
					else/*返回状态信息E4 03 EPCCommand 00 CheckSum*/
					{
						unsigned char data[5]={0};
						data[0]=0xE4;
						data[1]=0x03;
						data[2]=EPCCommand;
						data[3]=0x00;
						CheckS = CheckSum(data, 4) ;
						data[4]=CheckS;
						TCP_Send_PC(fd,data, 5);
					}
				}
				else if((EPCCommand==R2000_INVENTORYSINGLE) || (EPCCommand==R2000_OPTIONONETAG) || (EPCCommand==R2000_CALIBRATIONGROSSGAIN))
				{
					for(j=0;j<100;j++)
					value[0]=0xFF;
					value[1]=0x00;
					TCP_Send_PC(fd,value, 2);
				}
				wm=0;
				
			}
			else/*返回错误信息代码*/
			{
				recvbuffer[0]=0xE4;
				recvbuffer[1]=0x04;
				recvbuffer[2]=EPCCommand;
				recvbuffer[3]=databuf[12];
				recvbuffer[4]=databuf[13];
				CheckS = CheckSum(recvbuffer, 5) ;
				recvbuffer[5]=CheckS;
				TCP_Send_PC(fd,recvbuffer, 6);
			}
			break;
		case 0x0005://Uart0_Send("读标包\r\n", 8);
			if((EPCCommand==R2000_INVENTORYSINGLE) || (EPCCommand==R2000_INVENTORYINFINITE) || (EPCCommand==R2000_OPTIONONETAG))/*判断是否为读标签--读单卡或多卡*/
			{
				unsigned char wm1=0;
				unsigned char recvbuf[14]={0};
				recvbuf[wm1++]=0x00;
				for(j=0;j<12;j++)
				{
					recvbuf[wm1++]=databuf[22+j];     
				}
				recvbuf[wm1++]=0xFF;
				TCP_Send_PC(fd,recvbuf, 14);
			}
			else
			{
				for(j=0;j<12;j++)
				{
					recvbuffer[wm++]=databuf[22+j];
				}
			}
			break;
		
		case 0x000E:/*等待超时*/
			value[0]=0xFF;
			value[1]=0xFF;
			TCP_Send_PC(fd,value, 2);
			break;
		case 0x300E:/*Calibration Data*/
			if (1)/* 为什么这样写if(1) ，因为这里是GCC 编译器的一个BUG，在CASE语句里不能直接声明变量，否则编译出错。但是加上if语句后在if语句里面可以声明变量，可以编译成功。*/
			{
				unsigned char wm2=0;
				unsigned char recvbuf1[34]={0};
				recvbuf1[wm2++]=0x00;
				for(j=0;j<32;j++)
				{
					recvbuf1[wm2++]=databuf[8+j];     
				}
				recvbuf1[wm2++]=0xFF;
				TCP_Send_PC(fd,recvbuf1, 34);
			}
			break;
		default:	
			//Uart0_Send("错误包\r\n", 8);
			TCP_Send_PC(fd,databuf+2, 2);
			break;
	}
}

void TCP_Send_PC(int fd,unsigned char *databuf,int length)
{
	write(fd,databuf,length);
}
