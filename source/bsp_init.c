#include "usr_include.h"

void bsp_init(void)
{
    BOARD_ConfigMPU();        /* 初始化内存保护单元*/
    BOARD_InitBootPins();     /* 初始化引脚*/
    BOARD_BootClockRUN();     /* 初始化时钟配置*/
    BOARD_InitDebugConsole(); /* 调试接口初始化*/
    vusr_enetInitPort();      /* 初始化网络端口*/
    vusr_elcdifInit();        /* 初始化 LCD 屏幕*/
    vusr_qtmrInit();          /* 开启定时器*/
	iusr_semcInit();		  /* 初始化 SDRAM */
}