/******************************************************************************

 Filename:	camproset.c

 Descriptions
 - Camera Processor Initialization code using SCCB.
 - SCCB(Serial Camera Contol Bus) H/W dependent code.
 - Camera Processor Initialization code using IIC.

 History
  - July 23, 2003. Draft Version 0.0 by purnnamu
  - Janualy 15, 2004. Modifed by Boaz
  - Feb. 10, 2004. Modified by junon for S3C2440A


 Copyright (c) 2003 SAMSUNG Electronics.
 # However, Anybody can use this code without our permission.

 ******************************************************************************/
#include "def.h"
#include "2440addr.h"
#include "2440lib.h"

#include "camproset.h"
#include "camdef.h"
#include "camdata.h"


#define CAMWRDATA      (1)
#define CAMRDDATA      (3)
#define CAMSETRDADDR   (4)

#define CAMIICBUFSIZE 0x20


#if USED_CAM_TYPE==CAM_OV7620
#define CAM_ID		(0x42)
#define CAM_NAME  "OV7620"
#elif USED_CAM_TYPE==CAM_S5X433
#define CAM_ID		(0x42)
#define CAM_NAME  "S5X433"
#elif USED_CAM_TYPE==CAM_S5X532
#define CAM_ID		(0x5a)
#define CAM_NAME  "S5X532"
#elif USED_CAM_TYPE==CAM_S5X3A1
#define CAM_ID		(0x5a)
#define CAM_NAME  "S5X3A1"
#elif USED_CAM_TYPE==CAM_AU70H
#define CAM_ID		(0x22)
#define CAM_NAME  "AU70H"
#endif


static UCHAR _CAMiicData[CAMIICBUFSIZE];
static volatile int _CAMiicDataCount;
//static volatile int _CAMiicStatus;
static volatile int _CAMiicMode;
static int _CAMiicPt;

volatile ULONG save_GPECON;


void * func_cammodule_test[][2]=
{
	//IIC
	(void *)Camera_WriteByte, 	"Write 1 byte data into Camera module register",
	(void *)Camera_ReadByte,  	"Read 1 byte data from Camera module register",
	(void *)Camera_WriteBlock,  	"Write a block of byte data into Camera module register",
	(void *)Camera_ReadBlock,  	"Read a block of byte data from Camera module register",
	0,0
};


void Camera_Iic_Test(void)
{
	int i;
	//int camclk;

	CameraModuleSetting();

	Uart_Printf("\n***** IIC Master Tx/Rx Test Program with %s Camera Module *****\n", CAM_NAME);

	while(1) 	{
		i=0;
		Uart_Printf("\n\n");

		while(1) {   //display menu
			Uart_Printf("%2d:%s\n", i, func_cammodule_test[i][1]);
			i++;
			if((int)(func_cammodule_test[i][0])==0) {
				Uart_Printf("\n");
				break;
			}
			if((i%4)==0)
			Uart_Printf("\n");
		}

		Uart_Printf("\nPress Enter key to exit : ");
		i = Uart_GetIntNum();

		if(i==-1) break;		// return.
		if(i>=0 && (i<((sizeof(func_cammodule_test)-1)/8)) )	// select and execute...
			( (void (*)(void)) (func_cammodule_test[i][0]) )();
	}

	Uart_Printf("\n");
   	rINTMSK |= BIT_IIC;
}


void IicPortSet(void)
{
	save_GPECON = rGPECON;
	rGPECON = rGPECON & ~(0xfUL<<28) | (0xaUL<<28);	//GPE15:IICSDA , GPE14:IICSCL
}

void IicPortReturn(void)
{
	rGPECON = save_GPECON;
}

void Wr_CamIIC(ULONG slvAddr, ULONG addr, UCHAR data)
{
	_CAMiicMode      = CAMWRDATA;
	_CAMiicPt        = 0;
	_CAMiicData[0]   = (UCHAR)addr;
	_CAMiicData[1]   = data;
	_CAMiicDataCount = 2;

	rIICDS        = slvAddr;            //0x5a: S5X532 Slave ID
	rIICSTAT      = 0xf0; //Start Master TX Condition
	rIICCON &= ~(1<<4);   //Clearing the pending bit isn't needed because the pending bit has been cleared.

	while(_CAMiicDataCount!=-1);
}

