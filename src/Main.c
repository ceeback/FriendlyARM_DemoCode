/****************************************************************
 NAME: u2440mon.c
 DESC: u2440mon entry point,menu,download
 HISTORY:
 Mar.25.2002:purnnamu: S3C2400X profile.c is ported for S3C2410X.
 Mar.27.2002:purnnamu: DMA is enabled.
 Apr.01.2002:purnnamu: isDownloadReady flag is added.
 Apr.10.2002:purnnamu: - Selecting menu is available in the waiting loop.
                         So, isDownloadReady flag gets not needed
                       - UART ch.1 can be selected for the console.
 Aug.20.2002:purnnamu: revision number change 0.2 -> R1.1
 Sep.03.2002:purnnamu: To remove the power noise in the USB signal, the unused CLKOUT0,1 is disabled.
 ****************************************************************/
#define GLOBAL_CLK      1

#include <stdlib.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "mmu.h"
#include "profile.h"
#include "memtest.h"
#include "config_GAO.h"
#include "MAIN.h"

void Isr_Init(void);
void HaltUndef(void);
void HaltSwi(void);
void HaltPabort(void);
void HaltDabort(void);
void ClearMemory(void);
void Temp_function(void);

void Clk0_Enable(int clock_sel);
void Clk1_Enable(int clock_sel);
void Clk0_Disable(void);
void Clk1_Disable(void);

extern void Lcd_Tft_NEC35_Init(void);
extern void Lcd_Tft_NEC35_Test( void ) ;
extern void Test_Touchpanel(void) ;
extern void Test_Adc(void) ;        //adc test
extern void KeyScan_Test(void) ;
extern void RTC_Display(void) ;
extern void Test_IrDA_Tx(void) ;
extern void PlayMusicTest(void) ;
extern void RecordTest( void ) ;
extern void Test_Iic(void) ;
extern void Test_SDI(void) ;
extern void Camera_Test( void ) ;



ULONG FCLK;
ULONG HCLK;
ULONG PCLK;
ULONG UCLK;

volatile ULONG downloadAddress;
void (*restart)(void)=(void (*)(void))0x0;
volatile UCHAR *downPt;
volatile ULONG downloadFileSize;
volatile USHORT checkSum;
volatile ULONG err=0;
volatile ULONG totalDmaCount;
volatile int isUsbdSetConfiguration;
int download_run=0;
ULONG tempDownloadAddress;
int menuUsed=0;
ULONG *pMagicNum=(ULONG *)Image$$RW$$Limit;
static ULONG cpu_freq;
static ULONG UPLL;

CMD_TIPS_ST CmdTip[] = {
                { Temp_function,        "Please input 1-16 to select test" } ,
                { BUZZER_PWM_Test,      "Test PWM" } ,
                { RTC_Display,          "RTC time display" } ,
                { Test_Adc,             "Test ADC" } ,
                { KeyScan_Test,         "Test interrupt and key scan" } ,
                { Test_Touchpanel,      "Test Touchpanel" } ,
                { Lcd_Tft_NEC35_Test,   "Test NEC 3.5\" LCD" } ,
                { Test_Iic,             "Test IIC EEPROM, if use QQ2440, please remove the LCD" } ,
                { PlayMusicTest,        "UDA1341 play music" } ,
                { RecordTest,           "UDA1341 record voice" } ,
                { Test_SDI,             "Test SD Card" } ,
                { Camera_Test,          "Test CMOS Camera"},
                { 0, 0}
            };


static void cal_cpu_bus_clk(void)
{
    ULONG val;
    UCHAR m, p, s;

    val = rMPLLCON;
    m = (val>>12)&0xff;
    p = (val>>4)&0x3f;
    s = val&3;

    //(m+8)*FIN*2 不要超出32位数!
    FCLK = ((m+8)*(FIN/100)*2)/((p+2)*(1<<s))*100;

    val = rCLKDIVN;
    m = (val>>1)&3;
    p = val&1;
    val = rCAMDIVN;
    s = val>>8;

    switch (m) {
    case 0:
        HCLK = FCLK;
        break;
    case 1:
        HCLK = FCLK>>1;
        break;
    case 2:
        if(s&2)
            HCLK = FCLK>>3;
        else
            HCLK = FCLK>>2;
        break;
    case 3:
        if(s&1)
            HCLK = FCLK/6;
        else
            HCLK = FCLK/3;
        break;
    }

    if(p)
        PCLK = HCLK>>1;
    else
        PCLK = HCLK;

    if(s&0x10)
        cpu_freq = HCLK;
    else
        cpu_freq = FCLK;

    val = rUPLLCON;
    m = (val>>12)&0xff;
    p = (val>>4)&0x3f;
    s = val&3;
    UPLL = ((m+8)*FIN)/((p+2)*(1<<s));
    UCLK = (rCLKDIVN&8)?(UPLL>>1) : UPLL;
}


