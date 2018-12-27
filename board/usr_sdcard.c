#include "usr_sdcard.h"
#include "ff.h"
#include "diskio.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_sd_disk.h"
#include "fsl_sdmmc_host.h"

#define DEMO_SDCARD_POWER_CTRL_FUNCTION_EXIST

void BOARD_PowerOffSDCARD(void);
void BOARD_PowerOnSDCARD(void);

AT_NONCACHEABLE_SECTION(static FATFS g_fileSystem); /* File system object */
AT_NONCACHEABLE_SECTION(static FIL jpgFil);

static const sdmmchost_detect_card_t s_sdCardDetect = {
#ifndef BOARD_SD_DETECT_TYPE
    .cdType = kSDMMCHOST_DetectCardByGpioCD,
#else
    .cdType = BOARD_SD_DETECT_TYPE,
#endif
    .cdTimeOut_ms = (~0U),
};

/*! @brief SDMMC card power control configuration */
#if defined DEMO_SDCARD_POWER_CTRL_FUNCTION_EXIST
static const sdmmchost_pwr_card_t s_sdCardPwrCtrl = {
    .powerOn = BOARD_PowerOnSDCARD,
    .powerOnDelay_ms = 500U,
    .powerOff = BOARD_PowerOffSDCARD,
    .powerOffDelay_ms = 0U,
};
#endif

void BOARD_PowerOffSDCARD(void)
{
    /* 
        Do nothing here.

        SD card will not be detected correctly if the card VDD is power off, 
       the reason is caused by card VDD supply to the card detect circuit, this issue is exist on EVK board rev A1 and A2.

        If power off function is not implemented after soft reset and prior to SD Host initialization without remove/insert card, 
       a UHS-I card may not reach its highest speed mode during the second card initialization. 
       Application can avoid this issue by toggling the SD_VDD (GPIO) before the SD host initialization.
    */
}

void BOARD_PowerOnSDCARD(void)
{
    BOARD_USDHC_SDCARD_POWER_CONTROL(true);
}

void BOARD_USDHCClockConfiguration(void)
{
    /*configure system pll PFD2 fractional divider to 18*/
    CLOCK_InitSysPfd(kCLOCK_Pfd0, 0x12U);
    /* Configure USDHC clock source and divider */
    CLOCK_SetDiv(kCLOCK_Usdhc1Div, 0U);
    CLOCK_SetMux(kCLOCK_Usdhc1Mux, 1U);
}

static status_t sdcardWaitCardInsert(void)
{
    /* Save host information. */
    g_sd.host.base = SD_HOST_BASEADDR;
    g_sd.host.sourceClock_Hz = SD_HOST_CLK_FREQ;
    /* card detect type */
    g_sd.usrParam.cd = &s_sdCardDetect;
#if defined DEMO_SDCARD_POWER_CTRL_FUNCTION_EXIST
    g_sd.usrParam.pwr = &s_sdCardPwrCtrl;
#endif
    /* SD host init function */
    if (SD_HostInit(&g_sd) != kStatus_Success)          // 初始化 USDHC1 HOST
    {
        PRINTF("\r\nSD host init fail\r\n");
        return kStatus_Fail;
    }
    /* power off card */
    SD_PowerOffCard(g_sd.host.base, g_sd.usrParam.pwr);     // SD 卡掉电
    /* wait card insert */
    if (SD_WaitCardDetectStatus(SD_HOST_BASEADDR, &s_sdCardDetect, true) == kStatus_Success)
    {
        PRINTF("\r\nCard inserted.\r\n");
        /* power on the card */
        SD_HostReset(&(g_sd.host));             // 复位 SD host
        SD_PowerOnCard(g_sd.host.base, g_sd.usrParam.pwr);  // SD 卡上电
        PRINTF("\r\nCard PoweOnCard.\r\n");
    }
    else
    {
        PRINTF("\r\nCard detect fail.\r\n");
        return kStatus_Fail;
    }
		PRINTF("SD Card init start\r\n");
    if (SD_CardInit(&g_sd))
    {
        PRINTF("SD Card Init is  succeed\r\n");
    }
    else
    {
        PRINTF("SD Card Init faild \r\n");
    }
		return kStatus_Success;
}

int MOUNT_SDCard(void)
{
    FRESULT error;
    const TCHAR driverName[3U] = {SDDISK + '0', ':', '/'};
    memset((void *)&g_fileSystem, 0, sizeof(g_fileSystem));

    /* Wait for the card insert. */
    if (sdcardWaitCardInsert() != kStatus_Success)
    {
        PRINTF("Card not inserted.\r\n");
        return -1;
    }

    // Mount the driver
    if (f_mount(&g_fileSystem, driverName, 0))
    {
        PRINTF("Mount volume failed.\r\n");
        return -2;
    }

#if (FF_FS_RPATH >= 2U)
    if (f_chdrive((char const *)&driverName[0U]))
    {
        PRINTF("Change drive failed.\r\n");
        return -3;
    }
#endif

    // // Open file to check
    // error = f_open(&jpgFil, _T("/000.jpg"), FA_OPEN_EXISTING);
    // if (error != FR_OK)
    // {
    //     PRINTF("No demo jpeg file!\r\n");
    //     return -4;
    // }
    // else
    // {
    //     PRINTF("File is exict\n");
    // }

    // f_close(&jpgFil);

    return 0;
}