void Rd_CamIIC(ULONG slvAddr,ULONG addr,UCHAR *data)
{

	/*IIC Slave Addr Write + IIC Reg Addr Write */
	_CAMiicMode      = CAMSETRDADDR;
	_CAMiicPt        = 0;
	_CAMiicData[0]   = (UCHAR)addr;
	_CAMiicDataCount = 1;

	rIICDS   = slvAddr;
	rIICSTAT = 0xf0;   //Master Tx, Start
	rIICCON &= ~(1<<4);                    //Resumes IIC operation.
	//Clearing the pending bit isn't needed because the pending bit has been cleared.
	while(_CAMiicDataCount!=-1);

	_CAMiicMode      = CAMRDDATA;
	_CAMiicPt        = 0;
	_CAMiicDataCount = 1;

	rIICDS   = slvAddr;
	rIICSTAT = 0xb0;                    //Master Rx,Start
	rIICCON &= ~(1<<4);                   //Resumes IIC operation.

	while(_CAMiicDataCount!=-1);

	*data = _CAMiicData[1];
}


void __irq Cam_IICInt(void)
{
	ULONG iicSt,i;

	ClearPending(BIT_IIC);
	iicSt   = rIICSTAT;
	rINTMSK |= BIT_IIC;

	if(iicSt & 0x8){}           //When bus arbitration is failed.
	if(iicSt & 0x4){}           //When a slave address is matched with IICADD
	if(iicSt & 0x2){}           //When a slave address is 0000000b
	if(iicSt & 0x1){}           //When ACK isn't received

	switch(_CAMiicMode) {
		case CAMRDDATA:
			if((_CAMiicDataCount--)==0) {
				_CAMiicData[_CAMiicPt++] = rIICDS;

				rIICSTAT = 0x90;      //Stop MasRx condition
				rIICCON  &= ~(1<<4);      //Resumes IIC operation.
				Delay(1);                 //Wait until stop condtion is in effect., Too long time...  # need the time 2440:Delay(1), 24A0: Delay(2)
								//The pending bit will not be set after issuing stop condition.
				break;
			}
			_CAMiicData[_CAMiicPt++] = rIICDS;     //The last data has to be read with no ack.

			if((_CAMiicDataCount)==0)
				rIICCON &= ~((1<<7)|(1<<4));      //Resumes IIC operation with NOACK in case of S5X532 Cameara
//				rIICCON = 0x6f;			//Resumes IIC operation with NOACK in case of S5X532 Cameara
			else
				rIICCON &= ~(1<<4);      //Resumes IIC operation with ACK
			break;
		case CAMWRDATA:
			if((_CAMiicDataCount--)==0) {
				rIICSTAT = 0xd0;                //stop MasTx condition
				rIICCON  &= ~(1<<4);                //resumes IIC operation.
				Delay(1);                       //wait until stop condtion is in effect. # need the time 2440:Delay(1), 24A0: Delay(2)
					//The pending bit will not be set after issuing stop condition.
				break;
			}
			rIICDS = _CAMiicData[_CAMiicPt++];        //_iicData[0] has dummy.
			for(i=0;i<10;i++);                  //for setup time until rising edge of IICSCL
				rIICCON &= ~(1<<4);                     //resumes IIC operation.
			break;
		case CAMSETRDADDR: //Uart_Printf("[S%d]",_iicDataCount);
			if((_CAMiicDataCount--)==0) {
				rIICSTAT = 0xd0;                //stop MasTx condition
				rIICCON  &= ~(1<<4);                //resumes IIC operation.
				Delay(1);      //wait until stop condtion is in effect.

				break;                  //IIC operation is stopped because of IICCON[4]
			}
			rIICDS = _CAMiicData[_CAMiicPt++];
			for(i=0;i<10;i++);          //for setup time until rising edge of IICSCL
				rIICCON &= ~(1<<4);             //resumes IIC operation.
			break;
		default:
			break;
	}

	rINTMSK &= ~BIT_IIC;
}


