#include "def.h"
#include "2440addr.h"
#include "2440lib.h"
#include "AudioDrv.h"
#include "WindowsXP_Wav.h"		//����Ϊ243552���ֽ�

#define	BUF_SIZE	(16*1024)

extern ULONG downloadAddress;
extern ULONG downloadFileSize;
//extern UCHAR USB_OR_UART_Download_OK ;

//static USHORT fsTable[] = {8000,11025,16000,22050,32000,44100,48000,0};

void PlayMusicTest(void)
{
	int size, i, j, err;
	WAVEFORMATEX fmt;
	WAVEHDR hdr[2048];
	HWAVEOUT hwo;
	UCHAR pause = 0;
	UCHAR mute = 0;
	ULONG volume;
	UCHAR *buf;

	downloadAddress = _NONCACHE_STARTADDRESS;
	buf = (UCHAR *)downloadAddress ;
	for( i = 0; i < 243552; i++ )  buf[i] = WindowsXP_Wav[i] ;
	downloadFileSize = 243552 ;

	size = *(ULONG *)(downloadAddress+0x28);
	i = 0;

	while(size>0)
	{
		hdr[i].lpData = (LPSTR)(downloadAddress+0x2c+i*BUF_SIZE);
		hdr[i].dwBufferLength = (size>BUF_SIZE)?BUF_SIZE:size;
		size -= BUF_SIZE;
		i++;
	}

	fmt.wFormatTag		= WAVE_FORMAT_PCM;
	fmt.nChannels		= *(USHORT *)(downloadAddress+0x16);
	fmt.nSamplesPerSec	= *(ULONG *)(downloadAddress+0x18);
	fmt.nAvgBytesPerSec	= *(ULONG *)(downloadAddress+0x1c);
	fmt.nBlockAlign		= *(USHORT *)(downloadAddress+0x20);
	fmt.wBitsPerSample	= *(USHORT *)(downloadAddress+0x22);
	Uart_Printf("\nSample Rate = %d, Channels = %d, %dBitsPerSample, size = %d\n",
			fmt.nSamplesPerSec, fmt.nChannels, fmt.wBitsPerSample, *(ULONG *)(downloadAddress+0x28));

	hwo = 0;
	err = waveOutOpen(&hwo,
				0,
				&fmt,
				0,
				0,
				0);
	Uart_Printf("\nerr = %x\n", err);
	for(j=0;j<i;j++)
		waveOutWrite(0,	&hdr[j], 0);

	Uart_Printf("Now playing the file\n");
	Uart_Printf("Press 'ESC' to quit, '+' to inc volume, '-' to dec volume, 'm' to mute, 'p' to pause\n");

	waveOutGetVolume(0,	&volume);
	while(1)
	{
		UCHAR key = Uart_Getch();
		if( key == ESC_KEY )
			break;
		if(key=='p')
		{
			pause ^= 1;
			if(pause&1)
				waveOutPause(0);
			else
				waveOutRestart(0);
		}
		if(key=='m')
		{
			mute ^= 1;
			if(mute&1)
				waveOutSetVolume(0, 0);
			else
				waveOutSetVolume(0, volume);
		}
		if((key=='+')&&(volume<=64535))
		{
			volume += 1000;
			waveOutSetVolume(0, volume);
		}
		if((key=='-')&&(volume>=1000))
		{
			volume -= 1000;
			waveOutSetVolume(0, volume);
		}

	}
	waveOutClose(0);
}

