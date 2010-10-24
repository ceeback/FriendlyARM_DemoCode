/************************************************************
File Name	: camif.c
Descriptions
 -S3C2440 camera test routines & basic libraries
History
 - July 23, 2003. Draft Version 0.0 by purnnamu
 - Janualy 15, 2004. Modifed by Boaz

Copyright 2004 SAMSUNG Electronics.
However, Anybody can use this code without our permission.
*************************************************************/

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"

#include "camif.h"
#include "camproset.h" // for camera setting

extern UCHAR camera_pic_240x320[];
extern UCHAR flower1_240_320[];
extern void Lcd_Tft_NEC35_Init(void);

//*****************************************************************************
#define MVAL		(13)
#define MVAL_USED 	(0)		//0=each frame   1=rate by MVAL
#define INVVDEN		(1)		//0=normal       1=inverted
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control

#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

#if LCD_TYPE==LCD_TYPE_N35
//TFT 240320
#define LCD_XSIZE_TFT 	(240)
#define LCD_YSIZE_TFT 	(320)

//TFT 240320
#define SCR_XSIZE_TFT 	(240)
#define SCR_YSIZE_TFT 	(320)
//#define SCR_XSIZE_TFT 	(320)
//#define SCR_YSIZE_TFT 	(240)

//TFT240320
#define HOZVAL_TFT	(LCD_XSIZE_TFT-1)
#define LINEVAL_TFT	(LCD_YSIZE_TFT-1)

//Timing parameter for LCD LQ035Q7DB02
#define VBPD		(1)		//垂直同步信号的后肩
#define VFPD		(5)		//垂直同步信号的前肩
#define VSPW		(1)		//垂直同步信号的脉宽

#define HBPD		(36)		//水平同步信号的后肩
#define HFPD		(19)		//水平同步信号的前肩
#define HSPW		(5)		//水平同步信号的脉宽

#define CLKVAL_TFT	(4)
//FCLK=180MHz,HCLK=90MHz,VCLK=6.5MHz

#elif LCD_TYPE==LCD_TYPE_A70
//TFT 240320
#define LCD_XSIZE_TFT 	(800)
#define LCD_YSIZE_TFT 	(480)

//TFT 240320
#define SCR_XSIZE_TFT 	(800)
#define SCR_YSIZE_TFT 	(480)

//TFT240320
#define HOZVAL_TFT	(LCD_XSIZE_TFT-1)
#define LINEVAL_TFT	(LCD_YSIZE_TFT-1)

#define VBPD		(32)		//垂直同步信号的后肩
#define VFPD		(9)		//垂直同步信号的前肩
#define VSPW		(1)		//垂直同步信号的脉宽

#define HBPD		(47)		//水平同步信号的后肩
#define HFPD		(15)		//水平同步信号的前肩
#define HSPW		(95)		//水平同步信号的脉宽

#define CLKVAL_TFT	(2)
//FCLK=180MHz,HCLK=90MHz,VCLK=6.5MHz

#elif LCD_TYPE==LCD_TYPE_VGA1024x768
//TFT 240320
#define LCD_XSIZE_TFT 	(1024)
#define LCD_YSIZE_TFT 	(768)

//TFT 240320
#define SCR_XSIZE_TFT 	(1024)
#define SCR_YSIZE_TFT 	(768)

//TFT240320
#define HOZVAL_TFT	(LCD_XSIZE_TFT-1)
#define LINEVAL_TFT	(LCD_YSIZE_TFT-1)

#define VBPD		(1)		//垂直同步信号的后肩
#define VFPD		(1)		//垂直同步信号的前肩
#define VSPW		(1)		//垂直同步信号的脉宽

#define HBPD		(15)		//水平同步信号的后肩
#define HFPD		(199)		//水平同步信号的前肩
#define HSPW		(15)		//水平同步信号的脉宽

#define CLKVAL_TFT	(3)
//FCLK=180MHz,HCLK=90MHz,VCLK=6.5MHz
#endif



volatile static unsigned short LCD_BUFFER_CAM[SCR_YSIZE_TFT][SCR_XSIZE_TFT];
//*****************************************************************************

volatile ULONG camTestMode;
volatile ULONG camCodecCaptureCount;
volatile ULONG camPviewCaptureCount;
volatile ULONG camCodecStatus;
volatile ULONG camPviewStatus;
volatile ULONG amount;

ULONG save_GPJCON, save_GPJDAT, save_GPJUP;

UCHAR flagCaptured_P = 0;
UCHAR flagCaptured_C = 0;

int Test_OV9650(void);	//capbily

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
TFT LCD 电源控制引脚使能
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
//*****************************************************************************
static void LCD_Init(void)
{
    //qjy: turn on the blacklight!
   // GPB1_TO_OUT(); //REM, capbily
   // GPB1_TO_1();   //REM, capbily

//    rGPCUP=0xffffffff; // Disable Pull-up register
    rGPCUP  = 0x00000000;
  //  rGPCCON=0xaaaa56a9; //Initialize VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND
	 rGPCCON = 0xaaaa02a9;

//    rGPDUP=0xffffffff; // Disable Pull-up register
     rGPDUP  = 0x00000000;
   rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]

	rLCDCON1=(CLKVAL_TFT<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD<<24)|(LINEVAL_TFT<<14)|(VFPD<<6)|(VSPW);
	rLCDCON3=(HBPD<<19)|(HOZVAL_TFT<<8)|(HFPD);
	rLCDCON4=(MVAL<<8)|(HSPW);
