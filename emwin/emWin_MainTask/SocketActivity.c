#include "SocketActivity.h"
#include "DIALOG.h"
#include "usr_socket.h"
#include "stdlib.h"
#include "sys.h"
/******************************************** Define 定义 **************************************/
#define HEIGHT	232
#define WIDTH	480

#define ID_WINDOW_01	(GUI_ID_USER + 0x00)

#define ID_BUTTON_SOCKET (GUI_ID_USER + 0x10)
#define ID_BUTTON_MSG	(GUI_ID_USER + 0x11)

#define COLOR_WITHE	    0xAA555555
#define COLOR_GREEN		0x8800AA00

#define MSG_NUM		134

/* Queue Read Task */
#define QR_STK_SIZE 1000
#define QR_TASK_PRIO 1
TaskHandle_t QueueReadTask_Handle;
static void vTaskQueueRead(void *arg);
/******************************************** sturct ***************************************/
typedef struct {
	int xPos;
	int yPos;
	int xSize;
	int ySize;
	int Index;
	int MsgId;
	int Msg[135];
} MSG_BUTTON;


/******************************************** 外部变量 *****************************************/


extern GUI_HWIN hWin_Background;
extern QueueHandle_t Socket_Queue;
/********************************************* 全局变量 ****************************************/
static int Msg_Count = 0;
static U32 SocketButtonColor = COLOR_WITHE;
static char SocketButtonString[13] = "Open Socket";
static GUI_HWIN hWin_Socket = NULL;
static GUI_RECT	Rect_mail_1 = { 0, 0, 40, 232 };
static GUI_RECT	Rect_mail_2 = { 40, 0, 160, 232 };

static U8 SocketString[MSG_NUM] = {0};
static U8 SocketString_1[MSG_NUM] = {0};
static U8 SocketString_2[MSG_NUM] = {0};
static U8 SocketString_3[MSG_NUM] = {0};
/******************************************** 函数声明 ***************************************/

static void _cbWindow_SocketActivity(WM_MESSAGE *pMsg);
static void _cbButton_Msg(WM_MESSAGE *pMsg);


static void _CreateButton_msg(MSG_BUTTON msg);
static void _CreateButton_OpenSocket(void);


static MSG_BUTTON SetButton_Msg(void);
static void _CreateActivity(void);
static int FindQueueEnd(char * pMsg);

static void CreateActivitySocket_QueueRead(void);
static void DeleteActivitySocket_QueueRead(void);
/******************************************** Static 函数 ***************************************/

static void _cbWindow_SocketActivity(WM_MESSAGE *pMsg)
{
	int Id, NCode;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_SetBkColor(GUI_WHITE);
		GUI_Clear();
		GUI_SetColor(0x00AAAAAA);
		GUI_FillRectEx(&Rect_mail_1);
		GUI_SetColor(0x00E0E0E0);
		GUI_FillRectEx(&Rect_mail_2);
		GUI_SetTextMode(GUI_TEXTMODE_TRANS);
		GUI_SetFont(&GUI_Font20_1);
		GUI_SetColor(GUI_BLACK);
		GUI_DispStringAt(SocketString,200,20);
		break;
	case WM_NOTIFY_PARENT:
		NCode = pMsg->Data.v;
		switch (NCode)
		{
		case WM_NOTIFICATION_CLICKED:
			Id = WM_GetId(pMsg->hWinSrc);
			GUI_SetColor(GUI_BLACK);
			switch (Id)
			{
			case ID_BUTTON_SOCKET:
				if (SocketButtonColor == COLOR_WITHE)
				{
					GUI__memcpy(SocketButtonString, "Close Scoket", 13);
					SocketButtonColor = COLOR_GREEN;
					OpenSocketServer();
					CreateActivitySocket_QueueRead();
				}
				else
				{
					GUI__memcpy(SocketButtonString, "Open Scoket", 12);
					SocketButtonColor = COLOR_WITHE;
					CloseSocketServer();
					DeleteActivitySocket_QueueRead();
				}
				break;
			case ID_BUTTON_MSG + 1:
				GUI__memset(SocketString, 0, MSG_NUM);
				GUI__memcpy(SocketString,SocketString_1,MSG_NUM);
				WM_InvalidateWindow(hWin_Socket);
				break;
			case ID_BUTTON_MSG + 2:
				GUI__memset(SocketString, 0, MSG_NUM);
				GUI__memcpy(SocketString,SocketString_2,MSG_NUM);
				WM_InvalidateWindow(hWin_Socket);	
				break;
			case ID_BUTTON_MSG + 3:
				GUI__memset(SocketString, 0, MSG_NUM);
				GUI__memcpy(SocketString,SocketString_3,MSG_NUM);
				WM_InvalidateWindow(hWin_Socket);	
				break;
			}
			break;
		}
		break;
	default:
		WM_DefaultProc(pMsg);
	}
}

