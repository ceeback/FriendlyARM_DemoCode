/**************************************************************
The initial and control for 640×480 16Bpp TFT LCD----VGA
**************************************************************/

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"

#define CLKVAL_TFT_800x480	(1)
//FCLK = 180MHz, HCLK = PCLK = 90MHz
//VCLK = HCLK / [(CLKVAL+1) * 2]     ( CLKVAL >= 0 )
//VCLK = 45MHz		//  34MHz < VCLK < 40MHz

#define MVAL		(13)
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control
#define BPP24BL     (1)		//0 = LSB valid   1 = MSB Valid

#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

//TFT 800x480
#define LCD_XSIZE_TFT_800x480 	(800)
#define LCD_YSIZE_TFT_800x480 	(480)

//TFT 800x480
#define SCR_XSIZE_TFT_800x480 	(800)
#define SCR_YSIZE_TFT_800x480 	(480)

//TFT800x480
#define HOZVAL_TFT_800x480	(LCD_XSIZE_TFT_800x480-1)
#define LINEVAL_TFT_800x480	(LCD_YSIZE_TFT_800x480-1)

//以下参数只针对VGA
//# 640x400 @ 70 Hz, 31.5 kHz hsync
//Modeline "640x400" 25.175 640 664 760 800 400 409 411 450
#define HFPD_800x480		(15)		//水平同步信号的前肩
#define HSPW_800x480		(95)		//水平同步信号的脉宽
#define HBPD_800x480		(47)		//水平同步信号的后肩

#define VFPD_800x480		(9)		//垂直同步信号的前肩
#define VSPW_800x480		(1)		//垂直同步信号的脉宽
#define VBPD_800x480		(32)	//垂直同步信号的后肩

extern UCHAR car_800x480[];	//宽800，高480
extern UCHAR ipodtouch_800x480[];	//宽800，高480

volatile static unsigned short LCD_BUFER[SCR_YSIZE_TFT_800x480][SCR_XSIZE_TFT_800x480];

/**************************************************************
640×480 TFT LCD数据和控制端口初始化
**************************************************************/
static void Lcd_Port_Init( void )
{
    rGPCUP=0xffffffff; // Disable Pull-up register
    rGPCCON=0xaaaa02a8; //Initialize VD[7:0],VM,VFRAME,VLINE,VCLK

    rGPDUP=0xffffffff; // Disable Pull-up register
    rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]
}