#if LCD_TYPE==LCD_TYPE_VGA1024x768
	rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<3)|(1<<0);
#else
//	rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<6)|(BSWP<<1)|(HWSWP);	//FRM5:6:5,HSYNC and VSYNC are inverted
    rLCDCON5 = (1<<11) | (1<<10) | (1<<9) | (1<<8) | (0<<7) | (0<<6)
             | (1<<3)  |(BSWP<<1) | (HWSWP);
#endif

	rLCDSADDR1=(((ULONG)LCD_BUFFER_CAM>>22)<<21)|M5D((ULONG)LCD_BUFFER_CAM>>1);
	rLCDSADDR2=M5D( ((ULONG)LCD_BUFFER_CAM+(SCR_XSIZE_TFT*LCD_YSIZE_TFT*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_TFT-LCD_XSIZE_TFT)/1)<<11)|(LCD_XSIZE_TFT/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    rTCONSEL &= (~7) ;     // Disable LPC3480
   //  rTCONSEL&=~((1<<4)|1); // Disable LCC3600, LPC3600
	rTPAL=0; // Disable Temp Palette
	LcdBkLtSet( 70 ) ;
	Lcd_PowerEnable(0, 1);
    Lcd_EnvidOnOff(1);		//turn on vedio

}
//*****************************************************************************
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

			if ( ( (x0+x) < SCR_XSIZE_TFT) && ( (y0+y) < SCR_YSIZE_TFT) )
				 LCD_BUFFER_CAM[y0+y][x0+x] = c ;

    		p = p + 2 ;
    	}
    }
}

void Camera_Test(void)
{
	int i;

	Uart_Printf("\nCamera Preview Test\n");

	CamReset();

	// Initializing camif
	rCLKCON |= (1<<19); // enable camclk
	CamPortSet();
	//ChangeUPllValue(60, 4, 1);		// UPLL clock = 96MHz, PLL input 16.9344MHz
	//--- capbily
	ChangeUPllValue(56, 2, 1);		// UPLL clock = 96MHz, PLL input 12MHz
	//---
	rCLKDIVN|=(1<<3); // UCLK 48MHz setting for UPLL 96MHz
	// 0:48MHz, 1:24MHz, 2:16MHz, 3:12MHz...
	// Camera clock = UPLL/[(CAMCLK_DIV+1)X2]

	switch(USED_CAM_TYPE)
	{
	case CAM_AU70H :
		if (AU70H_VIDEO_SIZE==1152)
			SetCAMClockDivider(CAMCLK24000000); //Set Camera Clock for SXGA
		if (AU70H_VIDEO_SIZE==640)
			SetCAMClockDivider(CAMCLK16000000); //Set Camera Clock for VGA
		break;
	case CAM_S5X3A1 :
		SetCAMClockDivider(CAMCLK24000000); //Set Camera Clock for SXGA
		break;
	default : 	// 24MHz
		SetCAMClockDivider(CAMCLK24000000); //Set Camera Clock 24MHz s5x532, ov7620
		break;
	}


#if (USED_CAM_TYPE==CAM_OV7620)	//capbily
	i = Test_OV9650();
	if( i )
	{
		Uart_Printf("\nTest is failed!!!\n");
		return ;
	}
#else
	// Initializing camera module
	CamModuleReset(); // s5x532 must do this..
	Delay(100); // ready time of s5x433, s5x532 IIC interface. needed...
	CameraModuleSetting();
#endif

	Uart_Printf("Initializing end...\n");

	Test_CamPreview() ;

	Uart_Printf("\nCamera Preview Test End\n");

//	CamModuleReset(); // s5x532 must do this..

	rCLKCON &= ~(1<<19); // disable camclk
	Lcd_Tft_NEC35_Init() ;		//

}


void CamPortSet(void)
{
	save_GPJCON = rGPJCON;
	save_GPJDAT = rGPJDAT;
	save_GPJUP = rGPJUP;

	rGPJCON = 0x2aaaaaa;
	rGPJDAT = 0;
	rGPJUP = 0;

	//--- capbily
	rGPGCON &= ~(3<<24);
	rGPGCON |= 1<<24;
	rGPGUP  |= 1<<12;
#if (USED_CAM_TYPE==CAM_OV7620)
	rGPGDAT &= ~(1<<12);
#else
	rGPGDAT |= 1<<12;
#endif
	//---
}

void CamPortReturn(void)
{
	rGPJCON = save_GPJCON;
	rGPJDAT = save_GPJDAT;
	rGPJUP = save_GPJUP;
}

void CamPreviewIntUnmask(void)
{
    rINTSUBMSK &= ~(BIT_SUB_CAM_P);//INT CAMERA Port A ENABLE
    rINTMSK &= ~(BIT_CAM);
}

void CamCodecIntUnmask(void)
{
    rINTSUBMSK &= ~(BIT_SUB_CAM_C);//INT CAMERA Port B ENABLE
    rINTMSK &= ~(BIT_CAM);
}

void CamPreviewIntMask(void)
{
    rINTSUBMSK |= BIT_SUB_CAM_P;//INT CAMERA Port A ENABLE
    rINTMSK |= (BIT_CAM);
}

void CamCodecIntMask(void)
{
    rINTSUBMSK |= BIT_SUB_CAM_C;//INT CAMERA Port B ENABLE
    rINTMSK |= (BIT_CAM);
}


