#include "DIALOG.h"
#include "smartphone.c"
#include "startmenu.c"
#include "EnetActivity.h"
#include "HomeActivity.h"
#include "SocketActivity.h"
#include "giphy_1.c"
#include "internet.c"

#include "FreeRTOS.h"
#include "queue.h"
#include "ip_addr.h"
#include "WeatherActivity.h"

/**************************** Define 定义区 **********************************************************/
#define JPEGRAM_USE	81792		/* 480*100 + 33*1024 */

#define APPID_GAME		0x01
#define APPID_MOVIE		0x02
#define	APPID_CAMERA	0x03
#define APPID_SETTING	0x11
#define APPID_TIME		0x12
#define APPID_MAIL		0x13
#define APPID_WEATHER	0x14

#define SMENUID_EXIT		0x21
#define SMENUID_POWEROFF	0x22

#define APP_WIDTH		40
#define APP_HEIGHT		35

#define SMENU_WIDTH		30
#define SMENU_HEIGHT	30

#define ID_WINDOW_01	(GUI_ID_USER + 0x00)
#define ID_WINDOW_02	(GUI_ID_USER + 0x01)
#define ID_WINDOW_03	(GUI_ID_USER + 0x02)
#define ID_WINDOW_04	(GUI_ID_USER + 0x03)
#define ID_WINDOW_05	(GUI_ID_USER + 0x04)

#define ID_BUTTON_01	(GUI_ID_USER + 0x10)
#define ID_BUTTON_02	(GUI_ID_USER + 0x11)
#define ID_BUTTON_03	(GUI_ID_USER + 0x12)
#define ID_BUTTON_04	(GUI_ID_USER + 0x13)
#define ID_BUTTON_05	(GUI_ID_USER + 0x14)
#define ID_BUTTON_06	(GUI_ID_USER + 0x15)
#define ID_BUTTON_07	(GUI_ID_USER + 0x16)
#define ID_BUTTON_21	(GUI_ID_USER + 0x20)
#define ID_BUTTON_22	(GUI_ID_USER + 0x21)


#define APP_SOCKET_WIN_WIDTH	480
#define APP_SOCKET_WIN_HEIGHT	232


/************************************ 结构体定义区 *******************************************************/
typedef struct {
	int ID;						/* Application ID */
	int xPos;					/* Application x pos*/
	int yPos;					/* Application y pos*/
	int xSize;				/* Application x size */
	int ySize;				/* Application y size */
	char * AppName;		/* Application name */
}APP;

static const APP SmartPhoneApp[] = {
	{APPID_GAME,	10, 20,  APP_WIDTH, APP_HEIGHT, "Game"},
	{APPID_MOVIE,	10, 65,  APP_WIDTH, APP_HEIGHT, "Movie"},
	{APPID_CAMERA,	10, 110, APP_WIDTH, APP_HEIGHT, "Camera"},
	{APPID_SETTING, 10, 232, APP_WIDTH, APP_HEIGHT, "Setting"},
	{APPID_TIME,	60, 232, APP_WIDTH, APP_HEIGHT, "Time"},
	{APPID_MAIL,	110, 232, APP_WIDTH, APP_HEIGHT, "Mail"},
	{APPID_WEATHER, 160, 232, APP_WIDTH, APP_HEIGHT, "Weather"},
};

static const APP StartMenu[] = {
	{SMENUID_EXIT,		7, 100, SMENU_WIDTH, SMENU_HEIGHT, "Exit"},
	{SMENUID_POWEROFF,  5, 143, SMENU_WIDTH, SMENU_HEIGHT, "PowerOff"},
};

/********************************** 外部变量区 ******************************/
extern QueueHandle_t Socket_Queue;
extern QueueHandle_t IP_Queue;

/********************************* 全局变量区 *******************************/

/* 字符串 */
static char *IP_Text = { "0.0.0.0" };
static char Queue_season;
/* 句柄变量 */
GUI_HWIN hWin_Background;
static GUI_HWIN hWin_StartMenu;
static GUI_HWIN hBut_EnetStatus;
static GUI_HWIN hWin_IPText;

/* 计数变量 */
static int GifFarme = 0;

/* 标志变量 */
static char Flag_HomeActivity = 0;
static char GetIp_Flag = 0;


/********************************* 函数申明区 **********************************/
static void _cbButton_App(WM_MESSAGE * pMsg);
static void _cbWindow_Background(WM_MESSAGE * pMsg);
static void _cbWindow_StartMenu(WM_MESSAGE * pMsg);
static void _cbButton_EnetStatus(WM_MESSAGE *pMsg);
static void _cbWindow_IPText(WM_MESSAGE *pMsg);

static void _CreateWindow_StartMenu(void);
static void _CreateWindow_SmartPhone_HomeActivity(void);
static void _CreateWindow_EnetStatus(void);
static void _CreateWindow_IPText(void);

