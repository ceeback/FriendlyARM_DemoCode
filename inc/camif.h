/******************************************************************************

 Filename:	camif.h

 Descriptions
 - header file of camif.c

 History
 - July 23, 2003. Draft Version 0.0 by purnnamu
 - Janualy 15, 2004. Modifed by Boaz

 Copyright (c) 2003 SAMSUNG Electronics.
 # However, Anybody can use this code without our permission.

 ******************************************************************************/


#ifndef __CAMIF_H__
#define __CAMIF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "camdef.h"


#if 0 //If you have to use
extern volatile ULONG camTestMode;
extern volatile ULONG camCodecCaptureCount;
extern volatile ULONG camPviewCaptureCount;
extern volatile ULONG camPviewStatus;
extern volatile ULONG camCodecStatus;
#endif


void CalculateBurstSize(ULONG dstHSize,ULONG *mainBurstSize,ULONG *RemainedBurstSize);
void CalculatePrescalerRatioShift(ULONG srcWidth, ULONG dstWidth, ULONG *ratio,ULONG *shift);

void __irq CamPviewIsr(void);
void __irq CamCodecIsr(void);
void __irq CamIsr(void);


void CamPortSet(void);
void CamPortReturn(void);
void CamPreviewIntUnmask(void);
void CamCodecIntUnmask(void);
void CamPreviewIntMask(void);
void CamCodecIntMask(void);
void CamReset(void);
void CamModuleReset(void);
void SetCAMClockDivider(int divn);

void CamCaptureStart(ULONG mode);
void CamCaptureStop(void);
void _CamPviewStopHw(void);
void _CamCodecStopHw(void);


void Test_CamPreview(void);
void Test_CamCodec(void);
void Test_YCbCr_to_RGB(void);
void Camera_Test(void);


#ifdef __cplusplus
}
#endif

#endif /*__CAMIF_H__*/
