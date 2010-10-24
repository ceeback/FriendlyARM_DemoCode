/*********************************************
  NAME: memtest.c
  DESC: test SDRAM of SMDK2440 b/d
  HISTORY:
  03.27.2002:purnnamu: first release
 *********************************************/

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "mmu.h"


void MemoryTest(void)
{
    int i;
    ULONG data;
    int memError=0;
    ULONG *pt;

    //
    // memory test
    //
    //Uart_Printf("Memory Test(%xh-%xh):WR",_RAM_STARTADDRESS,(_ISR_STARTADDRESS&0xfff0000));
    //test sdram from _RAM_STARTADDRESS+2M, hzh
	Uart_Printf("Memory Test(%xh-%xh):WR",_RAM_STARTADDRESS+0x00200000,(_ISR_STARTADDRESS&0xffff0000));

    //pt=(ULONG *)_RAM_STARTADDRESS;
    pt=(ULONG *)(_RAM_STARTADDRESS+0x00200000);	//hzh
    while((ULONG)pt<(_ISR_STARTADDRESS&0xffff0000))
    {
	*pt=(ULONG)pt;
	pt++;
    }

    Uart_Printf("\b\bRD");
    //pt=(ULONG *)_RAM_STARTADDRESS;
    pt=(ULONG *)(_RAM_STARTADDRESS+0x00200000);	//hzh

    while((ULONG)pt<(_ISR_STARTADDRESS&0xffff0000))
    {
	data=*pt;
	if(data!=(ULONG)pt)
	{
	    memError=1;
	    Uart_Printf("\b\bFAIL:0x%x=0x%x\n",i,data);
	    break;
	}
	pt++;
    }

    if(memError==0)Uart_Printf("\b\bO.K.\n");
}

