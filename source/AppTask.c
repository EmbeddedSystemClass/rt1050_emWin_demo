#include "httpsrv.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "fsl_semc.h"

#include "board.h"

#include "GUI.h"
#include "emwin_support.h"
#include "queue.h"

#include "HomeActivity.h"
/*********************************************************************************
 * 
 *  FreeRTOS 任务的各种参数
 * 
 **********************************************************************************/
//#ifndef HTTPD_DEBUG
//#define HTTPD_DEBUG LWIP_DBG_ON
//#endif


/* Start Task */
#define START_STK_SIZE 1000
#define START_TASK_PRIO 1
TaskHandle_t StartTask_Handle;
static void start_task(void *arg);

/* Enet Task */
#define ENET_STK_SIZE 1024
#define ENET_TASK_PRIO 5
TaskHandle_t EnetTask_Handle;
static void vTaskEnet(void *arg);

/* EmWin touch Task */
#define TOUCH_STK_SIZE 512
#define TOUCH_TASK_PRIO 4
TaskHandle_t TouchTask_Handle;
static void vTaskTouch(void *arg);

/* EmWin exec Task */
#define VTASKGUI_STK_SIZE 1024
#define VTASKGUI_TASK_PRIO 1
TaskHandle_t vTaskGUI_Handle;
static void vTaskGUI(void *arg);

/* Task test Task */
#define VTASKIF_STK_ZISE 512
#define VTASKIF_TASK_PRIO 1
TaskHandle_t vTaskUserIF_Handle;
static void vTaskTaskUserIF(void *arg);

/* 队列参数 */
#define IPQUEUE_LENTH 1
QueueHandle_t IP_Queue;

#define SOCKETQUEUE_LENTH 10
QueueHandle_t Socket_Queue;

SemaphoreHandle_t BinarySem_Handle = NULL;


/********************************************************************
 * 
 *  FreeRTOS 的任务函数
 * 
 ********************************************************************/

/*!
 * @简介 
 */

/******************* 状态打印任务 ****************************
 * 输入参数： arg - 传递值
 * 任务优先级：1 （最低优先级）
 * 任务作用：该函数会打印出各个任务的 CPU 和 RAM 的占用率。
 */
static void vTaskTaskUserIF(void *arg)
{
    uint8_t ucKeyCode;
    uint8_t pcWriteBuffer[500];

    while (1)
    {
        if (!BOARD_Touch_Poll())
        {
            printf("================================================================================\r\n");
            printf("TaskName\t\tTaskStatus\tTaskPRIO\tStack\tTaskNO.\r\n");
            vTaskList((char *)&pcWriteBuffer);
            printf("%s\r\n", pcWriteBuffer);

            printf("=================================================================================\r\n");
            printf("\r\nTaskName\t\tCountRun\t\tUsed\r\n");
            vTaskGetRunTimeStats((char *)pcWriteBuffer);
            printf("%s\r\n", pcWriteBuffer);
        }
        vTaskDelay(20);
    }
}

/********************************* 以太网任务 *********************
 * 输入参数：arg - 传递值
 * 任务优先级：5
 * 函数作用: ①：初始化以太网口，启用 DHCP 服务获取 IP 地址
 *           
 */ 

static void vTaskEnet(void *arg)
{
    BaseType_t xReturn = pdPASS;    /* 定义一个创建信息的返回值，初始值为 pdPASS */
    LWIP_UNUSED_ARG(arg);
    /* Enet initlation and use DHCP server get ip address */
    vusr_enetInit(); 
    // http_server_socket_init();
    // isocketTCPServer(arg);
    vTaskDelete(NULL);
}

/****************************** EmWin 刷新任务 ***********************
 * 任务作用：刷新屏幕
 * 任务优先级： 1
 */  
static void vTaskGUI(void *arg)
{
    PRINTF("Start EmWin \r\n");
    while (1)
    {
#ifdef GUI_BUFFERS
        GUI_MULTIBUF_Begin();
#endif
        GUI_Exec();
#ifdef GUI_BUFFERS
        GUI_MULTIBUF_End();
#endif
        vTaskDelay(5);
    }
}

