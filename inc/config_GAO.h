//�ļ���:config_GAO.h
//����ʱ��:2009-10-03
#ifndef  config_GAO
#define config_GAO

#define BAUDRATE       115200
#define SELECTED_UART  0

// GPB1 ΪLCD ģ��ı�������ź�.
// GPB1/TOUT1 for Backlight control(PWM)
#define GPB1_TO_OUT()   (rGPBUP &= 0xfffd, rGPBCON &= 0xfffffff3, rGPBCON |= 0x00000004)
#define GPB1_TO_1()     (rGPBDAT |= 0x0002)
#define GPB1_TO_0()     (rGPBDAT &= 0xfffd)

#endif  // config_GAO.h