/******************************************************
 *                                                                      							*
 *                       camera interface initialization                    		*
 *                                                                            					*
 *******************************************************/

void CamReset(void)
{
	rCIGCTRL |= (1UL<<31); //camera I/F soft reset
	Delay(10);
	rCIGCTRL &= ~(1UL<<31);
}

void CamModuleReset(void)
{
	switch(USED_CAM_TYPE)
	{
	case CAM_OV7620 : // reset - active high
	//case CAM_S5X532 : // reset - active low, but H/W inverted.. so, in this case active high, masked by capbily
	case CAM_S5X433 : // reset - active low, but H/W inverted.. so, in this case active high
	case CAM_S5X3A1 : // reset - active low, but H/W inverted.. so, in this case active high
		rCIGCTRL |= (1<<30);	  //external camera reset high
		Delay(30);
		rCIGCTRL &= ~(1<<30);	//external camera reset low
		break;
	case CAM_S5X532 : // reset - active low, move here by capbily
	case CAM_AU70H : // reset - active low
	default :
		rCIGCTRL &= ~(1<<30);	//external camera reset low
		Delay(10);
		rCIGCTRL |= (1<<30); //external camera reset high
		break;
	}
}

// 0:48MHz, 1:24MHz, 2:16MHz, 3:12MHz...
// Camera clock = UPLL/[(CAMCLK_DIV+1)X2]
void SetCAMClockDivider(int divn)
{
	rCAMDIVN = (rCAMDIVN & ~(0xf))|(1<<4)|(divn); // CAMCLK is divided..
}