void Temp_function()
{
    Uart_Printf("\nPlease input 1-16 to select test!!!\n");
}

// 设置系统主时钟
/* 外部晶振频率为12MHz */ 
void SetSysClock(ULONG ulFclk)
{
    ULONG key; /* 分频因数 */
    int mdiv, pdiv, sdiv;
    
    switch(ulFclk)
    {
        case FCLK_200M:
            mdiv = 92;
            pdiv = 4;
            sdiv = 1;
            key  = 12;
            break;
        case FCLK_300M:
            mdiv = 67;
            pdiv = 1;
            sdiv = 1;
            key  = 13;
            break;   
        case FCLK_400M:
        default:
            mdiv = 92;
            pdiv = 1;
            sdiv = 1;
            key  = 14;
            break;
    }

    ChangeMPllValue(mdiv, pdiv, sdiv);
    ChangeClockDivider(key, 12); //固定设置HCLK为100M,PCLK为50M.
    
}

void Main(void)
{
    char *mode;
    int i;

    ULONG mpll_val = 0 ;
    //ULONG divn_upll = 0 ;

    Port_Init();
    Isr_Init();

    SetSysClock(FCLK_400M);
    cal_cpu_bus_clk();

    Uart_Init( 0,BAUDRATE );
    Uart_Select( 0 );

    Beep(2000, 100);


    Uart_Printf("\n<***********************************************>\n"
                   "           SBC2440 Test Program VER1.0\n"
                   "                www.arm9.net\n"
                   "      Build time is: %s  %s\n", __DATE__ , __TIME__ );
    Uart_Printf( "          Image$$RO$$Base  = 0x%x\n", Image$$RO$$Base );
    Uart_Printf( "          Image$$RO$$Limit = 0x%x\n", Image$$RO$$Limit );
    Uart_Printf( "          Image$$RW$$Base  = 0x%x\n", Image$$RW$$Base );
    Uart_Printf( "          Image$$RW$$Limit = 0x%x\n", Image$$RW$$Limit );
    Uart_Printf( "          Image$$ZI$$Base  = 0x%x\n", Image$$ZI$$Base );
    Uart_Printf( "          Image$$ZI$$Limit = 0x%x\n", Image$$ZI$$Limit );
    Uart_Printf( "          cal_cpu_bus_clk  = %dHz\n", cpu_freq );
    Uart_Printf("<***********************************************>\n");

    rMISCCR=rMISCCR&~(1<<3); // USBD is selected instead of USBH1
    rMISCCR=rMISCCR&~(1<<13); // USB port 1 is enabled.


//
//  USBD should be initialized first of all.
//
//  isUsbdSetConfiguration=0;

//  rd_dm9000_id();         //
//  rGPBCON &= ~(3<<20);    //CF_CARD Power
//  rGPBCON |= 1<<20;
//  rGPBDAT |= 1<<10;
//  rDSC0 = 0x155;
//  rDSC1 = 0x15555555;
    rDSC0 = 0x2aa;
    rDSC1 = 0x2aaaaaaa;
    //Enable NAND, USBD, PWM TImer, UART0,1 and GPIO clock,
    //the others must be enabled in OS!!!
    rCLKCON = 0xfffff0;



    //MMU_EnableICache();
        MMU_Init(); //
        //Uart_Printf("NOR Flash ID is 0x%08x\n", GetFlashID());

    pISR_SWI=(_ISR_STARTADDRESS+0xf0);  //for pSOS

    Led_Display(0x66);

#if USBDMA
    mode="DMA";
#else
    mode="Int";
#endif

    // CLKOUT0/1 select.
    //Uart_Printf("CLKOUT0:MPLL in, CLKOUT1:RTC clock.\n");
    //Clk0_Enable(0);   // 0:MPLLin, 1:UPLL, 2:FCLK, 3:HCLK, 4:PCLK, 5:DCLK0
    //Clk1_Enable(2);   // 0:MPLLout, 1:UPLL, 2:RTC, 3:HCLK, 4:PCLK, 5:DCLK1
    Clk0_Disable();
    Clk1_Disable();

    mpll_val = rMPLLCON;

#if LCD_TYPE==LCD_TYPE_N35
    Lcd_Tft_NEC35_Init();
#elif LCD_TYPE==LCD_TYPE_A70
    Lcd_Tft_800x480_Init();
#elif LCD_TYPE==LCD_TYPE_VGA1024x768
    Lcd_VGA_1024x768_Init();
#endif


    download_run=1; //The default menu is the Download & Run mode.

    while(1)
    {
        UCHAR idx;

        Uart_Printf("\nPlease select function : \n");
        for(i=0; CmdTip[i].fun!=0; i++)
            Uart_Printf("%d : %s\n", i, CmdTip[i].tip);
        idx = Uart_GetIntNum_GJ() ;
        if(idx<i)
        {
            (*CmdTip[idx].fun)();
            Delay(20);
            Uart_Init( 0,BAUDRATE );
        }

    }

}