static void DeleteWindow_StartMenu(void);

static void Disp_IPText(void);
static void Disp_EnetStatus(void);

/*******************************************************************************
 *
 *	Static 函数定义区
 *
 *******************************************************************************/

/* This is Application button callback function */
static void _cbButton_App(WM_MESSAGE * pMsg)
{
	int Id;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		if (BUTTON_IsPressed(pMsg->hWin))
		{
			Id = WM_GetId(pMsg->hWin);
			switch ((Id - GUI_ID_USER) / 16)
			{
			case 0x00:
				GUI_SetColor(0xDD000000 | GUI_RED);
				GUI_FillRoundedRect(SmartPhoneApp->xSize - SmartPhoneApp->ySize, 0, SmartPhoneApp->ySize, SmartPhoneApp->ySize, 15);
				break;
			case 0x01:
				GUI_SetColor(0xDD000000 | GUI_RED);
				GUI_FillRoundedRect(SmartPhoneApp->xSize - SmartPhoneApp->ySize, 0, SmartPhoneApp->ySize, SmartPhoneApp->ySize, 15);
				break;
			case 0x02:
				GUI_SetColor(0xDD000000 | GUI_WHITE);
				GUI_FillRect(0, 0, SmartPhoneApp->ySize, SmartPhoneApp->ySize);
				break;
			}
		}
		break;
	default:
		BUTTON_Callback(pMsg);
	}
}

static void _cbButton_EnetStatus(WM_MESSAGE *pMsg)
{
	int Id, NCode;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_SetBkColor(0x00474343);
		GUI_Clear();
		if(!GifFarme)
		{
			GUI_JPEG_Draw(_acinternet, 1024*40, 2, 7);
		}
		else
		{
			Disp_EnetStatus();
		}
		break;
	default:
		BUTTON_Callback(pMsg);
	}
}

/* This is Application background callback function */
static void _cbWindow_Background(WM_MESSAGE * pMsg)
{
	int Id, NCode;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_JPEG_Draw(_acsmartphone, JPEGRAM_USE, 0, 0);
		break;
	case WM_NOTIFY_PARENT:
		NCode = pMsg->Data.v;
		Id = WM_GetId(pMsg->hWinSrc);
		switch (NCode)
		{
		case WM_NOTIFICATION_CLICKED:
			switch (Id)
			{
			case ID_BUTTON_01:
				HideHomeActivity();
				CreateEnetActivity();
				break;
			case ID_BUTTON_02:
				break;
			case ID_BUTTON_03:
				break;
			case ID_BUTTON_04:
				_CreateWindow_StartMenu();
				break;
			case ID_BUTTON_05:
				break;
			case ID_BUTTON_06:
				CreateActivitySocket();
				break;
			case ID_BUTTON_07:
				CreateActivityWeather();
				break;
			case ID_WINDOW_03:
				_CreateWindow_IPText();
				break;
			}
			break;
		case WM_NOTIFICATION_RELEASED:
			break;
		}
		break;
	default:
		WM_DefaultProc(pMsg);
	}
}

static void _cbWindow_StartMenu(WM_MESSAGE * pMsg)
{
	int Id, NCode;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_JPEG_Draw(_acstartmenu, JPEGRAM_USE, 0, 0);
		break;
	case WM_NOTIFY_PARENT:
		Id = WM_GetId(pMsg->hWinSrc);
		NCode = pMsg->Data.v;
		switch (NCode)
		{
		case WM_NOTIFICATION_RELEASED:
			switch (Id)
			{
			case ID_BUTTON_21:
				HideActivitySocket();
				HideActivityWeather();
				DeleteWindow_StartMenu();
				break;
			case ID_BUTTON_22:
				DeleteActivitySocket();
				DeleteActivityWeather();
				DeleteWindow_StartMenu();
				break;
			}
			break;
		}
	default:
		WM_DefaultProc(pMsg);
	}
}

static void _cbWindow_IPText(WM_MESSAGE *pMsg)
{
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_SetColor(0x88 << 24 | GUI_WHITE);
		GUI_FillRect(0, 0, 100, 20);
		Disp_IPText();
		break;
	default:
		WM_DefaultProc(pMsg);
	}
}

static void _CreateWindow_StartMenu(void)
{
	GUI_HWIN hWin;
	int i;
	if (0 != hWin_StartMenu)
	{
		WM_DeleteWindow(hWin_StartMenu);
		hWin_StartMenu = 0;
		return;
	}
	hWin_StartMenu = WM_CreateWindowAsChild(0, 55, 200, 177, hWin_Background, WM_CF_SHOW, _cbWindow_StartMenu, 0);
	for (i = 0; i < GUI_COUNTOF(StartMenu); i++)
	{
		hWin = BUTTON_CreateUser(StartMenu[i].xPos, StartMenu[i].yPos, StartMenu[i].xSize, StartMenu[i].ySize,
			hWin_StartMenu, WM_CF_SHOW | WM_CF_HASTRANS, 0, ID_BUTTON_21 + i, 0);
		WM_SetCallback(hWin, _cbButton_App);
	}
}