/*************************** HomeActivity Enet 状态显示任务 *******************
 * 任务作用：用于显示以太网状态
 * 任务优先级：
 */

static void vTaskenetstatus(void *arg)
{
    int i = 0;
    BaseType_t xReturn;
		char rx_buffer; 
    while(1)
    {
        xReturn = xQueuePeek(IP_Queue,&rx_buffer,15);
        if(pdPASS == xReturn)
        {
            printf("get ip succeed\r\n");
			HomeActivity_EnetStatus(ENETSTATUS_ACCESS);
            vTaskDelete(NULL);
        }
        HomeActivity_EnetStatus(ENETSTATUS_LOADING);
    }

}



/******************************* 触摸屏响应任务 ******************
 * 任务作用：用于获取触摸的位置
 * 任务优先级：4
 */ 
static void vTaskTouch(void *arg)
{
    while (1)
    {
        /* Poll touch controller for update */
        BOARD_Touch_Poll();
        vTaskDelay(20);
    }
}

/*************************** 创建任务 *****************
 * 任务作用：用于创建各个任务
 * 任务优先级：1
 */ 

static void start_task(void *arg)
{
    /* 进入临界区 */
    taskENTER_CRITICAL();
    
    /* 创建二值信号量 */
    BinarySem_Handle = xSemaphoreCreateBinary();
    if(NULL == BinarySem_Handle)
    {
        printf("BinarySem_Handle create failed.\r\n");
    }    
    /* 创建消息队列 */
    IP_Queue = xQueueCreate(IPQUEUE_LENTH, sizeof(ip_addr_t));
    Socket_Queue = xQueueCreate(SOCKETQUEUE_LENTH, 135);
    
    /* Enet Task */
    if (xTaskCreate(vTaskEnet, "vTaskEnet", ENET_STK_SIZE, NULL, ENET_TASK_PRIO, &EnetTask_Handle) != pdPASS)
    {
        PRINTF("enet task create faild.\r\n");
        while (1)
            ;
    }
    /* EmWin touch Task */
    if (xTaskCreate(vTaskTouch, "vTaskTouch", TOUCH_STK_SIZE, NULL, TOUCH_TASK_PRIO, &TouchTask_Handle) != pdPASS)
    {
        PRINTF("emwin touch task create faild.\r\n");
        while (1)
            ;
    }
    /* EmWin exec Task */
    if (xTaskCreate(vTaskGUI, "vTaskGUI", VTASKGUI_STK_SIZE, NULL, VTASKGUI_TASK_PRIO, &vTaskGUI_Handle) != pdPASS)
    {
        PRINTF("vTaskGUI create faild.\r\n");
        while (1)
            ;
    }

    if (xTaskCreate(vTaskenetstatus, "vTaskenetstatus", VTASKGUI_STK_SIZE, NULL, VTASKGUI_TASK_PRIO, &vTaskGUI_Handle) != pdPASS)
    {
        PRINTF("vTaskGUI create faild.\r\n");
        while (1)
            ;
    }

    /* Task Test Task */
    // if (xTaskCreate(vTaskTaskUserIF, "vTaskTaskUserIF", VTASKIF_STK_ZISE, NULL, VTASKIF_TASK_PRIO, vTaskUserIF_Handle) != pdPASS)
    // {
    //     PRINTF("vTaskTaskUserIF create faild.\r\n");
    //     while (1)
    //         ;
    // }
    vTaskDelete(NULL);
    taskEXIT_CRITICAL();
}

/************************* 可调用的 API ***************
 * 函数作用：用于其他文件中的函数调用创建任务
 */ 
void AppTask_Start(void)
{
    if (xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &StartTask_Handle) != pdPASS)
    {
        printf("Start task create faild.\r\n");
        while (1)
            ;
    }
    /* run RTOS */
    vTaskStartScheduler();
}
