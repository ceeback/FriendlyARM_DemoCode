#ifndef	DMA_ADMIN_H
#define	DMA_ADMIN_H

#define	MAX_DMA_CHANNEL	4
//#define	DMA_IS_USED	1
#define	DMA_IS_HWTRIG	2
#define	DMA_IS_SWTRIG	1
#define	DMA_IS_FREE		0

// request source
#define	REQ_XDREQ0	0
#define	REQ_UART0	1
#define	REQ_SDI		2
#define	REQ_TIMER	3
#define	REQ_USB_EP1	4
#define	REQ_XDREQ1	0x10
#define	REQ_UART1	0x11
//#define	REQ_IISDI	0x12
#define	REQ_SPI		0x13
#define	REQ_USB_EP2	0x14
#define	REQ_IISDO	0x20
#define	REQ_IISDI	0x21
//#define	REQ_SDI		0x22
//#define	REQ_TIMER	0x23
#define	REQ_USB_EP3	0x24
#define	REQ_UART2	0x30
//#define	REQ_SDI		0x31
//#define	REQ_SPI		0x32
//#define	REQ_TIMER	0x33
#define	REQ_USB_EP4	0x34

// DISRCC, DIDSTC parameters
#define	SRC_LOC_APB			0x200
#define	SRC_LOC_AHB			0
#define	SRC_ADDR_FIXED		0x100
#define	SRC_ADDR_INC		0
#define	DST_LOC_APB			0x2000
#define	DST_LOC_AHB			0
#define	DST_ADDR_FIXED		0x1000
#define	DST_ADDR_INC		0
// DCON paramaters
#define	HANDSHAKE_MODE	0x80000000	//[31]
#define	DEMAND_MODE		0x00000000
#define	SYNC_AHB		0x40000000	//[30]
#define	SYNC_APB		0x00000000
#define	DONE_GEN_INT	0x20000000	//[29]
#define	DONE_NO_INT		0x00000000
#define	TSZ_BURST		0x10000000	//[28]
#define	TSZ_UNIT		0x00000000
#define	WHOLE_SVC		0x08000000	//[27]
#define	SINGLE_SVC		0x00000000
#define	HW_TRIG			0x00800000	//[23]
#define	SW_TRIG			0x00000000
#define	RELOAD_OFF		0x00400000	//[22]
#define	RELOAD_ON		0x00000000
#define	DSZ_8b			0x00000000
#define	DSZ_16b			0x00100000
#define	DSZ_32b			0x00200000	//[21:20]

///////////////////////////////////////////////////////
#define	DMA_START			0x8000
#define	REQUEST_DMA_FAIL	0x1000

/*
ULONG RequestDMAChannel(USHORT ch, USHORT DevID);
ULONG QueryDMAChannel(USHORT ch);
void ReleaseDMAChannel(USHORT ch);
*/

ULONG RequestDMASW(ULONG attr, ULONG mode);
ULONG RequestDMA(ULONG attr, ULONG mode);
USHORT ReleaseDMA(ULONG attr);
USHORT StartDMA(ULONG attr);
USHORT StopDMA(ULONG attr);
USHORT SetDMARun(ULONG attr, ULONG src_addr, ULONG dst_addr, ULONG len);
ULONG QueryDMAStat(ULONG attr);
ULONG QueryDMASrc(ULONG attr);
ULONG QueryDMADst(ULONG attr);

#endif	/* DMA_ADMIN_H	*/