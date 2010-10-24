/**************************************************************
The initial and control for 320��240 16Bpp TFT LCD----LCD_LTV350QV-F04
**************************************************************/

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "LCD_LTS350Q1_PE1.h"
#include "config_GAO.h"

#define MVAL		(13)
#define MVAL_USED 	(0)		//0=each frame   1=rate by MVAL
#define INVVDEN		(1)		//0=normal       1=inverted
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control

#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

//TFT 240320
#define LCD_XSIZE_TFT_240320 	(240)
#define LCD_YSIZE_TFT_240320 	(320)

#define SCR_XSIZE_TFT_240320 	(240)
#define SCR_YSIZE_TFT_240320 	(320)

//TFT240320
#define HOZVAL_TFT_240320	(LCD_XSIZE_TFT_240320-1)
#define LINEVAL_TFT_240320	(LCD_YSIZE_TFT_240320-1)

//Timing parameter for NEC3.5"
#define VBPD_240320		(1)		//��ֱͬ���źŵĺ��
#define VFPD_240320		(5)		//��ֱͬ���źŵ�ǰ��
#define VSPW_240320		(1)		//��ֱͬ���źŵ�����

#define HBPD_240320		(36)		//ˮƽͬ���źŵĺ��
#define HFPD_240320		(19)		//ˮƽͬ���źŵ�ǰ��
#define HSPW_240320		(5)		//ˮƽͬ���źŵ�����

#define CLKVAL_TFT_240320	(4)
//FCLK=180MHz,HCLK=90MHz,VCLK=6.5MHz



extern void Uart_Printf(char *f, ...) ;


static void Lcd_PowerEnable(int invpwren,int pwren);

volatile static unsigned short LCD_BUFFER[SCR_YSIZE_TFT_240320][SCR_XSIZE_TFT_240320];

/**************************************************************
320��240 16Bpp TFT LCD����ģ���ʼ��
**************************************************************/

static void Lcd_Init(void)
{
    // GPC �˿���Һ���ź��߼�����������.
    rGPCUP  = 0x00000000;  // 0:��������
	rGPCCON = 0xaaaa02a9;  // ��׼TFT����.

    // GPD �˿���Ҫ�ǲ���������
    rGPDUP  = 0x00000000; // 0:��������
    rGPDCON = 0xaaaaaaaa; // GPD �˿�ȫ����Ϊ VD[23:8]
    // TFT mode,16bpp.
	rLCDCON1=(CLKVAL_TFT_240320<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD_240320<<24)|(LINEVAL_TFT_240320<<14)|(VFPD_240320<<6)|(VSPW_240320);
	rLCDCON3=(HBPD_240320<<19)|(HOZVAL_TFT_240320<<8)|(HFPD_240320);
	rLCDCON4=(MVAL<<8)|(HSPW_240320);
    // 16bpp,5:6:5 format
    rLCDCON5 = (1<<11) | (1<<10) | (1<<9) | (1<<8) | (0<<7) | (0<<6)
             | (1<<3)  |(BSWP<<1) | (HWSWP);

	rLCDSADDR1=(((ULONG)LCD_BUFFER>>22)<<21)|M5D((ULONG)LCD_BUFFER>>1);
	rLCDSADDR2=M5D( ((ULONG)LCD_BUFFER+(SCR_XSIZE_TFT_240320*LCD_YSIZE_TFT_240320*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_TFT_240320-LCD_XSIZE_TFT_240320)/1)<<11)|(LCD_XSIZE_TFT_240320/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    rTCONSEL &= (~7) ;     // Disable LPC3480
	rTPAL=0; // Disable Temp Palette
}

/**************************************************************
LCD��Ƶ�Ϳ����ź��������ֹͣ��1������Ƶ���
**************************************************************/
static void Lcd_EnvidOnOff(int onoff)
{
    if(onoff==1)
	rLCDCON1|=1; // ENVID=ON
    else
	rLCDCON1 =rLCDCON1 & 0x3fffe; // ENVID Off
}

/**************************************************************
240��320 8Bpp TFT LCD ��Դ��������ʹ��(ע��:��NEC3.5" ��������ʵ�ʲ�û�иú�����Ӧ��Ӳ��֧��)
**************************************************************/
static void Lcd_PowerEnable(int invpwren,int pwren)
{
    //GPG4 is setted as LCD_PWREN
    rGPGUP=rGPGUP&(~(1<<4))|(1<<4); // Pull-up disable
    rGPGCON=rGPGCON&(~(3<<8))|(3<<8); //GPG4=LCD_PWREN
    rGPGDAT = rGPGDAT | (1<<4) ;
//	invpwren=pwren;
    //Enable LCD POWER ENABLE Function
    rLCDCON5=rLCDCON5&(~(1<<3))|(pwren<<3);   // PWREN
    rLCDCON5=rLCDCON5&(~(1<<5))|(invpwren<<5);   // INVPWREN
}

/**************************************************************
240��320 16Bpp TFT LCD�������ص���ʾ�������
**************************************************************/
static void PutPixel(ULONG x,ULONG y, ULONG c )
{
	if ( (x < SCR_XSIZE_TFT_240320) && (y < SCR_YSIZE_TFT_240320) )
		LCD_BUFFER[(y)][(x)] = c;
}


/**************************************************************
240��320 16Bpp TFT LCDȫ������ض���ɫ��Ԫ������
**************************************************************/
static void Lcd_ClearScr( ULONG c)
{
	ULONG x,y ;

    for( y = 0 ; y < SCR_YSIZE_TFT_240320 ; y++ )
    {
    	for( x = 0 ; x < SCR_XSIZE_TFT_240320 ; x++ )
    	{
			LCD_BUFFER[y][x] = c ;
    	}
    }
}

/**************************************************************
LCD��Ļ��ʾ��ֱ��ת
// LCD display is flipped vertically
// But, think the algorithm by mathematics point.
//   3I2
//   4 I 1
//  --+--   <-8 octants  mathematical cordinate
//   5 I 8
//   6I7
**************************************************************/
static void Glib_Line(int x1,int y1,int x2,int y2,int color)
{
	int dx,dy,e;
	dx=x2-x1;
	dy=y2-y1;

	if(dx>=0)
	{
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 1/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}
					x1+=1;
					e+=dy;
				}
			}
			else		// 2/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 8/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}
					x1+=1;
					e+=dy;
				}
			}
			else		// 7/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}
					y1-=1;
					e+=dx;
				}
			}
		}
	}
	else //dx<0
	{
		dx=-dx;		//dx=abs(dx)
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 4/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}
					x1-=1;
					e+=dy;
				}
			}
			else		// 3/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 5/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}
					x1-=1;
					e+=dy;
				}
			}
			else		// 6/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}
					y1-=1;
					e+=dx;
				}
			}
		}
	}
}