/* Description of Parameters
CoDstWidth: Destination Width of Codec Path
CoDstHeight: Destination Height of Codec Path
PrDstWidth: Destination Width of Preview Path
PrDstHeight: Destination Height of Preview Path
WinHorOffset: Size of Window Offset for Horizontal Direction
WinVerOffset: Size of Window Offset for Vertical Direction
CoFrameBuffer: Start Address for Codec DMA
PrFrameBuffer: Start Address for Previe DMA
*/
void CamInit(ULONG CoDstWidth, ULONG CoDstHeight, ULONG PrDstWidth, ULONG PrDstHeight,
			ULONG WinHorOffset, ULONG WinVerOffset,  ULONG CoFrameBuffer, ULONG PrFrameBuffer)
{
	ULONG WinOfsEn;
	ULONG divisor, multiplier;
	ULONG MainBurstSizeY, RemainedBurstSizeY, MainBurstSizeC, RemainedBurstSizeC, MainBurstSizeRGB, RemainedBurstSizeRGB;
	ULONG H_Shift, V_Shift, PreHorRatio, PreVerRatio, MainHorRatio, MainVerRatio;
	ULONG SrcWidth, SrcHeight;
	ULONG ScaleUp_H_Co, ScaleUp_V_Co, ScaleUp_H_Pr, ScaleUp_V_Pr;

	//constant for calculating codec dma address
	if(CAM_CODEC_OUTPUT)
		divisor=2; //CCIR-422
	else
		divisor=4; //CCIR-420

	//constant for calculating preview dma address
	if(CAM_PVIEW_OUTPUT)
		multiplier=4;
	else
		multiplier=2;

	if(WinHorOffset==0 && WinVerOffset==0)
		WinOfsEn=0;
	else
		WinOfsEn=1;

	SrcWidth=CAM_SRC_HSIZE-WinHorOffset*2;
	SrcHeight=CAM_SRC_VSIZE-WinVerOffset*2;

	if(SrcWidth>=CoDstWidth) ScaleUp_H_Co=0; //down
	else ScaleUp_H_Co=1;		//up

	if(SrcHeight>=CoDstHeight) ScaleUp_V_Co=0;
	else ScaleUp_V_Co=1;

	if(SrcWidth>=PrDstWidth) ScaleUp_H_Pr=0; //down
	else ScaleUp_H_Pr=1;		//up

	if(SrcHeight>=PrDstHeight) ScaleUp_V_Pr=0;   // edited 040225
	else ScaleUp_V_Pr=1;

	////////////////// common control setting
	rCIGCTRL |= (1<<26)|(0<<27); // inverse PCLK, test pattern
	//--- capbily
	//rCIGCTRL |= (0<<26)|(0<<27); // don't inverse PCLK, test pattern
	//---
	rCIWDOFST = (1<<30)|(0xf<<12); // clear overflow
	rCIWDOFST = 0;
	rCIWDOFST=(WinOfsEn<<31)|(WinHorOffset<<16)|(WinVerOffset);
	rCISRCFMT=(CAM_ITU601<<31)|(0<<30)|(0<<29)|(CAM_SRC_HSIZE<<16)|(CAM_ORDER_YCBYCR<<14)|(CAM_SRC_VSIZE);
	//--- capbily
	//rCISRCFMT=(CAM_ITU601<<31)|(1<<30)|(0<<29)|(CAM_SRC_HSIZE<<16)|(CAM_ORDER_YCBYCR<<14)|(CAM_SRC_VSIZE);
	//---

	////////////////// codec port setting
	if (CAM_CODEC_4PP)
	{
		rCICOYSA1=CoFrameBuffer;
		rCICOYSA2=rCICOYSA1+CoDstWidth*CoDstHeight+2*CoDstWidth*CoDstHeight/divisor;
		rCICOYSA3=rCICOYSA2+CoDstWidth*CoDstHeight+2*CoDstWidth*CoDstHeight/divisor;
		rCICOYSA4=rCICOYSA3+CoDstWidth*CoDstHeight+2*CoDstWidth*CoDstHeight/divisor;

		rCICOCBSA1=rCICOYSA1+CoDstWidth*CoDstHeight;
		rCICOCBSA2=rCICOYSA2+CoDstWidth*CoDstHeight;
		rCICOCBSA3=rCICOYSA3+CoDstWidth*CoDstHeight;
		rCICOCBSA4=rCICOYSA4+CoDstWidth*CoDstHeight;

		rCICOCRSA1=rCICOCBSA1+CoDstWidth*CoDstHeight/divisor;
		rCICOCRSA2=rCICOCBSA2+CoDstWidth*CoDstHeight/divisor;
		rCICOCRSA3=rCICOCBSA3+CoDstWidth*CoDstHeight/divisor;
		rCICOCRSA4=rCICOCBSA4+CoDstWidth*CoDstHeight/divisor;
	}
	else
	{
		rCICOYSA1=CoFrameBuffer;
		rCICOYSA2=rCICOYSA1;
		rCICOYSA3=rCICOYSA1;
		rCICOYSA4=rCICOYSA1;

		rCICOCBSA1=rCICOYSA1+CoDstWidth*CoDstHeight;
		rCICOCBSA2=rCICOCBSA1;
		rCICOCBSA3=rCICOCBSA1;
		rCICOCBSA4=rCICOCBSA1;

		rCICOCRSA1=rCICOCBSA1+CoDstWidth*CoDstHeight/divisor;
		rCICOCRSA2=rCICOCRSA1;
		rCICOCRSA3=rCICOCRSA1;
		rCICOCRSA4=rCICOCRSA1;
	}
	rCICOTRGFMT=(CAM_CODEC_IN_422<<31)|(CAM_CODEC_OUTPUT<<30)|(CoDstWidth<<16)|(CAM_FLIP_180<<14)|(CoDstHeight);

	CalculateBurstSize(CoDstWidth, &MainBurstSizeY, &RemainedBurstSizeY);
	CalculateBurstSize(CoDstWidth/2, &MainBurstSizeC, &RemainedBurstSizeC);
	rCICOCTRL=(MainBurstSizeY<<19)|(RemainedBurstSizeY<<14)|(MainBurstSizeC<<9)|(RemainedBurstSizeC<<4);

	CalculatePrescalerRatioShift(SrcWidth, CoDstWidth, &PreHorRatio, &H_Shift);
	CalculatePrescalerRatioShift(SrcHeight, CoDstHeight, &PreVerRatio, &V_Shift);
	MainHorRatio=(SrcWidth<<8)/(CoDstWidth<<H_Shift);
	MainVerRatio=(SrcHeight<<8)/(CoDstHeight<<V_Shift);

	rCICOSCPRERATIO=((10-H_Shift-V_Shift)<<28)|(PreHorRatio<<16)|(PreVerRatio);
	rCICOSCPREDST=((SrcWidth/PreHorRatio)<<16)|(SrcHeight/PreVerRatio);
	rCICOSCCTRL=(CAM_SCALER_BYPASS_OFF<<31)|(ScaleUp_H_Co<<30)|(ScaleUp_V_Co<<29)|(MainHorRatio<<16)|(MainVerRatio);

	rCICOTAREA=CoDstWidth*CoDstHeight;

	///////////////// preview port setting
	if (CAM_PVIEW_4PP) // codec view mode
	{
		rCIPRCLRSA1=PrFrameBuffer;
		rCIPRCLRSA2=rCIPRCLRSA1+PrDstWidth*PrDstHeight*multiplier;
		rCIPRCLRSA3=rCIPRCLRSA2+PrDstWidth*PrDstHeight*multiplier;
		rCIPRCLRSA4=rCIPRCLRSA3+PrDstWidth*PrDstHeight*multiplier;
	}
	else // direct preview mode
	{
		rCIPRCLRSA1 = (ULONG)LCD_BUFFER_CAM;
		rCIPRCLRSA2 = (ULONG)LCD_BUFFER_CAM;
		rCIPRCLRSA3 = (ULONG)LCD_BUFFER_CAM;
		rCIPRCLRSA4 = (ULONG)LCD_BUFFER_CAM;
	}

	rCIPRTRGFMT=(PrDstWidth<<16)|(CAM_FLIP_180<<14)|(PrDstHeight);

	if (CAM_PVIEW_OUTPUT==CAM_RGB24B)
		CalculateBurstSize(PrDstWidth*2, &MainBurstSizeRGB, &RemainedBurstSizeRGB);
	else // RGB16B
		CalculateBurstSize(PrDstWidth*2, &MainBurstSizeRGB, &RemainedBurstSizeRGB);
   	rCIPRCTRL=(MainBurstSizeRGB<<19)|(RemainedBurstSizeRGB<<14);

	CalculatePrescalerRatioShift(SrcWidth, PrDstWidth, &PreHorRatio, &H_Shift);
	CalculatePrescalerRatioShift(SrcHeight, PrDstHeight, &PreVerRatio, &V_Shift);
	MainHorRatio=(SrcWidth<<8)/(PrDstWidth<<H_Shift);
	MainVerRatio=(SrcHeight<<8)/(PrDstHeight<<V_Shift);
	rCIPRSCPRERATIO=((10-H_Shift-V_Shift)<<28)|(PreHorRatio<<16)|(PreVerRatio);
	rCIPRSCPREDST=((SrcWidth/PreHorRatio)<<16)|(SrcHeight/PreVerRatio);
	rCIPRSCCTRL=(1UL<<31)|(CAM_PVIEW_OUTPUT<<30)|(ScaleUp_H_Pr<<29)|(ScaleUp_V_Pr<<28)|(MainHorRatio<<16)|(MainVerRatio);

	rCIPRTAREA= PrDstWidth*PrDstHeight;
}



