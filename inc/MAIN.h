/****************************************************************
 NAME: MAIN.h
 DESC:
 HISTORY:
 Mar.29.2002:purnnamu: created first
 ****************************************************************/

#ifndef __MAIN_H__
#define __MAIN_H__

/* ϵͳʱ�Ӻ� */
#define  FCLK_200M   0
#define  FCLK_300M   1
#define  FCLK_400M   2


extern volatile UCHAR *downPt;
extern volatile ULONG totalDmaCount;
extern volatile ULONG downloadFileSize;
extern volatile ULONG downloadAddress;
extern volatile USHORT checkSum;

extern int download_run;
extern ULONG tempDownloadAddress;

typedef struct {
	void (*fun)(void);
	const char *tip;
}CMD_TIPS_ST;

extern char Image$$RW$$Limit[];
extern char Image$$RO$$Limit[];
extern char Image$$RO$$Base[];
extern char Image$$RW$$Limit[];
extern char Image$$RW$$Base[];
extern char Image$$ZI$$Limit[];
extern char Image$$ZI$$Base[];



#endif /*__MAIN_H__*/