#if 0
/**************************************************************
��LCD��Ļ�ϻ�һ������
**************************************************************/
static void Glib_Rectangle(int x1,int y1,int x2,int y2,int color)
{
    Glib_Line(x1,y1,x2,y1,color);
    Glib_Line(x2,y1,x2,y2,color);
    Glib_Line(x1,y2,x2,y2,color);
    Glib_Line(x1,y1,x1,y2,color);
}
#endif /* 0 */


/**************************************************************
��LCD��Ļ������ɫ���һ������
**************************************************************/
static void Glib_FilledRectangle(int x1,int y1,int x2,int y2,int color)
{
    int i;

    for(i=y1;i<=y2;i++)
	Glib_Line(x1,i,x2,i,color);
}


/**************************************************************
��LCD��Ļ��ָ������㻭һ��ָ����С��ͼƬ
**************************************************************/
static void Paint_Bmp(int x0,int y0,int h,int l,UCHAR bmp[])
{
	int x,y;
	ULONG c;
	int p = 0;

    for( y = y0 ; y < l ; y++ )
    {
    	for( x = x0 ; x < h ; x++ )
    	{
    		c = bmp[p+1] | (bmp[p]<<8) ;

			if ( ( (x0+x) < SCR_XSIZE_TFT_240320) && ( (y0+y) < SCR_YSIZE_TFT_240320) )
				LCD_BUFFER[y0+y][x0+x] = c ;

    		p = p + 2 ;
    	}
    }
}

/*************************************************************/
void Lcd_Tft_NEC35_Init(void)
{
    //qjy: turn on the blacklight!
    GPB1_TO_OUT();
    GPB1_TO_1();

    Lcd_Init();
	LcdBkLtSet( 70 ) ;
	Lcd_PowerEnable(0, 1);
    Lcd_EnvidOnOff(1);		//turn on vedio

    Lcd_ClearScr( (0x00<<11) | (0x00<<5) | (0x00) );
    Paint_Bmp(0, 0, 240, 320,iphone_unlock_240x320);

}


void Lcd_Tft_NEC35_Test( void )
{
 	// Lcd_Tft_LTS350Q1_PE1_Init();
   	Uart_Printf("\nTest NEC3.5\" (TFT LCD)!\n");

	Lcd_ClearScr( (0x00<<11) | (0x00<<5) | (0x00)  )  ;		//clear screen
	Uart_Printf( "\nLCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x1f<<11) | (0x3f<<5) | (0x1f)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x00<<11) | (0x00<<5) | (0x1f)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x00<<11) | (0x3f<<5) | (0x00)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr(0xffff);		//fill all screen with some color
	#define LCD_BLANK		12
	#define C_UP		( LCD_XSIZE_TFT_240320 - LCD_BLANK*2 )
	#define C_RIGHT		( LCD_XSIZE_TFT_240320 - LCD_BLANK*2 )
	#define V_BLACK		( ( LCD_YSIZE_TFT_240320 - LCD_BLANK*4 ) / 6 )
	Glib_FilledRectangle( LCD_BLANK, LCD_BLANK, ( LCD_XSIZE_TFT_240320 - LCD_BLANK ), ( LCD_YSIZE_TFT_240320 - LCD_BLANK ),0x0000);		//fill a Rectangle with some color

	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*0), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*1),0x001f);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*1), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*2),0x07e0);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*2), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*3),0xf800);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*3), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*4),0xffe0);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*4), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*5),0xf81f);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*5), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*6),0x07ff);		//fill a Rectangle with some color

	Glib_Line( LCD_BLANK,LCD_BLANK, (LCD_XSIZE_TFT_240320-LCD_BLANK), (LCD_YSIZE_TFT_240320-LCD_BLANK), 0x0000 ) ;
	Glib_Line( LCD_BLANK,(LCD_YSIZE_TFT_240320-LCD_BLANK), (LCD_XSIZE_TFT_240320-LCD_BLANK), LCD_BLANK, 0x0000 ) ;
	Glib_Line( (LCD_XSIZE_TFT_240320/2),(LCD_BLANK*2 + V_BLACK*0), (LCD_XSIZE_TFT_240320/2), (LCD_BLANK*2 + V_BLACK*6), 0x0000 ) ;
	Uart_Getch() ;		//Any Key To Next


    Paint_Bmp(0, 0, 240, 320, flower1_240_320);
    Paint_Bmp(0, 0, 240, 320, FriendlyARM_pic_240x320);
    Uart_Printf("bmp,Any Key To Next!\n");
    Uart_Getch() ;

}
//*************************************************************