/********************************************************
 CalculateBurstSize - Calculate the busrt lengths

 Description:
 - dstHSize: the number of the byte of H Size.

*/
void CalculateBurstSize(ULONG hSize,ULONG *mainBurstSize,ULONG *remainedBurstSize)
{
	ULONG tmp;
	tmp=(hSize/4)%16;
	switch(tmp) {
		case 0:
			*mainBurstSize=16;
			*remainedBurstSize=16;
			break;
		case 4:
			*mainBurstSize=16;
			*remainedBurstSize=4;
			break;
		case 8:
			*mainBurstSize=16;
			*remainedBurstSize=8;
			break;
		default:
			tmp=(hSize/4)%8;
			switch(tmp) {
				case 0:
					*mainBurstSize=8;
					*remainedBurstSize=8;
					break;
				case 4:
					*mainBurstSize=8;
					*remainedBurstSize=4;
				default:
					*mainBurstSize=4;
					tmp=(hSize/4)%4;
					*remainedBurstSize= (tmp) ? tmp: 4;
					break;
			}
			break;
	}
}



/********************************************************
 CalculatePrescalerRatioShift - none

 Description:
 - none

*/
void CalculatePrescalerRatioShift(ULONG SrcSize, ULONG DstSize, ULONG *ratio,ULONG *shift)
{
	if(SrcSize>=64*DstSize) {
		Uart_Printf("ERROR: out of the prescaler range: SrcSize/DstSize = %d(< 64)\n",SrcSize/DstSize);
		while(1);
	}
	else if(SrcSize>=32*DstSize) {
		*ratio=32;
		*shift=5;
	}
	else if(SrcSize>=16*DstSize) {
		*ratio=16;
		*shift=4;
	}
	else if(SrcSize>=8*DstSize) {
		*ratio=8;
		*shift=3;
	}
	else if(SrcSize>=4*DstSize) {
		*ratio=4;
		*shift=2;
	}
	else if(SrcSize>=2*DstSize) {
		*ratio=2;
		*shift=1;
	}
	else {
		*ratio=1;
		*shift=0;
	}

}


/********************************************************
 CamCaptureStart - Start camera capture operation.

 Description:
 - mode= CAM_CODEC_CAPTURE_ENABLE_BIT or CAM_PVIEW_CAPTURE_ENABLE_BIT or both

*/
void CamCaptureStart(ULONG mode)
{

	if(mode&CAM_CODEC_SCALER_CAPTURE_ENABLE_BIT) {
		camCodecStatus=CAM_STARTED;
		rCICOSCCTRL|=CAM_CODEC_SACLER_START_BIT;
	}

	if(mode&CAM_PVIEW_SCALER_CAPTURE_ENABLE_BIT) {
		camPviewStatus=CAM_STARTED;
		rCIPRSCCTRL|=CAM_PVIEW_SACLER_START_BIT;
	}

	if(mode&CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT) {
		camCodecStatus=CAM_STARTED;
		rCICOSCCTRL|=CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT;
	}

	rCIIMGCPT|=CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT|mode;
}


void CamCaptureStop(void)
{
	camCodecStatus=CAM_STOP_ISSUED;
	camPviewStatus=CAM_STOP_ISSUED;
}


void _CamCodecStopHw(void)
{
	rCICOSCCTRL &= ~CAM_CODEC_SACLER_START_BIT; //stop codec scaler.
	rCIIMGCPT &= ~CAM_CODEC_SCALER_CAPTURE_ENABLE_BIT; //stop capturing for codec scaler.
	if(!(rCIIMGCPT & CAM_PVIEW_SCALER_CAPTURE_ENABLE_BIT))
		rCIIMGCPT &= ~CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT; //stop capturing for preview scaler if needed.
	rCICOCTRL |= (1<<2); //Enable last IRQ at the end of frame capture.
		       //NOTE:LastIrqEn bit should be set after clearing CAPTURE_ENABLE_BIT & SCALER_START_BIT
}

void _CamPviewStopHw(void)
{
	rCIPRSCCTRL &= ~CAM_PVIEW_SACLER_START_BIT; //stop preview scaler.
	rCIIMGCPT &= ~CAM_PVIEW_SCALER_CAPTURE_ENABLE_BIT; //stop capturing for preview scaler.
	if(!(rCIIMGCPT&CAM_CODEC_SCALER_CAPTURE_ENABLE_BIT))
		rCIIMGCPT &= ~CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT; //stop capturing for codec scaler if needed.
	rCIPRCTRL |= (1<<2); //Enable last IRQ at the end of frame capture.
       	//NOTE:LastIrqEn bit should be set after clearing CAPTURE_ENABLE_BIT & SCALER_START_BIT
}