void RecordTest(void)
{
	int size, i, j, err;
	WAVEFORMATEX fmt;
	WAVEHDR hdr[2048];
	HWAVEIN hwi = 1;

	/*Uart_Printf("Please select Sample Rate:\n");
	for(i=0; fsTable[i]; i++)
		Uart_Printf("%d. %dHz\n", i, fsTable[i]);
	while(1)
	{
		UCHAR key = Uart_Getch();
		if((key-'0')>=0&&(key-'0'<=i-1))
		{
			fmt.nSamplesPerSec = fsTable[key-'0'];
			break;
		}
	}*/

	Uart_Printf( "\nThe Frequency of record is 48KHz\n" );
	fmt.nSamplesPerSec = 48000;		//����Ƶ��Ϊ48KHz
	fmt.wBitsPerSample = 16;

	fmt.wFormatTag		= WAVE_FORMAT_PCM;
	fmt.nChannels		= 2;
	fmt.nBlockAlign     = fmt.wBitsPerSample*fmt.nChannels/8;
	fmt.nAvgBytesPerSec	= fmt.nSamplesPerSec*fmt.nBlockAlign;

	downloadAddress  = 0x30800000;//_NONCACHE_STARTADDRESS;
	downloadFileSize = size = 16*1024*1024;
	i = 0;

	while(size>0)
	{
		hdr[i].lpData = (LPSTR)(downloadAddress+0x2c+i*BUF_SIZE);
		hdr[i].dwBufferLength = (size>BUF_SIZE)?BUF_SIZE:size;
		size -= BUF_SIZE;
		i++;
	}

	*(USHORT *)(downloadAddress+0x14) = fmt.wFormatTag;
	*(USHORT *)(downloadAddress+0x16) = fmt.nChannels;
	*(ULONG *)(downloadAddress+0x18) = fmt.nSamplesPerSec;
	*(ULONG *)(downloadAddress+0x1c) = fmt.nAvgBytesPerSec;
	*(USHORT *)(downloadAddress+0x20) = fmt.nBlockAlign;
	*(USHORT *)(downloadAddress+0x22) = fmt.wBitsPerSample;
	*(ULONG *)(downloadAddress+0x28) = downloadFileSize;

	err = waveInOpen(&hwi,
				0,
				&fmt,
				0,
				0,
				0);
	Uart_Printf("\nerr = %x\n", err);

	for(j=0;j<i;j++)
		if(waveInAddBuffer(hwi, &hdr[j], 0))
			Uart_Printf("Add buffer error!");

	Uart_Printf("Added %d buffer for record\n", i);
	Uart_Printf("Press any to Record\n");
	Uart_Getch();

	Uart_Printf("Now begin recording, Press 'ESC' to quit\n");
	waveInStart(hwi);

	while(1)
	{
		UCHAR key;

		key = Uart_GetKey();
		if( key == ESC_KEY )
			break;

//		Uart_Printf("%x,%x,%x,%x,%x,%x,%x\n", rDISRC2, rDISRCC2, rDIDST2, rDIDSTC2, rDCON2, rDSTAT2, rDMASKTRIG2);
//		Uart_Printf("%x,%x,%x,%x\n", rIISCON, rIISMOD, rIISPSR, rIISFCON);
	}
	waveInClose(hwi);
}

void RecordChannelOnOff(void)
{
	static int ChannelOn = 0;
	int err;
	WAVEFORMATEX fmt;
	WAVEHDR hdr;
	HWAVEIN hwi = 1;

	fmt.nSamplesPerSec  = 22050;
	fmt.wBitsPerSample  = 16;
	fmt.wFormatTag		= WAVE_FORMAT_PCM;
	fmt.nChannels		= 2;
	fmt.nBlockAlign     = fmt.wBitsPerSample*fmt.nChannels/8;
	fmt.nAvgBytesPerSec	= fmt.nSamplesPerSec*fmt.nBlockAlign;

	hdr.lpData = (LPSTR)0x30800000;//_NONCACHE_STARTADDRESS;
	hdr.dwBufferLength = BUF_SIZE;

	if(!ChannelOn) {
		err = waveInOpen(&hwi,
					0,
					&fmt,
					0,
					0,
					0);
		Uart_Printf("\nerr = %x\n", err);
		if(!err) {
			waveInAddBuffer(hwi, &hdr, 0);
			waveInStart(hwi);
			Uart_Printf("Record channel on\n");
			ChannelOn = 1;
		}
	} else {
		waveInClose(hwi);
		Uart_Printf("Record channel off\n");
		ChannelOn = 0;
	}
}
