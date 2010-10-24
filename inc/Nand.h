#ifndef __NAND_H
#define __NAND_H

////////////////////////////// 8-bit ////////////////////////////////
// Main function
void Test_Nand(void);

// Sub function
void Test_K9S1208(void);
void NF8_Program(void);
void PrintSubMessage(void);
void Test_NF8_Rw(void);
void Test_NF8_Page_Write(void);
void Test_NF8_Page_Read(void);
void Test_NF8_Block_Erase(void);
void NF8_PrintBadBlockNum(void);
void Test_NF8_Lock(void);
void Test_NF8_SoftUnLock(void);

UCHAR Read_Status(void);

//*************** H/W dependent functions ***************
// Assembler code for speed
/*
void __RdPage512(UCHAR *pPage);
void Nand_Reset(void);
void InputTargetBlock(void);

void NF8_Print_Id(void);
static USHORT NF8_CheckId(void);
static int NF8_EraseBlock(ULONG blockNum);
static int NF8_ReadPage(ULONG block,ULONG page,UCHAR *buffer);
static int NF8_WritePage(ULONG block,ULONG page,UCHAR *buffer);
static int NF8_IsBadBlock(ULONG block);
static int NF8_MarkBadBlock(ULONG block);
static void NF8_Init(void);
*/
//*******************************************************



#endif /*__NAND_H*/