void __irq CamIsr(void)
{

	ULONG completedFrameIndex;

	if (rSUBSRCPND&BIT_SUB_CAM_C)
	{
		//Uart_Printf("[C]");
		rGPFDAT ^= 1<<5;	//capbily
		CamCodecIntMask();
		rSUBSRCPND |= BIT_SUB_CAM_C;
		ClearPending(BIT_CAM);
		switch(camCodecStatus) {
			case CAM_STOP_ISSUED:
				_CamCodecStopHw();
				camCodecStatus=CAM_LAST_CAPTURING;
				Uart_Printf("cr=%x\n", rCICOCTRL);
				//Uart_Printf("cS1\n");
				break;
			case CAM_LAST_CAPTURING:
				camCodecStatus=CAM_STOPPED;
				CamCodecIntMask();
				//Uart_Printf("cS2\n");
				return;
			case CAM_STARTED:
				flagCaptured_C = 1;
//				_CamCodecStopHw();
				if(camTestMode&CAM_TEST_MODE_CODEC)	{
					if(camCodecCaptureCount>0)
						completedFrameIndex=(((rCICOSTATUS>>26)&0x3)+4-2)%4;
					//Uart_Printf("FrameIndex:%d\n",completedFrameIndex);
				}
				else {
					//Uart_Printf("Just Capturing without display");
				}
				break;
			case CAM_CODEC_SCALER_BYPASS_STATE:
				//Uart_Printf("cBP\n");
				break;
			default:
				break;
		}

		CamCodecIntUnmask();
	    camCodecCaptureCount++;
	}
	else
	{
		//Uart_Printf("[P]");
		rGPFDAT ^= 1<<4;	//capbily
		CamPreviewIntMask();
		rSUBSRCPND |= BIT_SUB_CAM_P;
		ClearPending(BIT_CAM) ;
		switch(camPviewStatus) {
			case CAM_STOP_ISSUED:
				_CamPviewStopHw();
				camPviewStatus=CAM_LAST_CAPTURING;
				Uart_Printf("pr=%x\n", rCIPRCTRL);
				//Uart_Printf("pS1\n");
				break;
			case CAM_LAST_CAPTURING:
				camPviewStatus=CAM_STOPPED;
				CamPreviewIntMask();
				//Uart_Printf("pS2\n");
				return;
			case CAM_STARTED:
				flagCaptured_P = 1;
				if(camTestMode&CAM_TEST_MODE_PVIEW) {
					if(camPviewCaptureCount >0)
						completedFrameIndex=(((rCIPRSTATUS>>26)&0x3)+4-2)%4;
					//Uart_Printf("FrameIndex:%d\n",completedFrameIndex);
				}
				else {
					//Uart_Printf("Preview Image Captured\n");
				}
			default:
				break;
		}

		CamPreviewIntUnmask();
		camPviewCaptureCount++;
	}
}



/******************************************************************************
 *                                                                            *
 *                   camera interface interrupts & controls                   *
 *                                                                            *
 ******************************************************************************/


ULONG Conv_YCbCr_Rgb(UCHAR y0, UCHAR y1, UCHAR cb0, UCHAR cr0)  // second solution... by junon
{
	// bit order is
	// YCbCr = [Cr0 Y1 Cb0 Y0], RGB=[R1,G1,B1,R0,G0,B0].

	int r0, g0, b0, r1, g1, b1;
	ULONG rgb0, rgb1, rgb;

	#if 1 // 4 frames/s @192MHz, 12MHz ; 6 frames/s @450MHz, 12MHz
	r0 = YCbCrtoR(y0, cb0, cr0);
	g0 = YCbCrtoG(y0, cb0, cr0);
	b0 = YCbCrtoB(y0, cb0, cr0);
	r1 = YCbCrtoR(y1, cb0, cr0);
	g1 = YCbCrtoG(y1, cb0, cr0);
	b1 = YCbCrtoB(y1, cb0, cr0);
	#endif

	if (r0>255 ) r0 = 255;
	if (r0<0) r0 = 0;
	if (g0>255 ) g0 = 255;
	if (g0<0) g0 = 0;
	if (b0>255 ) b0 = 255;
	if (b0<0) b0 = 0;

	if (r1>255 ) r1 = 255;
	if (r1<0) r1 = 0;
	if (g1>255 ) g1 = 255;
	if (g1<0) g1 = 0;
	if (b1>255 ) b1 = 255;
	if (b1<0) b1 = 0;

	// 5:6:5 16bit format
	rgb0 = (((USHORT)r0>>3)<<11) | (((USHORT)g0>>2)<<5) | (((USHORT)b0>>3)<<0);	//RGB565.
	rgb1 = (((USHORT)r1>>3)<<11) | (((USHORT)g1>>2)<<5) | (((USHORT)b1>>3)<<0);	//RGB565.

	rgb = (rgb1<<16) | rgb0;

	return(rgb);
}


