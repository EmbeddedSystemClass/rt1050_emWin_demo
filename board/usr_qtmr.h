#ifndef  _USR_QTMR_H__
#define  _USR_QTMR_H__

/* Get source clock for QTMR driver */
#define QTMR_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_IpgClk)

#define QTMR_IRQ_ID TMR3_IRQn
#define QTMR_IRQ_HANDLER TMR3_IRQHandler
#define USR_QTMR_BASEADDR TMR3
#define USR_FIRST_QTMR_CHANNEL kQTMR_Channel_0
#define USR_SECOND_QTMR_CHANNEL kQTMR_Channel_1
#define QTMR_ClockCounterOutput kQTMR_ClockCounter0Output

void vusr_qtmrInit(void);
#endif
