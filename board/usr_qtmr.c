#include "usr_qtmr.h"
#include "fsl_qtmr.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "fsl_clock.h"

volatile uint32_t ulHighFrequencyTimerTicks;
extern volatile uint32_t g_eventTimeMilliseconds;
//uint32_t Qtemr_Count;

void vusr_qtmrInit(void)
{
    qtmr_config_t qtmrConfig;
    QTMR_GetDefaultConfig(&qtmrConfig);
    /* Use IP bus clock div by 128 */
    qtmrConfig.primarySource = kQTMR_ClockDivide_128;

    //Timer use-case, 50 millisecond tick
    QTMR_Init(USR_QTMR_BASEADDR, USR_SECOND_QTMR_CHANNEL, &qtmrConfig);

    /* Set timer period to be 50 millisecond */
    QTMR_SetTimerPeriod(USR_QTMR_BASEADDR, USR_SECOND_QTMR_CHANNEL, MSEC_TO_COUNT(1U, (QTMR_SOURCE_CLOCK / 128)));

    /* Enable at the NVIC */
    EnableIRQ(QTMR_IRQ_ID);

    /* Enable timer compare interrupt */
    QTMR_EnableInterrupts(USR_QTMR_BASEADDR, USR_SECOND_QTMR_CHANNEL, kQTMR_CompareInterruptEnable);

    /* Start the second channel to count on rising edge of the primary source clock */
    QTMR_StartTimer(USR_QTMR_BASEADDR, USR_SECOND_QTMR_CHANNEL, kQTMR_PriSrcRiseEdge);
}

void QTMR_IRQ_HANDLER(void)
{
    /* Clear interrupt flag.*/
    QTMR_ClearStatusFlags(USR_QTMR_BASEADDR, USR_SECOND_QTMR_CHANNEL, kQTMR_CompareFlag);
    ulHighFrequencyTimerTicks ++;
		g_eventTimeMilliseconds ++;
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