void Isr_Init(void)
{
    pISR_UNDEF=(unsigned)HaltUndef;
    pISR_SWI  =(unsigned)HaltSwi;
    pISR_PABORT=(unsigned)HaltPabort;
    pISR_DABORT=(unsigned)HaltDabort;
    rINTMOD=0x0;      // All=IRQ mode
    rINTMSK=BIT_ALLMSK;   // All interrupt is masked.

    //pISR_URXD0=(unsigned)Uart0_RxInt;
    //rINTMSK=~(BIT_URXD0);   //enable UART0 RX Default value=0xffffffff

//#if 1
//  pISR_USBD =(unsigned)IsrUsbd;
//  pISR_DMA2 =(unsigned)IsrDma2;
//#else
//  pISR_IRQ =(unsigned)IsrUsbd;
        //Why doesn't it receive the big file if use this. (???)
        //It always stops when 327680 bytes are received.
//#endif
//  ClearPending(BIT_DMA2);
//  ClearPending(BIT_USBD);
    //rINTMSK&=~(BIT_USBD);

    //pISR_FIQ,pISR_IRQ must be initialized
}


void HaltUndef(void)
{
    Uart_Printf("Undefined instruction exception!!!\n");
    while(1);
}

void HaltSwi(void)
{
    Uart_Printf("SWI exception!!!\n");
    while(1);
}

void HaltPabort(void)
{
    Uart_Printf("Pabort exception!!!\n");
    while(1);
}

void HaltDabort(void)
{
    Uart_Printf("Dabort exception!!!\n");
    while(1);
}


void ClearMemory(void)
{
    //int i;
    //ULONG data;
    int memError=0;
    ULONG *pt;

    Uart_Printf("Clear Memory (%xh-%xh):WR",_RAM_STARTADDRESS,HEAPEND);

    pt=(ULONG *)_RAM_STARTADDRESS;
    while((ULONG)pt < HEAPEND)
    {
        *pt=(ULONG)0x0;
        pt++;
    }

    if(memError==0)Uart_Printf("\b\bO.K.\n");
}

void Clk0_Enable(int clock_sel)
{   // 0:MPLLin, 1:UPLL, 2:FCLK, 3:HCLK, 4:PCLK, 5:DCLK0
    rMISCCR = rMISCCR&~(7<<4) | (clock_sel<<4);
    rGPHCON = rGPHCON&~(3<<18) | (2<<18);
}

void Clk1_Enable(int clock_sel)
{   // 0:MPLLout, 1:UPLL, 2:RTC, 3:HCLK, 4:PCLK, 5:DCLK1
    rMISCCR = rMISCCR&~(7<<8) | (clock_sel<<8);
    rGPHCON = rGPHCON&~(3<<20) | (2<<20);
}

void Clk0_Disable(void)
{
    rGPHCON = rGPHCON&~(3<<18); // GPH9 Input
}

void Clk1_Disable(void)
{
    rGPHCON = rGPHCON&~(3<<20); // GPH10 Input
}