static void _cbButton_Msg(WM_MESSAGE *pMsg)
{
	MSG_BUTTON *msg = NULL;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		WM_GetUserData(pMsg->hWin, &msg,sizeof(MSG_BUTTON **));
		if(BUTTON_IsPressed(pMsg->hWin))
		{
			GUI_SetBkColor(0xAA00AAAA);
			GUI_Clear();
		}
		else
		{
			GUI_SetBkColor(0xAAAAAAAA);
			GUI_Clear();
		}
		GUI_SetTextMode(GUI_TEXTMODE_TRANS);
		GUI_SetColor(GUI_BLACK);
		switch(msg->Index)
		{
			case 1:
				GUI_DispStringAt("Client 1", 5, 10);
				break;
			case 2:
				GUI_DispStringAt("Client 2", 5, 10);
				break;
			case 3:
				GUI_DispStringAt("Client 3", 5, 10);
				break;
			default:
				GUI_DispStringAt("Error", 5, 10);
		}
		break;
	default:
		BUTTON_Callback(pMsg);
		break;
	}
}


static void _cbButton_Socket(WM_MESSAGE *pMsg)
{
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_SetColor(SocketButtonColor);
		GUI_AA_FillRoundedRect(0, 0, 100, 30, 15);
		GUI_SetTextMode(GUI_TEXTMODE_TRANS);
		GUI_SetColor(GUI_WHITE);
		GUI_DispStringHCenterAt(SocketButtonString, 50, 10);
		break;
	default:
		BUTTON_Callback(pMsg);
		break;
	}
}

static void _CreateButton_msg(MSG_BUTTON msg)
{
	WM_HWIN hButton_Msg;
	MSG_BUTTON *Msg = &msg;
	hButton_Msg = BUTTON_CreateUser(msg.xPos, msg.yPos, msg.xSize, msg.ySize, hWin_Socket, WM_CF_SHOW | WM_CF_HASTRANS, 0, msg.Index + ID_BUTTON_MSG, sizeof(MSG_BUTTON **));
	WM_SetUserData(hButton_Msg, &Msg,sizeof(MSG_BUTTON **));
	WM_SetCallback(hButton_Msg, _cbButton_Msg);
}

static MSG_BUTTON SetButton_Msg(void)
{
	MSG_BUTTON msg;
	msg.Index = Msg_Count;
	msg.xPos = 40;
	msg.xSize = 120;
	msg.yPos = 50 + Msg_Count * (30 + 5);
	msg.ySize = 30;
	return msg;
}

static void _CreateButton_OpenSocket(void)
{
	GUI_HWIN hButton;
	hButton = BUTTON_CreateUser(50, 20, 100, 30, hWin_Socket, WM_CF_SHOW | WM_CF_HASTRANS, 0, ID_BUTTON_SOCKET, 0);
	WM_SetCallback(hButton, _cbButton_Socket);
}

static void _CreateActivity(void)
{
	hWin_Socket = WINDOW_CreateUser(0, 0, WIDTH, HEIGHT, hWin_Background, WM_CF_SHOW | WM_CF_HASTRANS, 0, ID_WINDOW_01, _cbWindow_SocketActivity, 0);
	_CreateButton_OpenSocket();
}

static int FindQueueEnd(char * pMsg)
{
	int i;
	if(NULL == pMsg)
	{
		return -1;
	}
	for(i = 0; i < MSG_NUM; i++)
	{
		if('\0' == pMsg[i])
		{
			return i;
		}
	}
}

static void vTaskQueueRead(void *arg)
{
	char msg_buffer[MSG_NUM] = {0};
	int msg_len;
	while(1)
	{
		xQueueReceive(Socket_Queue, msg_buffer, portMAX_DELAY);
		if(-1 == (msg_len = FindQueueEnd(msg_buffer)))
		{
			return;
		}
		switch(msg_buffer[0])
		{
			case 1:
				GUI__memcpy(SocketString_1, &msg_buffer[1], msg_len); 
				GUI__memcpy(SocketString,SocketString_1,MSG_NUM);
				WM_InvalidateWindow(hWin_Socket);
				break;
			case 2:
				GUI__memcpy(SocketString_2, &msg_buffer[1], msg_len); 
				break;
			case 3:
				GUI__memcpy(SocketString_3, &msg_buffer[1], msg_len); 
				break;
			default:
				break;
		}
		WM_InvalidateWindow(hWin_Socket);
	}
	vTaskDelete(NULL);
}

static void CreateActivitySocket_QueueRead(void)
{
	if (xTaskCreate(vTaskQueueRead, "Socket Activity Queue Read", QR_STK_SIZE, NULL, QR_TASK_PRIO, &QueueReadTask_Handle) != pdPASS)
	{
			PRINTF("isocketClientRead create failed.\r\n");
			while (1);
	}
}

static void DeleteActivitySocket_QueueRead(void)
{
	vTaskDelete(QueueReadTask_Handle);
}
	


/*********************************************** 外部调用的函数 **********************************/
void CreateActivitySocket(void)
{
	if (NULL != hWin_Socket)
	{
		WM_ShowWindow(hWin_Socket);
		return;
	}
	_CreateActivity();
}

void DeleteActivitySocket(void)
{
	if (0 == hWin_Socket)
	{
		return;
	}
	WM_DeleteWindow(hWin_Socket);
	hWin_Socket = 0;
}

void HideActivitySocket(void)
{
	if (0 == hWin_Socket)
	{
		return;
	}
	WM_HideWindow(hWin_Socket);
}

void CreateActivitySocket_MsgButton(int ClientId)
{
	MSG_BUTTON msg;
	Msg_Count = ClientId;
	msg = SetButton_Msg();
	_CreateButton_msg(msg);
}