void Camera_WriteByte(void)
{
	ULONG RegData,RegAddr;
	//ULONG i, j,save_E,save_PE,pageNo;

#if (USED_CAM_TYPE==CAM_S5X532)||(USED_CAM_TYPE==CAM_S5X3A1)
	Uart_Printf("Input Write Page No of %s\n=>", CAM_NAME);
	pageNo = (UCHAR)Uart_GetIntNum();
#endif

	Uart_Printf("Input Write Register Address of %s\n=>", CAM_NAME);
	RegAddr = (UCHAR)Uart_GetIntNum();

	Uart_Printf("Input Write Transfer Data into %s\n=>", CAM_NAME);
	RegData = (UCHAR)Uart_GetIntNum();

#if (USED_CAM_TYPE==CAM_S5X532)||(USED_CAM_TYPE==CAM_S5X3A1)
	Wr_CamIIC(CAM_ID, (UCHAR)0xec, pageNo);  // set Page no
#endif
	Wr_CamIIC(CAM_ID, (UCHAR)RegAddr, RegData); // set register after setting page number
}


void Camera_ReadByte(void)
{
	ULONG RegAddr;
	// ULONG pageNo, RegData, save_PE, save_E, i, j;
	static UCHAR rdata[8];

#if (USED_CAM_TYPE==CAM_S5X532)||(USED_CAM_TYPE==CAM_S5X3A1)
	Uart_Printf("Input Write Page No of %s\n=>", CAM_NAME);
	pageNo = (UCHAR)Uart_GetIntNum();
	Wr_CamIIC(CAM_ID, (UCHAR)0xec, pageNo);  // set Page no
#endif

	Uart_Printf("Input Read Register Address of %s\n=>", CAM_NAME);
	RegAddr = (UCHAR)Uart_GetIntNum();
	Rd_CamIIC(CAM_ID, RegAddr, &rdata[0]);
	Uart_Printf("Register Addr: 0x%2x, data: 0x%2x\n", RegAddr,rdata[0]);
}


void Camera_WriteBlock(void)
{
	ULONG i;
	//ULONG i, j, save_E, save_PE, RegAddr, RegData;
	//static UCHAR rdata[256];

#if USED_CAM_TYPE==CAM_OV7620
	for(i=0; i<(sizeof(Ov7620_YCbCr8bit)/2); i++)
		Wr_CamIIC(CAM_ID, Ov7620_YCbCr8bit[i][0], Ov7620_YCbCr8bit[i][1]);
#elif USED_CAM_TYPE==CAM_S5X532
	for(i=0; i<(sizeof(S5X532_YCbCr8bit)/2); i++)
		Wr_CamIIC(CAM_ID, S5X532_YCbCr8bit[i][0], S5X532_YCbCr8bit[i][1]);
#elif USED_CAM_TYPE==CAM_S5X3A1
		for(i=0; i<(sizeof(S5X3A1_YCbCr8bit)/2); i++)
			Wr_CamIIC(CAM_ID, S5X3A1_YCbCr8bit[i][0], S5X3A1_YCbCr8bit[i][1]);
#elif USED_CAM_TYPE==CAM_AU70H
	for(i=0; i<(sizeof(Au70h)/2); i++)
		Wr_CamIIC(CAM_ID, Au70h[i][0], Au70h[i][1]);
#endif

   Uart_Printf("\nBlock TX Ended...\n");
}


