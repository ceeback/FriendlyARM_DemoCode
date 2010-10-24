#include "def.h"
#include "2440addr.h"
#include "2440lib.h"

#define	puts	Uart_Printf
#define	printf	Uart_Printf
#define	getch	Uart_Getch

#ifndef DEBUG_VERSION
#define	USE_UART_INT
#endif

extern ULONG downloadAddress, downloadFileSize;
static UCHAR *temp;

#ifdef	USE_UART_INT
static void __irq Uart0RxInt(void)
{
    ClearSubPending(BIT_SUB_RXD0); //rSUBSRCPND = BIT_SUB_RXD0;          //Clear pending bit (Requested)
    ClearPending(BIT_UART0);

    *temp ++= RdURXH0();
}
#endif

void call_linux(ULONG a0, ULONG a1, ULONG a2);

void comdownload(void)
{
	ULONG size;
	UCHAR *buf;
	USHORT checksum;

	puts("\nNow download file from uart0...\n");
	downloadAddress = _NONCACHE_STARTADDRESS;
	buf  = (UCHAR *)downloadAddress;
	temp = buf-4;

	Uart_GetKey();

#ifdef	USE_UART_INT
	pISR_UART0 = (ULONG)Uart0RxInt;		//串口接收数据中断
	ClearSubPending(BIT_SUB_RXD0);
	ClearPending(BIT_UART0);
	EnableSubIrq(BIT_SUB_RXD0);
	EnableIrq(BIT_UART0);
#endif

	while((ULONG)temp<(ULONG)buf)
    {
 #ifdef	USE_UART_INT
        Led_Display(0);
        Delay(1000);
        Led_Display(15);
        Delay(1000);
#else
		*temp++ = Uart_Getch();
#endif
    }							//接收文件长度,4 bytes

	size  = *(ULONG *)(buf-4);
	downloadFileSize = size-6;

#ifdef	USE_UART_INT
    printf("Download File Size = %d\n", size);
#endif

	while(((ULONG)temp-(ULONG)buf)<(size-4))
	{
#ifdef	USE_UART_INT
		Led_Display(0);
        Delay(1000);
        Led_Display(15);
        Delay(1000);
#else
		*temp++ = Uart_Getch();
#endif
	}

#ifdef	USE_UART_INT
	DisableSubIrq(BIT_SUB_RXD0);
	DisableIrq(BIT_UART0);
#endif

#ifndef	USE_UART_INT
	printf("Download File Size = %d\n", size);
#endif

	checksum = 0;
	for(size=0; size<downloadFileSize; size++)
		checksum += buf[size];
	if(checksum!=(buf[size]|(buf[size+1]<<8))) {
		puts("Checksum fail!\n");
		return;
	}

	puts("Are you sure to run? [y/n]\n");
	while(1)
	{
		UCHAR key = getch();
		if(key=='n')
			return;
		if(key=='y')
			break;
	}

	call_linux(0, 193, downloadAddress);
}