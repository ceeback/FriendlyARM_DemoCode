//===================================================================
// File Name : 2440lib.h
// Function  : S3C2440
// Date      : February 26, 2002
// Version   : 0.0
// History
//  0.0 :Feb.20.2002:SOP     : Programming start
//  0.01:Mar.29.2002:purnnamu: For POWEROFF_wake_up, the START... label is added
//===================================================================

#ifndef __2440lib_h__
#define __2440lib_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "def.h"

#define DebugOut Uart_Printf

#define min(x1,x2) (((x1)<(x2))? (x1):(x2))
#define max(x1,x2) (((x1)>(x2))? (x1):(x2))

#define ONESEC0 (62500)	             //16us resolution, max 1.04 sec
#define ONESEC1 (31250)	             //32us resolution, max 2.09 sec
#define ONESEC2 (15625)	             //64us resolution, max 4.19 sec
#define ONESEC3 (7812)	             //128us resolution, max 8.38 sec
#define ONESEC4 (PCLK/128/(0xff+1))  //@60Mhz, 128*4us resolution, max 32.53 sec

#define NULL 0

#define EnterPWDN(clkcon) ((void (*)(int))0x20)(clkcon)
void StartPointAfterPowerOffWakeUp(void); //purnnamu:Mar.29.2002


// 2440lib.c
void Delay(int time);              //Watchdog Timer is used.

void *malloc(unsigned nbyte);
void free(void *pt);

void Port_Init(void);
void Uart_Select(int ch);
void Uart_TxEmpty(int ch);
void Uart_Init(int mclk,int baud);
char Uart_Getch(void);
char Uart_GetKey(void);
void Uart_GetString(char *string);
int  Uart_GetIntNum(void);
int Uart_GetIntNum_GJ(void) ;
void Uart_SendByte(int data);
void Uart_Printf(char *fmt,...);
void Uart_SendString(char *pt);

void Timer_Start(int divider);    //Watchdog Timer is used.
int  Timer_Stop(void);            //Watchdog Timer is used.

void LcdBkLtSet(ULONG HiRatio) ;		//lcd backlight
void LCD_BackLight_Control( void ) ;

void Led_Display(int data);
void Beep(ULONG freq, ULONG ms) ;
void BUZZER_PWM_Test( void );
void ChangeMPllValue(int m,int p,int s);
void ChangeClockDivider(int hdivn_val,int pdivn_val);
void ChangeUPllValue(int m,int p,int s);
//void Test_Iic(void);
//void EnterCritical(int *i);
void Test_IrDA_Tx(void);

void outportw(USHORT, ULONG);
USHORT inportw(ULONG);
//void ExitCritical(int *i);

#ifdef __cplusplus
}
#endif

#endif  //__2440lib_h__