static void _CreateWindow_SmartPhone_HomeActivity(void)
{
	GUI_HWIN hBut;
	int i;
	hWin_Background = WM_CreateWindow(0, 0, 480, 272, WM_CF_SHOW, _cbWindow_Background, 0);
	for (i = 0; i < GUI_COUNTOF(SmartPhoneApp); i++)
	{
		hBut = BUTTON_CreateUser(SmartPhoneApp[i].xPos, SmartPhoneApp[i].yPos, SmartPhoneApp[i].xSize, SmartPhoneApp[i].ySize,
			hWin_Background, WM_CF_SHOW | WM_CF_HASTRANS, 0, ID_BUTTON_01 + i, 0);
		WM_SetCallback(hBut, _cbButton_App);
	}
}

static void _CreateWindow_EnetStatus(void)
{
	hBut_EnetStatus = BUTTON_CreateUser(380, 240, 23, 27, hWin_Background, WM_CF_SHOW|WM_CF_HASTRANS, 0, ID_WINDOW_03, 0);
	WM_SetCallback(hBut_EnetStatus, _cbButton_EnetStatus);
}

static void _CreateWindow_IPText(void)
{
	if (0 != hWin_IPText)
	{
		WM_DeleteWindow(hWin_IPText);
		hWin_IPText = 0;
		return;
	}
	hWin_IPText = WM_CreateWindowAsChild(340, 212, 100, 20, hWin_Background, WM_CF_SHOW | WM_CF_HASTRANS, _cbWindow_IPText, 0);
}

static void DeleteWindow_StartMenu(void)
{
	if (0 != hWin_StartMenu)
	{
		WM_DeleteWindow(hWin_StartMenu);
		hWin_StartMenu = 0;
	}
}

static void Disp_IPText(void)
{
	char *IP_Text_1 = NULL;
	BaseType_t xReturn;
	ip_addr_t dhcp_ip;
	GUI_SetColor(GUI_BLACK);
	GUI_SetFont(&GUI_Font16_1);
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	xReturn = xQueuePeek(IP_Queue, &dhcp_ip, 2);
	if (pdPASS != xReturn)
	{
		GUI_DispStringHCenterAt(IP_Text, 50, 3);
	}
	else
	{
		IP_Text_1 = (char *)malloc(30);
		strcpy(IP_Text_1, ipaddr_ntoa(&dhcp_ip));
		GUI_DispStringHCenterAt(IP_Text_1, 50, 3);
		free(IP_Text_1);
	}
}

static void Disp_EnetStatus(void)
{
	GUI_GIF_INFO InfoGif;
	GUI_GIF_IMAGE_INFO ImageInfoGif;
	GUI_GIF_GetInfo(_acgiphy_1, 1024 * 400, &InfoGif);
	if (GifFarme < InfoGif.NumImages)
	{
		GUI_GIF_GetImageInfo(_acgiphy_1, 1024 * 400, &ImageInfoGif, GifFarme);
		GUI_GIF_DrawSub(_acgiphy_1, 1024 * 400, 0, 0, GifFarme);
	}
	else
	{
		GifFarme = 0;
	}
}


/*********************************************************************************
 *
 *		能被外部调用的函数定义区
 *
 ********************************************************************************/

void MainTask(void)
{
	GUI_Init();
	GUI_EnableAlpha(1);
	WM_MULTIBUF_Enable(1);
	CreateHomeActivity();
	GUI_Exec();
}

void CreateHomeActivity(void)
{
	if (Flag_HomeActivity == 1)
	{
		WM_ShowWindow(hWin_Background);
		return;
	}
	_CreateWindow_SmartPhone_HomeActivity();
	_CreateWindow_EnetStatus();
	Flag_HomeActivity = 1;
}
void HideHomeActivity(void)
{
	if (Flag_HomeActivity == 0)
	{
		return;
	}
	WM_HideWindow(hWin_Background);
}

void DeleteHomeActivity(void)
{
	if (Flag_HomeActivity == 0)
	{
		return;
	}
	WM_DeleteWindow(hWin_Background);
}

void HomeActivity_EnetStatus(int flag)
{
	if(flag)
	{
		GifFarme++;
		WM_InvalidateWindow(hBut_EnetStatus);
	}
	else
	{
		GifFarme = 0;
		GetIp_Flag = 1;
		WM_InvalidateWindow(hBut_EnetStatus);
	}
}