//===================================================================
// File Name : 2440slib.h
// Function  : S3C2440 
// Date      : February 20, 2002
// Version   : 0.0
// History
//   0.0 : Programming start (February 20,2002) -> SOP
//===================================================================

#ifndef __2440slib_h__
#define __2440slib_h__

#ifdef __cplusplus
extern "C" {
#endif

int SET_IF(void);
void WR_IF(int cpsrValue);
void CLR_IF(void);
void EnterCritical(ULONG *pSave);
void ExitCritical(ULONG *pSave);
void MMU_EnableICache(void);
void MMU_DisableICache(void);
void MMU_EnableDCache(void);
void MMU_DisableDCache(void);
void MMU_EnableAlignFault(void);
void MMU_DisableAlignFault(void);
void MMU_EnableMMU(void);
void MMU_DisableMMU(void);
void MMU_SetTTBase(ULONG base);
void MMU_SetDomain(ULONG domain);

void MMU_SetFastBusMode(void);  //GCLK=HCLK
void MMU_SetAsyncBusMode(void); //GCLK=FCLK @(FCLK>=HCLK)

void MMU_InvalidateIDCache(void);
void MMU_InvalidateICache(void);
void MMU_InvalidateICacheMVA(ULONG mva);
void MMU_PrefetchICacheMVA(ULONG mva);
void MMU_InvalidateDCache(void);
void MMU_InvalidateDCacheMVA(ULONG mva);
void MMU_CleanDCacheMVA(ULONG mva);
void MMU_CleanInvalidateDCacheMVA(ULONG mva);
void MMU_CleanDCacheIndex(ULONG index);
void MMU_CleanInvalidateDCacheIndex(ULONG index);	
void MMU_WaitForInterrupt(void);
	
void MMU_InvalidateTLB(void);
void MMU_InvalidateITLB(void);
void MMU_InvalidateITLBMVA(ULONG mva);
void MMU_InvalidateDTLB(void);
void MMU_InvalidateDTLBMVA(ULONG mva);

void MMU_SetDCacheLockdownBase(ULONG base);
void MMU_SetICacheLockdownBase(ULONG base);

void MMU_SetDTLBLockdown(ULONG baseVictim);
void MMU_SetITLBLockdown(ULONG baseVictim);

void MMU_SetProcessId(ULONG pid);

#ifdef __cplusplus
}
#endif

#endif   //__2440slib_h__