/**************************************************************
4*4 Key Scan
**************************************************************/

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"

/******************************************************************************
	1X6 矩阵键盘
六个输入引脚：	EINT8 -----( GPG0  )
				EINT11 -----( GPG3  )
				EINT13-----( GPG5  )
				EINT14-----( GPG6 )
				EINT15-----( GPG7 )
				EINT19-----( GPG11 )


******************************************************************************/
UCHAR Key_Scan( void )
{
	Delay( 80 ) ;

	if(      (rGPGDAT&(1<< 0)) == 0 )
		return 1 ;
	else if( (rGPGDAT&(1<< 3)) == 0 )
		return 2;
	else if( (rGPGDAT&(1<< 5)) == 0 )
		return 3 ;
	else if( (rGPGDAT&(1<< 6)) == 0 )
		return 4 ;
	else if( (rGPGDAT&(1<< 7)) == 0 )
		return 5 ;
	else if( (rGPGDAT&(1<<11)) == 0 )
		return 6 ;
	else
		return 0xff;

}

static void __irq Key_ISR(void)
{
	UCHAR key;
	ULONG r;

	EnterCritical(&r);
	if(rINTPND==BIT_EINT8_23) {
		ClearPending(BIT_EINT8_23);
		if(rEINTPEND&(1<<8)) {
		//Uart_Printf("eint11\n");
			rEINTPEND |= 1<< 8;
		}
		if(rEINTPEND&(1<<11)) {
		//Uart_Printf("eint11\n");
			rEINTPEND |= 1<< 11;
		}
		if(rEINTPEND&(1<<13)) {
		//Uart_Printf("eint11\n");
			rEINTPEND |= 1<< 13;
		}
		if(rEINTPEND&(1<<14)) {
		//Uart_Printf("eint11\n");
			rEINTPEND |= 1<< 14;
		}
		if(rEINTPEND&(1<<15)) {
		//Uart_Printf("eint11\n");
			rEINTPEND |= 1<< 15;
		}
		if(rEINTPEND&(1<<19)) {
		//	Uart_Printf("eint19\n");
			rEINTPEND |= 1<< 19;
		}
	}

	key=Key_Scan();
	if( key == 0xff )
		Uart_Printf( "Interrupt occur... Key is released!\n") ;
	else
		Uart_Printf( "Interrupt occur... K%d is pressed!\n", key) ;

	ExitCritical(&r);
}

void KeyScan_Test(void)
{
	rGPGCON = rGPGCON & (~((3<<22)|(3<<6)|(3<<0)|(3<<10)|(3<<12)|(3<<14))) |
						   ((2<<22)|(2<<6)|(2<<0)|(2<<10)|(2<<12)|(2<<14)) ;		//GPG11,3 set EINT
    /* Key1~6,按键中断设置为低电平中断 */
	rEXTINT1 &= ~(7<<0);
	rEXTINT1 |= (0<<0);	//set eint8 falling edge int

	rEXTINT1 &= ~(7<<12);
	rEXTINT1 |= (0<<12);	//set eint11 falling edge int

	rEXTINT1 &= ~(7<<20);
	rEXTINT1 |= (0<<20);	//set eint13 falling edge int

	rEXTINT1 &= ~(7<<24);
	rEXTINT1 |= (0<<24);	//set eint14 falling edge int

	rEXTINT1 &= ~(7<<28);
	rEXTINT1 |= (0<<28);	//set eint15 falling edge int

	rEXTINT2 &= ~(0xf<<12);
	rEXTINT2 |= (0<<12);	//set eint19 falling edge int

    /* 清空中断指示 */
	rEINTPEND |= (1<<8)|(1<<11)|(1<<13)|(1<<14)|(1<<15)|(1<<19);		//clear eint 11,19
	/* 开启中断使能 */
	rEINTMASK &= ~((1<<8)|(1<<11)|(1<<13)|(1<<14)|(1<<15)|(1<<19));	//enable eint11,19
	ClearPending(BIT_EINT0|BIT_EINT2|BIT_EINT8_23);
	pISR_EINT0 = pISR_EINT2 = pISR_EINT8_23 = (ULONG)Key_ISR;
	EnableIrq(BIT_EINT0|BIT_EINT2|BIT_EINT8_23);

    Uart_Printf("\nKey Scan Test, press ESC key to exit !\n");
	while( Uart_GetKey() != ESC_KEY ) ;
	DisableIrq(BIT_EINT0|BIT_EINT2|BIT_EINT8_23);
}
