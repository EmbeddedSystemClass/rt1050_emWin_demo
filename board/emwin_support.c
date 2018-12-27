/*
 * The Clear BSD License
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "GUI.h"
#include "WM.h"
#include "emwin_support.h"
#include "GUIDRV_Lin.h"

#include "fsl_debug_console.h"
#include "fsl_elcdif.h"
#include "fsl_lpi2c.h"
#include "fsl_ft5406_rt.h"
#include "fsl_gpio.h"
#include "clock_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
/*
**      Define everything necessary for different color depths
*/

#if LCD_BITS_PER_PIXEL < 32 /* Buffer definitions for emwin and LCD framebuffer */
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t s_gui_memory[GUI_NUMBYTES * LCD_BYTES_PER_PIXEL], FRAME_BUFFER_ALIGN);
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t s_vram_buffer[VRAM_SIZE * GUI_BUFFERS * LCD_BYTES_PER_PIXEL], FRAME_BUFFER_ALIGN);

#else
uint32_t s_gui_memory[(GUI_NUMBYTES)];
uint32_t s_vram_buffer[VRAM_SIZE * GUI_BUFFERS];
#endif

/* Memory address definitions */
#define GUI_MEMORY_ADDR ((uint32_t)s_gui_memory)
#define VRAM_ADDR ((uint32_t)s_vram_buffer)

static volatile int32_t s_LCDpendingBuffer = -1;
static xSemaphoreHandle xQueueMutex;
static xSemaphoreHandle xSemaTxDone;
/*******************************************************************************
 * Implementation of PortAPI for emWin LCD driver
 ******************************************************************************/
/* Enable interrupt. */
void BOARD_EnableLcdInterrupt(void)
{
    EnableIRQ(LCDIF_IRQn);
}

void APP_LCDIF_IRQHandler(void)
{
    uint32_t intStatus;

    intStatus = ELCDIF_GetInterruptStatus(APP_ELCDIF);

    ELCDIF_ClearInterruptStatus(APP_ELCDIF, intStatus);

    if (intStatus & kELCDIF_CurFrameDone)
    {
        if (s_LCDpendingBuffer >= 0)
        {
            /* Send a confirmation that the given buffer is visible */
            GUI_MULTIBUF_Confirm(s_LCDpendingBuffer);
            s_LCDpendingBuffer = -1;
        }
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void LCDIF_IRQHandler(void)
{
    APP_LCDIF_IRQHandler();
}

void APP_ELCDIF_Init(void)
{
    const elcdif_rgb_mode_config_t config = {
        .panelWidth = APP_IMG_WIDTH,
        .panelHeight = APP_IMG_HEIGHT,
        .hsw = APP_HSW,
        .hfp = APP_HFP,
        .hbp = APP_HBP,
        .vsw = APP_VSW,
        .vfp = APP_VFP,
        .vbp = APP_VBP,
        .polarityFlags = APP_POL_FLAGS,
        .bufferAddr = VRAM_ADDR,
        .pixelFormat = ELCDIF_PIXEL_FORMAT,
        .dataBus = APP_LCDIF_DATA_BUS,
    };
    ELCDIF_RgbModeInit(APP_ELCDIF, &config);

    BOARD_EnableLcdInterrupt();
    ELCDIF_EnableInterrupts(APP_ELCDIF, kELCDIF_CurFrameDoneInterruptEnable);
    NVIC_EnableIRQ(LCDIF_IRQn);
    ELCDIF_RgbModeStart(APP_ELCDIF);
}

/*******************************************************************************
 * Implementation of communication with the touch controller
 ******************************************************************************/

/* Touch driver handle. */
static ft5406_rt_handle_t touchHandle;

static void BOARD_Touch_Init(void)
{
    lpi2c_master_config_t masterConfig = {0};
    /*
    * masterConfig.debugEnable = false;
    * masterConfig.ignoreAck = false;
    * masterConfig.pinConfig = kLPI2C_2PinOpenDrain;
    * masterConfig.baudRate_Hz = 100000U;
    * masterConfig.busIdleTimeout_ns = 0;
    * masterConfig.pinLowTimeout_ns = 0;
    * masterConfig.sdaGlitchFilterWidth_ns = 0;
    * masterConfig.sclGlitchFilterWidth_ns = 0;
    */
    LPI2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Hz = BOARD_TOUCH_I2C_BAUDRATE;

    /* Initialize the LPI2C master peripheral */
    LPI2C_MasterInit(BOARD_TOUCH_I2C, &masterConfig, BOARD_TOUCH_I2C_CLOCK_FREQ);

    /* Initialize the touch handle. */
    FT5406_RT_Init(&touchHandle, BOARD_TOUCH_I2C);
}

void BOARD_Touch_Deinit(void)
{
    LPI2C_MasterDeinit(BOARD_TOUCH_I2C);
}

int BOARD_Touch_Poll(void)
{
    touch_event_t touch_event;
    int touch_x;
    int touch_y;
    GUI_PID_STATE pid_state;

    if (kStatus_Success != FT5406_RT_GetSingleTouch(&touchHandle, &touch_event, &touch_x, &touch_y))
    {
        return 0;
    }
    else if (touch_event != kTouch_Reserved)
    {
        vTaskDelay(15);
        if (touch_event != kTouch_Reserved)
        {
            pid_state.x = touch_y;
            pid_state.y = touch_x;
            pid_state.Pressed = ((touch_event == kTouch_Down) || (touch_event == kTouch_Contact));
            pid_state.Layer = 0;
            GUI_TOUCH_StoreStateEx(&pid_state);
            return 1;
        }
    }
    return 0;
}

/*******************************************************************************
 * Application implemented functions required by emWin library
 ******************************************************************************/
void LCD_X_Config(void)
{
    GUI_MULTIBUF_Config(GUI_BUFFERS);
    GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER, COLOR_CONVERSION, 0, 0);
    LCD_SetSizeEx(0, LCD_WIDTH, LCD_HEIGHT);
    LCD_SetVSizeEx(0, LCD_WIDTH, LCD_HEIGHT);
    LCD_SetVRAMAddrEx(0, (void *)VRAM_ADDR);
    BOARD_Touch_Init();
}

int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void *p)
{
    uint32_t addr;
    int result = 0;
    LCD_X_SHOWBUFFER_INFO *pData;
    switch (Cmd)
    {
    case LCD_X_INITCONTROLLER:
    {
        APP_ELCDIF_Init();
        break;
    }
    case LCD_X_SHOWBUFFER:
    {
        pData = (LCD_X_SHOWBUFFER_INFO *)p;
        /* Calculate address of the given buffer */
        addr = VRAM_ADDR + VRAM_SIZE * pData->Index;
        /* Make the given buffer visible */
        ELCDIF_SetNextBufferAddr(APP_ELCDIF, addr);
        //
        // Remember buffer index to be used by ISR
        //
        s_LCDpendingBuffer = pData->Index;
        while (s_LCDpendingBuffer >= 0)
            ;
        return 0;
    }

    default:
        result = -1;
        break;
    }

    return result;
}

