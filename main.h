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

#define 	R2000_READTAG 				0x80
#define 	R2000_WRITETAG 				0x81
#define 	R2000_KILLTAG 				0x82
#define 	R2000_LOCKTAG 				0x83
#define 	R2000_BLOCKERASE 			0x84
#define 	R2000_QTREAD 				0x85
#define 	R2000_QTWRITE 				0x86
#define 	R2000_INVENTORYSINGLE 			0x87
#define 	R2000_INVENTORYINFINITE 		0x88
#define 	R2000_SETANTANNA 			0x89
#define 	R2000_SETANTANNAPOWER 			0x8A
#define 	R2000_SETFREQUENCY 			0x8B
#define 	R2000_STOPINVENTORY 			0x40
#define 	R2000_RESETREADER 			0x41
#define 	R2000_SETREMAINTIME 			0x43
#define 	R2000_SETSINGLEPARAMETER 		0x60
#define 	R2000_QUERYSINGLEPARAMETER 		0x61
#define 	R2000_SETMULTIPARAMETER 		0x62
#define 	R2000_QUERYMULTIPARAMETER 		0x63
#define 	R2000_SETANATNAINVENTORYCOUNT 		0x64
#define 	R2000_SETANATNASENSRESTHRSH 		0x65
#define 	R2000_OPTIONONETAG 			0x66
#define 	R2000_READVERSION 			0x67
#define 	R2000_MACERRORCODE 			0x68
#define 	R2000_CALIBRATIONGROSSGAIN 		0x69
#define 	R2000_RETRIEVE 				0x6A

#define 	MAXSIZE					8192

static long wm;

static unsigned char wBuf[MAXSIZE],rBuf[MAXSIZE];

static int speed_arr[] = {B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300,
		   B38400, B19200, B9600, B4800, B2400, B1200, B300};

static int name_arr[] = {230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300,
		  38400, 19200, 9600, 4800, 2400, 1200, 300};
static int server_fd;
static int client_fd;
static int serial_port_fd;
static int EPCCommand;
