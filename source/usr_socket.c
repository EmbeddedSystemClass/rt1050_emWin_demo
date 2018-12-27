#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "sys.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "SocketActivity.h"


/**************************************** Define ************************************************/
#define socketPORT 5000

/* TCP Socket Server Task */
#define TSS_STK_SIZE 1024
#define TSS_TASK_PRIO 3
TaskHandle_t TSSTask_Handle;
static int isocketTCPServer(void *arg);

/* TCP Socket Client Task */
#define TSC_STK_SIZE 512
#define TSC_TASK_PRIO 2
TaskHandle_t TSCTask_Handle;
static void isocketClientRead(void *arg);

/***************************************** 全局变量 *************************************************/
static int sock_server;


/***************************************** 外部变量 *****************************************************/
extern QueueHandle_t IP_Queue;
extern QueueHandle_t Socket_Queue;
//extern int ENETACTIVITY_Socket_Flag;

/****************************************** 函数声明 *************************************************/




/****************************************** static 函数 *****************************************************************/
static int isocketTCPServer(void * arg)
{
    struct sockaddr_in server_addr, client_addr;
    int sock_client;
    int client_addr_size, err;
    ip_addr_t queue_ipaddr;
    char server_queue[] = {"server access succeed.\r\n"};
    if ((sock_server = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        PRINTF("Socket Create failed.\r\n");
        return -1;
    }
		xQueuePeek(IP_Queue, &queue_ipaddr, portMAX_DELAY);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ipaddr_ntoa(&queue_ipaddr));
    server_addr.sin_port = htons(socketPORT);
    client_addr_size = sizeof(client_addr);
    if (bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        PRINTF("bind failed");
    }
    if (listen(sock_server, 10) == -1)
    {
        perror("listen");
    }
    while (1)
    {
        printf("wait socket client\r\n");
        sock_client = accept(sock_server, (struct sockaddr *)&client_addr, &client_addr_size);
				CreateActivitySocket_MsgButton(sock_client);
        write(sock_client, server_queue, sizeof(server_queue));
        if (xTaskCreate(isocketClientRead, "Client Read server", TSC_STK_SIZE, (void *)&sock_client , TSC_TASK_PRIO, NULL) != pdPASS)
        {
            PRINTF("isocketClientRead create failed.\r\n");
            while (1);
        }
				/* 客户端不能超过 3,否则会出错，原因暂时未查到 */
				if(sock_client == 3)		
				{
					vTaskSuspend(NULL);
				}
    }
}

static void isocketClientRead(void *arg)
{
    char *msg_buffer = NULL;
		int sock_client;
		err_t err;
		sock_client = *((int *)arg);
		msg_buffer = (char*)calloc(1, 135);
		memset(msg_buffer, 0, 135);
    msg_buffer[0] = sock_client;
		while (1)
    {
			err = read(sock_client, &msg_buffer[1], 134);
			if(err == 0)
			{
				break;
			}
			PRINTF("receive msg : %x\r\n", msg_buffer);
			xQueueSend(Socket_Queue, msg_buffer, 134);
    }
		free(msg_buffer);
		close(sock_client);
    vTaskDelete(NULL);
}

/********************************* 可调用函数 ************************************************************************/
void OpenSocketServer(void)
{
	if (xTaskCreate(isocketTCPServer, "Create Socket server", TSS_STK_SIZE, NULL , TSS_TASK_PRIO, &TSSTask_Handle) != pdPASS)
	{
			PRINTF("Socket server create failed.\r\n");
			while (1)
					;
	}
}

void CloseSocketServer(void)
{
	close(sock_server);
	if(NULL != TSSTask_Handle)
	{
		vTaskDelete(TSSTask_Handle);
	}
}	