void Camera_ReadBlock(void)
{
	ULONG i;
	//ULONG i, j, save_E, save_PE, RegAddr, RegData;
	static UCHAR rdata[256];

#if USED_CAM_TYPE==CAM_OV7620
	for(i=0; i<(sizeof(Ov7620_YCbCr8bit)/2);i++)
		Rd_CamIIC(CAM_ID, Ov7620_YCbCr8bit[i][0], &rdata[i]);
	for(i=0; i<(sizeof(Ov7620_YCbCr8bit)/2);i++)
		Uart_Printf("Addr: 0x%2x, W: 0x%2x, R: 0x%2x\n", Ov7620_YCbCr8bit[i][0], Ov7620_YCbCr8bit[i][1], rdata[i]);
#elif USED_CAM_TYPE==CAM_S5X532
    for(i=0; i<(sizeof(S5X532_YCbCr8bit)/2);i++)
   	{
		if(S5X532_YCbCr8bit[i][0] == 0xec)
			Wr_CamIIC(CAM_ID, S5X532_YCbCr8bit[i][0], S5X532_YCbCr8bit[i][1]);
		else
			Rd_CamIIC(CAM_ID, S5X532_YCbCr8bit[i][0], &rdata[i]);
    }
  	for(i=0; i<(sizeof(S5X532_YCbCr8bit)/2);i++)
	{
	  	if(S5X532_YCbCr8bit[i][0] == 0xec)
	       Uart_Printf("Page: 0x%2x\n",  S5X532_YCbCr8bit[i][1]);
		else
	  		Uart_Printf("Addr: 0x%2x, W: 0x%2x, R: 0x%2x\n", S5X532_YCbCr8bit[i][0], S5X532_YCbCr8bit[i][1], rdata[i]);
  	}
#elif USED_CAM_TYPE==CAM_S5X3A1
	for(i=0; i<(sizeof(S5X3A1_YCbCr8bit)/2);i++)
	{
	if(S5X3A1_YCbCr8bit[i][0] == 0xec)
		Wr_CamIIC(CAM_ID, S5X3A1_YCbCr8bit[i][0], S5X3A1_YCbCr8bit[i][1]);
	else
		Rd_CamIIC(CAM_ID, S5X3A1_YCbCr8bit[i][0], &rdata[i]);
	}
	for(i=0; i<(sizeof(S5X3A1_YCbCr8bit)/2);i++)
{
		if(S5X3A1_YCbCr8bit[i][0] == 0xec)
			 Uart_Printf("Page: 0x%2x\n",  S5X3A1_YCbCr8bit[i][1]);
	else
			Uart_Printf("Addr: 0x%2x, W: 0x%2x, R: 0x%2x\n", S5X3A1_YCbCr8bit[i][0], S5X3A1_YCbCr8bit[i][1], rdata[i]);
	}
#elif USED_CAM_TYPE==CAM_AU70H
	for(i=0; i<(sizeof(Au70h)/2);i++)
		Rd_CamIIC(CAM_ID, Au70h[i][0], &rdata[i]);
	for(i=0; i<(sizeof(Au70h)/2);i++)
		Uart_Printf("Addr: 0x%2x, W: 0x%2x, R: 0x%2x\n", Au70h[i][0], Au70h[i][1], rdata[i]);
#endif

}


int CameraModuleSetting(void)
{
	//ULONG i, j, save_E, save_PE, RegAddr, RegData;
	//static UCHAR rdata[256];
	int donestatus;

	IicPortSet();

	// for camera init, added by junon
	pISR_IIC = (unsigned)Cam_IICInt;
	rINTMSK &= ~(BIT_IIC);

	//Enable ACK, Prescaler IICCLK=PCLK/512, Enable interrupt, Transmit clock value Tx clock=IICCLK/16
	rIICCON  = (1<<7) | (1<<6) | (1<<5) | (0x3);

	rIICADD  = 0x10;                    //24A0 slave address = [7:1]
	rIICSTAT = 0x10;                    //IIC bus data output enable(Rx/Tx)
    rIICLC = (1<<2)|(3);   			// SDAOUT has 5clock cycle delay

	donestatus=1;

	Camera_WriteBlock();
//	Camera_ReadBlock();

	return donestatus;
}