void Display_Cam_Image(ULONG size_x, ULONG size_y)
{
	UCHAR *buffer_y, *buffer_cb, *buffer_cr;
	//UCHAR y0,y1,cb0,cr0;
	//int r0,r1,g0,g1,b0,b1;
	ULONG rgb_data0;// rgb_data1;
	ULONG x, y;
	int temp;

	if (CAM_CODEC_4PP)
		temp = (((rCICOSTATUS>>26)&0x3)+4-2)%4; // current frame memory block
	else
		temp = 4;
	//Uart_Printf("Current Frame memory %d\n", temp);

	switch (temp) // current frame mem - 2
	{
	case 0:
		buffer_y = (UCHAR *)rCICOYSA1;
		buffer_cb = (UCHAR *)rCICOCBSA1;
		buffer_cr = (UCHAR *)rCICOCRSA1;
		break;
	case 1:
		buffer_y = (UCHAR *)rCICOYSA2;
		buffer_cb = (UCHAR *)rCICOCBSA2;
		buffer_cr = (UCHAR *)rCICOCRSA2;
		break;
	case 2:
		buffer_y = (UCHAR *)rCICOYSA3;
		buffer_cb = (UCHAR *)rCICOCBSA3;
		buffer_cr = (UCHAR *)rCICOCRSA3;
		break;
	case 3:
		buffer_y = (UCHAR *)rCICOYSA4;
		buffer_cb = (UCHAR *)rCICOCBSA4;
		buffer_cr = (UCHAR *)rCICOCRSA4;
		break;
	default :
		buffer_y = (UCHAR *)rCICOYSA1;
		buffer_cb = (UCHAR *)rCICOCBSA1;
		buffer_cr = (UCHAR *)rCICOCRSA1;
		break;
	}

	//Uart_Printf("End setting : Y-0x%x, Cb-0x%x, Cr-0x%x\n", buffer_y, buffer_cb, buffer_cr);
#if 0
	// for checking converting time
	rGPGCON = (rGPGCON&~(1<<23))|(1<<22); //EINT19 -> GPG11 output
	rGPGUP |= (1<<11);
	rGPGDAT &= ~(1<<11);
	Delay(90);
	rGPGDAT |=(1<<11);

	rgb_data0 = 0;
#endif

#if CAM_CODEC_OUTPUT==CAM_CCIR420
	for (y=0;y<size_y;y++) // YCbCr 4:2:0 format
	{
		for (x=0;x<size_x;x+=2)
		{
			rgb_data0 = Conv_YCbCr_Rgb(*buffer_y++, *buffer_y++, *buffer_cb++, *buffer_cr++);
			frameBuffer16BitTft240320[y][x/2] = rgb_data0;

			if ( (x==(size_x-2)) && ((y%2)==0) ) // when x is last pixel & y is even number
			{
				buffer_cb -= size_x/2;
				buffer_cr -= size_x/2;
			}
		}
	}
#else
	for (y=0;y<size_y;y++) // YCbCr 4:2:2 format
	{
		for (x=0;x<size_x;x+=2)
		{
			rgb_data0 = Conv_YCbCr_Rgb(*buffer_y++, *buffer_y++, *buffer_cb++, *buffer_cr++);
			//frameBuffer16BitTft240320[y][x/2] = rgb_data0;

		//--- capbily
			//frameBuffer16BitTft240320是32位的数组!!!
			//in HWSWP=1 16BPP mode d0~d15 is 1st pixel, d16~d31 is 2nd
			//in HWSWP=0 16BPP mode d0~d15 is 2nd pixel, d16~d31 is 1st

		//	USHORT lt;
			//Uart_Printf("%x,%x\n", *buffer_cb, *buffer_cr);
			//测试只输出Y(亮度)
		//	lt = (*buffer_y++)>>3;
		//	rgb_data0  = ((lt<<11)|(lt<<5)|(lt))<<16;	//1st pixel
		//	lt = (*buffer_y++)>>3;
		//	rgb_data0 |= (lt<<11)|(lt<<5)|(lt);		//2nd pixel
			//测试只输出U
		//	lt   = (*buffer_cb)>>3;
		//	rgb_data0  = ((lt<<11)|(lt<<5)|(lt))<<16;	//1st pixel
		//	lt   = (*buffer_cb++)>>3;
		//	rgb_data0 |= (lt<<11)|(lt<<5)|(lt);		//2nd pixel
			//测试只输出V
		//	lt   = (*buffer_cr)>>3;
		//	rgb_data0  = ((lt<<11)|(lt<<5)|(lt))<<16;	//1st pixel
		//	lt   = (*buffer_cr++)>>3;
		//	rgb_data0 |= (lt<<11)|(lt<<5)|(lt);		//2nd pixel

			//测试ov9650固定输出U=0x1f,V=0x70的模式
		//	rgb_data0  = *buffer_y++<<24;	//1st pixel
		//	rgb_data0 |= *buffer_cb++<<16;
		//	rgb_data0 |= *buffer_y++<<8;	//2nd pixel
		//	rgb_data0 |= *buffer_cr++;
		//	rgb_data0 &= ~0xff00f800;
		//	rgb_data0 |= 0x0700;

			//测试ov9650输出RGB565格式
		//	rgb_data0  = *buffer_y++<<24;	//1st pixel
		//	rgb_data0 |= *buffer_cb++<<16;
		//	rgb_data0 |= *buffer_y++<<8;	//2nd pixel
		//	rgb_data0 |= *buffer_cr++;
			//RGB高低交换
		//	rgb_data0  = *buffer_cb++<<24;	//1st pixel
		//	rgb_data0 |= *buffer_y++<<16;
		//	rgb_data0 |= *buffer_cr++<<8;	//2nd pixel
		//	rgb_data0 |= *buffer_y++;
			//	rgb_data0 &= 0xf800f800;	//red   only
			//	rgb_data0 &= 0x07e007e0;	//green only
			//	rgb_data0 &= 0x001f001f;	//blue  only

			LCD_BUFFER_CAM[y][x/2] = rgb_data0;
			//Uart_Printf("%08x\n", rgb_data0);
		//---
		}
	}
#endif

//	memcpy((UCHAR*)0x30100000,  frameBuffer16BitTft240320, 320*240*2); // QCIF=178*144*2
#if 0
	rGPGDAT &= ~(1<<11);
	Delay(30);
	rGPGDAT |=(1<<11);
	rGPGCON = (rGPGCON&~(1<<22))|(1<<23); // GPG11 output -> EINT19
#endif
}