/**************************************************************
640×480 TFT LCD功能模块初始化
**************************************************************/
static void Lcd_Init(void)
{
	rLCDCON1=(CLKVAL_TFT_800x480<<8)|(1<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,16bpp TFT,ENVID=off
	rLCDCON2=(VBPD_800x480<<24)|(LINEVAL_TFT_800x480<<14)|(VFPD_800x480<<6)|(VSPW_800x480);
	rLCDCON3=(HBPD_800x480<<19)|(HOZVAL_TFT_800x480<<8)|(HFPD_800x480);
	rLCDCON4=(MVAL<<8)|(HSPW_800x480);
	rLCDCON5=(1<<11)|(1<<9)|(1<<8)|(1<<3)|(1<<0);	//FRM5:6:5,HSYNC and VSYNC are inverted

	rLCDSADDR1=(((ULONG)LCD_BUFER>>22)<<21)|M5D((ULONG)LCD_BUFER>>1);
	rLCDSADDR2=M5D( ((ULONG)LCD_BUFER+(SCR_XSIZE_TFT_800x480*LCD_YSIZE_TFT_800x480*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_TFT_800x480-LCD_XSIZE_TFT_800x480)/1)<<11)|(LCD_XSIZE_TFT_800x480/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    rTCONSEL&=~((1<<4)|1); // Disable LCC3600, LPC3600
	rTPAL=0; // Disable Temp Palette
}

/**************************************************************
LCD视频和控制信号输出或者停止，1开启视频输出
**************************************************************/
static void Lcd_EnvidOnOff(int onoff)
{
    if(onoff==1)
	rLCDCON1|=1; // ENVID=ON
    else
	rLCDCON1 =rLCDCON1 & 0x3fffe; // ENVID Off
}

/**************************************************************
320×240 8Bpp TFT LCD 电源控制引脚使能
**************************************************************/
static void Lcd_PowerEnable(int invpwren,int pwren)
{
    //GPG4 is setted as LCD_PWREN
    rGPGUP = rGPGUP|(1<<4); // Pull-up disable
    rGPGCON = rGPGCON|(3<<8); //GPG4=LCD_PWREN

    //Enable LCD POWER ENABLE Function
    rLCDCON5 = rLCDCON5&(~(1<<3))|(pwren<<3);   // PWREN
    rLCDCON5 = rLCDCON5&(~(1<<5))|(invpwren<<5);   // INVPWREN
}

/**************************************************************
640×480 TFT LCD移动观察窗口
**************************************************************/
static void Lcd_MoveViewPort(int vx,int vy)
{
    ULONG addr;

    SET_IF();
	#if (LCD_XSIZE_TFT_800x480<32)
    	    while((rLCDCON1>>18)<=1); // if x<32
	#else
    	    while((rLCDCON1>>18)==0); // if x>32
	#endif
        addr=(ULONG)LCD_BUFER+(vx*2)+vy*(SCR_XSIZE_TFT_800x480*2);
	rLCDSADDR1= ( (addr>>22)<<21 ) | M5D(addr>>1);
	rLCDSADDR2= M5D(((addr+(SCR_XSIZE_TFT_800x480*LCD_YSIZE_TFT_800x480*2))>>1));
	CLR_IF();
}

/**************************************************************
640×480 TFT LCD移动观察窗口
**************************************************************/
static void MoveViewPort(void)
{
    int vx=0,vy=0,vd=1;

    Uart_Printf("\n*Move the LCD view windos:\n");
    Uart_Printf(" press 8 is up\n");
    Uart_Printf(" press 2 is down\n");
    Uart_Printf(" press 4 is left\n");
    Uart_Printf(" press 6 is right\n");
    Uart_Printf(" press Enter to exit!\n");

    while(1)
    {
    	switch(Uart_Getch())
    	{
    	case '8':
	    if(vy>=vd)vy-=vd;
        break;

    	case '4':
    	    if(vx>=vd)vx-=vd;
    	break;

    	case '6':
                if(vx<=(SCR_XSIZE_TFT_800x480-LCD_XSIZE_TFT_800x480-vd))vx+=vd;
   	    break;

    	case '2':
                if(vy<=(SCR_YSIZE_TFT_800x480-LCD_YSIZE_TFT_800x480-vd))vy+=vd;
   	    break;

    	case '\r':
   	    return;

    	default:
	    break;
		}
	Uart_Printf("vx=%3d,vy=%3d\n",vx,vy);
	Lcd_MoveViewPort(vx,vy);
    }
}

/**************************************************************
640×480 TFT LCD单个象素的显示数据输出
**************************************************************/
static void PutPixel(ULONG x,ULONG y,USHORT c)
{
    if(x<SCR_XSIZE_TFT_800x480 && y<SCR_YSIZE_TFT_800x480)
		LCD_BUFER[(y)][(x)] = c;
}

/**************************************************************
640×480 TFT LCD全屏填充特定颜色单元或清屏
**************************************************************/
static void Lcd_ClearScr( USHORT c)
{
	ULONG x,y ;

    for( y = 0 ; y < SCR_YSIZE_TFT_800x480 ; y++ )
    {
    	for( x = 0 ; x < SCR_XSIZE_TFT_800x480 ; x++ )
    	{
			LCD_BUFER[y][x] = c ;
    	}
    }
}

/**************************************************************
LCD屏幕显示垂直翻转
// LCD display is flipped vertically
// But, think the algorithm by mathematics point.
//   3I2
//   4 I 1
//  --+--   <-8 octants  mathematical cordinate
//   5 I 8
//   6I7
**************************************************************/
static void Glib_Line(int x1,int y1,int x2,int y2, USHORT color)
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

/**************************************************************
在LCD屏幕上画一个矩形
**************************************************************/
static void Glib_Rectangle(int x1,int y1,int x2,int y2, USHORT color)
{
    Glib_Line(x1,y1,x2,y1,color);
    Glib_Line(x2,y1,x2,y2,color);
    Glib_Line(x1,y2,x2,y2,color);
    Glib_Line(x1,y1,x1,y2,color);
}

/**************************************************************
在LCD屏幕上用颜色填充一个矩形
**************************************************************/
static void Glib_FilledRectangle(int x1,int y1,int x2,int y2, USHORT color)
{
    int i;

    for(i=y1;i<=y2;i++)
	Glib_Line(x1,i,x2,i,color);
}

/**************************************************************
在LCD屏幕上指定坐标点画一个指定大小的图片
**************************************************************/
static void Paint_Bmp(int x0,int y0,int h,int l,UCHAR bmp[])
{
	int x,y;
	ULONG c;
	int p = 0;

    for( y = 0 ; y < l ; y++ )
    {
    	for( x = 0 ; x < h ; x++ )
    	{
    		c = bmp[p+1] | (bmp[p]<<8) ;

			if ( ( (x0+x) < SCR_XSIZE_TFT_800x480) && ( (y0+y) < SCR_YSIZE_TFT_800x480) )
				LCD_BUFER[y0+y][x0+x] = c ;

    		p = p + 2 ;
    	}
    }
}

/**************************************************************
**************************************************************/
void Lcd_Tft_800x480_Init(void)
{

    Lcd_Init();
	LcdBkLtSet( 70 ) ;
	Lcd_PowerEnable(0, 1);
    Lcd_EnvidOnOff(1);		//turn on vedio

    Lcd_ClearScr( (0x00<<11) | (0x00<<5) | (0x00) );
    Paint_Bmp(0, 0, 800, 480,ipodtouch_800x480);

}

/**************************************************************
**************************************************************/
void Test_Lcd_TFT_640_480(void)
{
   	Uart_Printf("\nTest TFT LCD 640×480(VGA)!\n");

    Lcd_Port_Init();
    Lcd_Init();
    Lcd_EnvidOnOff(1);		//turn on vedio

	Lcd_ClearScr( (0x00<<11) | (0x00<<5) | (0x00)  )  ;		//clear screen
	Uart_Printf( "\nLCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x1f<<11) | (0x3f<<5) | (0x1f)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input
/*
	Lcd_ClearScr( (0x00<<11) | (0x00<<5) | (0x1f)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x00<<11) | (0x3f<<5) | (0x00)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x1f<<11) | (0x00<<5) | (0x00)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x00<<11) | (0x3f<<5) | (0x1f)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x1f<<11) | (0x00<<5) | (0x1f)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x1f<<11) | (0x3f<<5) | (0x00)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input
*/
	Lcd_ClearScr(0xffff);		//fill all screen with some color
	#define LCD_BLANK		30
	#define C_UP		( LCD_XSIZE_TFT_800x480 - LCD_BLANK*2 )
	#define C_RIGHT		( LCD_XSIZE_TFT_800x480 - LCD_BLANK*2 )
	#define V_BLACK		( ( LCD_YSIZE_TFT_800x480 - LCD_BLANK*4 ) / 6 )
	Glib_FilledRectangle( LCD_BLANK, LCD_BLANK, ( LCD_XSIZE_TFT_800x480 - LCD_BLANK ), ( LCD_YSIZE_TFT_800x480 - LCD_BLANK ),0x0000);		//fill a Rectangle with some color

	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*0), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*1),0x001f);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*1), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*2),0x07e0);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*2), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*3),0xf800);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*3), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*4),0xffe0);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*4), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*5),0xf81f);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*5), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*6),0x07ff);		//fill a Rectangle with some color
   	Uart_Printf( "LCD color test, please look! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

/*
    Paint_Bmp(0,0,640,480, girl0_640_480);		//paint a bmp
   	Uart_Printf( "LCD paint a bmp, please look! press any key to continue! \n" );
    Uart_Getch() ;		//wait uart input

    Paint_Bmp(0,0,640,480, girl1_640_480);		//paint a bmp
   	Uart_Printf( "LCD paint a bmp, please look! press any key to continue! \n" );
    Uart_Getch() ;		//wait uart input

    Paint_Bmp(0,0,640,480, flower1_640_480);		//paint a bmp
   	Uart_Printf( "LCD paint a bmp, please look! press any key to continue! \n" );
    Uart_Getch() ;		//wait uart input

    Paint_Bmp(0,0,640,480, girl2_640_480);		//paint a bmp
   	Uart_Printf( "LCD paint a bmp, please look! press any key to continue! \n" );
    Uart_Getch() ;		//wait uart input
*/
    Paint_Bmp(0,0,800,480, car_800x480);		//paint a bmp
   	Uart_Printf( "LCD paint a bmp, please look! press any key to continue! \n" );
    Uart_Getch() ;		//wait uart input

    Lcd_EnvidOnOff(0);		//turn off vedio
}
//*************************************************************