void GUI_X_Config(void)
{
    /* Assign work memory area to emWin */
    GUI_ALLOC_AssignMemory((void *)GUI_MEMORY_ADDR, GUI_NUMBYTES);

    /* Select default font */
    GUI_SetDefaultFont(GUI_FONT_6X8);
}

void GUI_X_Init(void)
{
}

/* Dummy RTOS stub required by emWin */
void GUI_X_InitOS(void)
{
    /* Create Mutex lock */
    xQueueMutex = xSemaphoreCreateMutex();
    configASSERT(xQueueMutex != NULL);

    /* Queue Semaphore */
    vSemaphoreCreateBinary(xSemaTxDone);
    configASSERT(xSemaTxDone != NULL);
}

/* Dummy RTOS stub required by emWin */
void GUI_X_Lock(void)
{
    if (xQueueMutex == NULL)
    {
        GUI_X_InitOS();
    }

    xSemaphoreTake(xQueueMutex, portMAX_DELAY);
}

/* Dummy RTOS stub required by emWin */
void GUI_X_Unlock(void)
{
    xSemaphoreGive(xQueueMutex);
}

/* Dummy RTOS stub required by emWin */
U32 GUI_X_GetTaskId(void)
{
    return ((U32)xTaskGetCurrentTaskHandle());
}

void GUI_X_ExecIdle(void)
{
}

void GUI_X_WaitEvent(void)
{
    while (xSemaphoreTake(xSemaTxDone, portMAX_DELAY) != pdTRUE)
        ;
}

void GUI_X_SignalEvent(void)
{
    xSemaphoreGive(xSemaTxDone);
}

GUI_TIMER_TIME GUI_X_GetTime(void)
{
    return ((int)xTaskGetTickCount());
}

void GUI_X_Delay(int Period)
{
    vTaskDelay(Period);
}

void *emWin_memcpy(void *pDst, const void *pSrc, long size)
{
    return memcpy(pDst, pSrc, size);
}