#if 0
void Display_Cam_Image(int offset_x, int offset_y, int size_x, int size_y)
{
	UCHAR* CamFrameAddr, LcdFrameAddr;
	int i, temp;

	Lcd_MoveViewPort(offset_x, offset_y, USED_LCD_TYPE);

	switch(camPviewCaptureCount%4)
	{
	case 2 :
		temp = rCIPRCLRSA1;
		break;
	case 3 :
		temp = rCIPRCLRSA2;
		break;
	case 0 :
		temp = rCIPRCLRSA3;
		break;
	case 1 :
	default:
		temp = rCIPRCLRSA4;
		break;
	}

	*CamFrameAddr = temp;
	*LcdFrameAddr = LCD_BUFFER_CAM;
	for(i=0;i<size_y;i++)
	{
		memcpy(LcdFrameAddr, CamFrameAddr, size_x*2);
		*LcdFrameAddr += size_x*4; // added virtual screen
		*CamFrameAddr += size_x*2;
	}
}
#endif

void Test_CamPreview(void)
{

	//UCHAR flag;
	//ULONG i,j,k,value;

	Uart_Printf("\nNow Start Camera Preview\n");

	//camera global variables
	camTestMode=CAM_TEST_MODE_PVIEW;
	camCodecCaptureCount=0;
	camPviewCaptureCount=0;
	camPviewStatus=CAM_STOPPED;
	camCodecStatus=CAM_STOPPED;
	flagCaptured_P=0;

	LCD_Init() ;
#if LCD_TYPE==LCD_TYPE_N35
	//Paint_Bmp(0, 0, 240, 320, flower1_240_320);
	Paint_Bmp(0, 0, 240, 320, camera_pic_240x320);
#endif

	Uart_Printf( "preview sc control = %x\n" , rCIPRSCCTRL ) ;

	// Initialize Camera interface
	switch(USED_CAM_TYPE)
	{
	case CAM_S5X532 : // defualt for test : data-falling edge, ITU601, YCbCr
		CamInit(640, 480, 240, 320, 112, 20,  CAM_FRAMEBUFFER_C, CAM_FRAMEBUFFER_P);
		break;
	case CAM_S5X3A1 : // defualt for test : data-falling edge, YCbCr
		CamInit(640, 480, 240, 320, 120, 100,	CAM_FRAMEBUFFER_C, CAM_FRAMEBUFFER_P);
		rCISRCFMT = rCISRCFMT & ~(1UL<<31); // ITU656
//		rCIGCTRL &= ~(1<<26); // inverse PCLK, test pattern
		break;
	//--- add by capbily
	case CAM_OV7620:
#if LCD_TYPE==LCD_TYPE_N35
		CamInit(240, 180, 240, 180, 100, 100,  CAM_FRAMEBUFFER_C, CAM_FRAMEBUFFER_P);
#elif LCD_TYPE==LCD_TYPE_A70
		CamInit(800, 480, 800, 480, 0, 0,  CAM_FRAMEBUFFER_C, CAM_FRAMEBUFFER_P);
#endif
		break;
	//---
	default :
		CamInit(640, 480, 320, 240, 0, 0,  CAM_FRAMEBUFFER_C, CAM_FRAMEBUFFER_P);
		break;
	}
	Uart_Printf("preview sc control = %x\n", rCIPRSCCTRL);


	// Start Capture
	rSUBSRCPND |= BIT_SUB_CAM_C|BIT_SUB_CAM_P;
	ClearPending(BIT_CAM);
	pISR_CAM = (ULONG)CamIsr;
	CamPreviewIntUnmask();
	CamCaptureStart(CAM_PVIEW_SCALER_CAPTURE_ENABLE_BIT);
	Uart_Printf("Press 'ESC' key to exit!\n");
	while (1)
	{
		if (flagCaptured_P)
		{
			flagCaptured_P = 0;
//			Uart_Printf("Enter Cam A port, count = %d\n",camCodecCaptureCount);
		}
		if ( Uart_GetKey() == ESC_KEY ) break;
	}

	CamCaptureStop();

	Uart_Printf("\nWait until the current frame capture is completed.\n");
	while(camPviewStatus!=CAM_STOPPED)
		if (Uart_GetKey()== '\r') break;

	Uart_Printf("CIS format = %x\n", rCISRCFMT);
	Uart_Printf("image cap = %x\n", rCIIMGCPT);
	Uart_Printf("preview sc control = %x\n", rCIPRSCCTRL);
	Uart_Printf("preview control = %x\n", rCIPRCTRL);
	Uart_Printf("codec sc control = %x\n", rCICOSCCTRL);
	Uart_Printf("codec control = %x\n", rCICOCTRL);
	Uart_Printf("pr addr1 = %x\n", rCIPRCLRSA1);
	Uart_Printf("pr addr2 = %x\n", rCIPRCLRSA2);

	Uart_Printf("camCodecCaptureCount=%d\n",camCodecCaptureCount);
	Uart_Printf("camPviewCaptureCount=%d\n",camPviewCaptureCount);
//	CamPreviewIntMask();
}

