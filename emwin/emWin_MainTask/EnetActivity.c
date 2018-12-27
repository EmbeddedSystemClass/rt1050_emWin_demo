#include "DIALOG.h"
#include "bg_480x272.c"
#include "DIALOG.h"
#include "stdlib.h"
#include "spring.c"
#include "summer.c"
#include "autumn.c"
#include "winter.c"
#include "smartphone.c"
#include "EnetActivity.h"
#include "HomeActivity.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "string.h"
#include "ip_addr.h"
#include "usr_socket.h"

char Queue_season;
int ucGetip_Flag;
extern QueueHandle_t Socket_Queue;
extern QueueHandle_t IP_Queue;

/****************************************************************
* 
* 	Define
* 
****************************************************************/
#define JPEGRAM_USE 81792 /* 480*100 + 33*1024 */

#define FONT_BARMENU GUI_Font16_ASCII

#define ID_WINDOW_0 (GUI_ID_USER + 0x00)
#define ID_BUTTON_0 (GUI_ID_USER + 0x10)

#define _INDEX_PAGE 1
#define _INDEX_SEASON 2
#define _INDEX_STRING 3
#define _INDEX_EXIT 4

/****************************************************************
* 
* 	Typedef
* 
****************************************************************/
typedef struct
{
	int xPos;				   /* 左上角 X 的坐标 */
	int yPos;				   /* 左上角 Y 的坐标 */
	int xSize;				   /* X 轴的长度 */
	int ySize;				   /* Y 轴的长度 */
	int Index;				   /* 索引号 */
	const char **apTextSingle; /* 子菜单选项 */
	const char *apTextName;	/* 选项的名称 */
	int menunum;			   /* 菜单数量 */
} APRA;

static const char *_acText1_Single[] = {"Enet", "Home"};
static const char *_acText2_Single[] = {"Spring", "Summer", "Autumn", "Winter"};
static const char *_acText3_Single[] = {"About", "Help"};
static const char *_acText4_Single[] = {"Get"};

static APRA slect_button[] = {
	{80, 0, 80, 40, _INDEX_PAGE, _acText1_Single, "Page", 2},
	{160, 0, 80, 40, _INDEX_SEASON, _acText2_Single, "Season", 4},
	{240, 0, 80, 40, _INDEX_STRING, _acText3_Single, "Bin", 2},
	{320, 0, 80, 40, _INDEX_EXIT, _acText4_Single, "Exit", 1}};

static APRA *pslect_button[GUI_COUNTOF(slect_button)];

static const unsigned char *bgArray[] = {
	_acbg_480x272,
	_acspring,
	_acsummer,
	_acautumn,
	_acwinter};

/****************************************************************
* 
*  Variable
* 
****************************************************************/
static WM_HWIN hWin_BarWindow;
static WM_HWIN hWin_BgWindow;
static WM_HWIN hBUtton_Touch;
static WM_HWIN hWin_TextWindow;
static WM_HWIN _hWinBarMenu;

static char *textwindow_IP = "Server IP : ";
static char *textwindow_PORT = "Server PORT :";
static char *textwidnow_SEASON = "Send '0x80' + Key can change background";

static int _IndexBarMenu;
static int _IndexBackground;
static int Flag_EnetActivity = 0;
static int Queue_Mode;

int ENETACTIVITY_Socket_Flag = 0;

/****************************************************************
* 
* 	Function declaration
* 
****************************************************************/
static void _DeleteMenuWidnow(void);
static void _ApendMenuWindow(GUI_HWIN hWin, APRA *_aArpa);
static void _CreatePage1(void);

/*******************************************************************************
*
* 	Static Code
*
*****************************************************************************/
static void vEmwinDataInit(void)
{
	ucGetip_Flag = 1;
	textwindow_IP = (char *)malloc(sizeof(char) * 30);
	textwindow_PORT = (char *)malloc(sizeof(char) * 30);
	memcpy(textwindow_IP, 0, sizeof(textwindow_IP));
	memcpy(textwindow_PORT, 0, sizeof(textwindow_PORT));
	strcpy(textwindow_IP, "Server IP :");
	strcpy(textwindow_PORT, "Server PORT :");
}

static char *cpEmwinCpyServerAddr(char *ipaddress, char *port)
{
}

/**************************************************************************************
*
*	Static Callback function
*
**************************************************************************************/

/* 用于解决首次创建界面时出现 barmenu 自动创建的问题 */
static int enetactivity_lockvalue = 1;
static void enetactivity_lock(void)
{
	enetactivity_lockvalue = 1;
}

static void enetactivity_unlock(void)
{
	enetactivity_lockvalue = 0;
}

static int enetactivity_judge(void)
{
	if(enetactivity_lockvalue)
	{
		/* lock */
		return 1;
	}
	else if(enetactivity_lockvalue == 0)
	{
		return 0;
	}
}





/* CallBack function on button in bar menu */
static void _cbButtonMenu(WM_MESSAGE *pMsg)
{
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		if (BUTTON_IsPressed(pMsg->hWin))
		{
			GUI_SetColor(0x90ul << 24 | GUI_WHITE);
			GUI_FillRect(0, 0, 160, 40);
		}
		break;
	default:
		BUTTON_Callback(pMsg);
	}
}

/* CallBack function on baackground window */
static void _cbbgwindow(WM_MESSAGE *pMsg)
{
	int Id, NCode;
	ip_addr_t dhcp_ip;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_JPEG_Draw(bgArray[_IndexBackground], JPEGRAM_USE, 0, 0);
		GUI_SetBkColor(0xB0ul << 24 | GUI_BLACK);
		GUI_Clear();
		break;
	case WM_NOTIFY_PARENT:
		NCode = pMsg->Data.v;
		switch (NCode)
		{
		case WM_NOTIFICATION_CLICKED:
			break;
		case WM_NOTIFICATION_RELEASED:
			break;
		}
		break;
	default:
		WM_DefaultProc(pMsg);
	}
}

/* CallBack function on bar menu */
static void _cbBarMenu(WM_MESSAGE *pMsg)
{
	static WM_HTIMER hTimer;
	GUI_HWIN hWin = pMsg->hWin;
	APRA *_aApra = NULL;
	int Id, NCode;
	/* 首次创建 EnetActivity 时会出现自启动现象，暂不知具体原因,故加上一个判断函数 */
	if(enetactivity_judge())
	{
		return;
	}
	switch (pMsg->MsgId)
	{
	case WM_CREATE:
		hTimer = WM_CreateTimer(hWin, 0, 3000, 0);
		break;
	case WM_TIMER:
		if (hTimer)
		{
			WM_DeleteTimer(hTimer);
			hTimer = 0;
		}
		if (hWin != _hWinBarMenu)
		{
			return;
		}
		_DeleteMenuWidnow();
		break;
	case WM_PAINT:
		WM_GetUserData(hWin, &_aApra, sizeof(_aApra));
		_ApendMenuWindow(hWin, _aApra);
		break;
	case WM_NOTIFY_PARENT:
		NCode = pMsg->Data.v;
		Id = WM_GetId(pMsg->hWinSrc);
		WM_GetUserData(hWin, &_aApra, sizeof(_aApra));
		switch (NCode)
		{
		case WM_NOTIFICATION_RELEASED:
			_DeleteMenuWidnow();
			switch (_aApra->Index)
			{
			case _INDEX_PAGE:
				switch (Id)
				{
				case ID_BUTTON_0:
					break;
				case ID_BUTTON_0 + 1:
					_DeleteMenuWidnow();
					HideEnetActivity();
					CreateHomeActivity();
					break;
				}
				WM_InvalidateWindow(hWin_TextWindow);
				break;
			case _INDEX_SEASON:
				switch (Id)
				{
				case ID_BUTTON_0:
					_IndexBackground = 1;
					break;
				case ID_BUTTON_0 + 1:
					_IndexBackground = 2;
					break;
				case ID_BUTTON_0 + 2:
					_IndexBackground = 3;
					break;
				case ID_BUTTON_0 + 3:
					_IndexBackground = 4;
					break;
				}
				WM_InvalidateWindow(hWin_TextWindow);
				WM_InvalidateWindow(hWin_BgWindow);
				break;
			case _INDEX_STRING:
				switch (Id)
				{
				case ID_BUTTON_0:
					break;
				case ID_BUTTON_0 + 1:
					break;
				}
				WM_InvalidateWindow(hWin_TextWindow);
				break;
			case _INDEX_EXIT:
//				WM_InvalidateWindow(hWin_TextWindow);
				break;
			}
			break;
		case WM_NOTIFICATION_CLICKED:

			break;
		default:
			break;
		}
		break;
	default:
		WM_DefaultProc(pMsg);
	}
}

/* CallBack function on bar window */
static void _cbbarwidnow(WM_MESSAGE *pMsg)
{
	int NCode, Id, Index, FontSizeY;
	int xPos, yPos, xSize, ySize;
	WM_HWIN hWin;
	APRA *_aApra = NULL;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_SetBkColor(0xE0 << 24 | GUI_WHITE);
		GUI_Clear();
		break;
	case WM_NOTIFY_PARENT:
	{
		NCode = pMsg->Data.v;
		hWin = pMsg->hWinSrc;
		Id = WM_GetId(hWin);
		Index = Id - GUI_ID_BUTTON0;
		WM_GetUserData(hWin, &_aApra, sizeof(_aApra));
		switch (NCode)
		{
		case WM_NOTIFICATION_CLICKED:
			enetactivity_unlock();
			if ((_IndexBarMenu != (int)Index) && (_IndexBarMenu < (int)GUI_COUNTOF(slect_button)))
			{
				if (_hWinBarMenu)
				{
					/* Delete the menu window on click other widget */
					_DeleteMenuWidnow();
				}
				if (3 == Index)
				{
					HideEnetActivity();
					CreateHomeActivity();
					return;
				}
				GUI_SetFont(&FONT_BARMENU);
				FontSizeY = GUI_GetFontDistY();
				xPos = _aApra->xPos;
				yPos = _aApra->yPos + _aApra->ySize;
				xSize = _aApra->xSize;
				ySize = (FontSizeY + 10) * _aApra->menunum;
				_hWinBarMenu = WM_CreateWindowAsChild(xPos, yPos, xSize, ySize, WM_HBKWIN, WM_CF_SHOW | WM_CF_MOTION_Y | WM_CF_HASTRANS, _cbBarMenu, sizeof(APRA *));
				_IndexBarMenu = Index;
				WM_SetUserData(_hWinBarMenu, &pslect_button[Index], sizeof(_aApra));
			}
			break;
		case WM_NOTIFICATION_RELEASED:
			break;
		}
	}
	break;
	default:
		WM_DefaultProc(pMsg);
	}
}

/* CallBack function on button in bar window */
static void _cbSelButton(WM_MESSAGE *pMsg)
{
	APRA *_aApra = NULL;
	switch (pMsg->MsgId)
	{
	/* 会出现闪屏的现象,暂未解决 */
	case WM_PAINT:
		WM_GetUserData(pMsg->hWin, &_aApra, sizeof(_aApra));
		if (BUTTON_IsPressed(pMsg->hWin))
		{
			GUI_SetColor(0x30ul << 24 | GUI_WHITE);
			GUI_FillRect(0, 0, 80, 40);
		}
		else
		{
			GUI_SetColor(0x90ul << 24 | GUI_WHITE);
			GUI_DrawLine(0, 4, 0, 36);
			GUI_DrawLine(79, 4, 79, 36);
			GUI_SetFont(&GUI_Font20_ASCII);
			GUI_SetTextMode(GUI_TEXTMODE_TRANS);
			GUI_DispStringHCenterAt(_aApra->apTextName, 40, 10);
		}
		break;
	default:
		BUTTON_Callback(pMsg);
	}
}

static void _cbgetButton(WM_MESSAGE *pMsg)
{
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		if (BUTTON_IsPressed(pMsg->hWin))
		{
			GUI_SetColor(0x30ul << 24 | GUI_WHITE);
			GUI_AA_FillRoundedRect(0, 0, 160, 40, 15);
			GUI_SetColor(0x30ul << 24 | GUI_WHITE);
			GUI_SetFont(&GUI_Font20_ASCII);
			GUI_SetTextMode(GUI_TEXTMODE_TRANS);
			GUI_DispStringHCenterAt("Touch", 80, 10);
		}
		else
		{
			GUI_SetColor(0x30ul << 24 | GUI_LIGHTRED);
			GUI_AA_FillRoundedRect(0, 0, 160, 40, 15);
			GUI_SetColor(0x30ul << 24 | GUI_WHITE);
			GUI_SetFont(&GUI_Font20_ASCII);
			GUI_SetTextMode(GUI_TEXTMODE_TRANS);
			GUI_DispStringHCenterAt("Touch", 80, 10);
		}
		break;
	default:
		BUTTON_Callback(pMsg);
	}
}

static void _cbtextwindow(WM_MESSAGE *pMsg)
{
	int FontSizeY;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		switch (Queue_Mode)
		{
		case _INDEX_PAGE:
			GUI_SetFont(&GUI_Font20_1);
			GUI_SetTextMode(GUI_TEXTMODE_TRANS);
			GUI_DispStringAt(textwindow_IP, 20, 0);
			FontSizeY = GUI_GetFontSizeY();
			GUI_DispStringAt(textwindow_PORT, 20, 0 + FontSizeY);
			break;
		case _INDEX_SEASON:
			GUI_SetFont(&GUI_Font20_1);
			GUI_SetTextMode(GUI_TEXTMODE_TRANS);
			GUI_DispStringAt(textwidnow_SEASON, 20, 0);
			break;
		case _INDEX_STRING:
			break;
		case _INDEX_EXIT:
			break;
		}
	default:
		WM_DefaultProc(pMsg);
	}
}

static void _CreateWindow(void)
{
	/* Create Background Window */
	hWin_BgWindow = WM_CreateWindow(0, 0, 480, 272, WM_CF_SHOW, _cbbgwindow, 0);

	/* Create bar window */
	hWin_BarWindow = WM_CreateWindowAsChild(0, 0, 480, 40, hWin_BgWindow, WM_CF_SHOW | WM_CF_HASTRANS, _cbbarwidnow, 0);
	WM_CreateWindow(0, 60, 42, 42, WM_CF_MOTION_Y | WM_CF_HASTRANS, _cbBarMenu, 0);

	/* Create Text window*/
	hWin_TextWindow = WM_CreateWindowAsChild(0, 220, 480, 40, hWin_BgWindow, WM_CF_SHOW | WM_CF_HASTRANS, _cbtextwindow, 0);
}

static void _CreatButton(void)
{
	char i;
	APRA *_aApra = NULL;
	WM_HWIN hWin_Button;
	/* Selcet button in bar window*/
	for (i = 0; i < 4; i++)
	{
		hWin_Button = BUTTON_CreateUser(slect_button[i].xPos, slect_button[i].yPos, slect_button[i].xSize,
										slect_button[i].ySize, hWin_BarWindow, WM_CF_SHOW | WM_CF_HASTRANS, 0, GUI_ID_BUTTON0 + i, sizeof(APRA *));
		WM_SetCallback(hWin_Button, _cbSelButton);
		_aApra = pslect_button[i];
		WM_SetUserData(hWin_Button, &_aApra, sizeof(APRA *));
	}
	/* Create Touch button */
	hBUtton_Touch = BUTTON_CreateUser(160, 150, 160, 40, hWin_BgWindow, WM_CF_SHOW | WM_CF_HASTRANS, 0, GUI_ID_USER + 0x00, 0);
	WM_SetCallback(hBUtton_Touch, _cbgetButton);
}

static void _ApendMenuWindow(GUI_HWIN hWin, APRA *_aArpa)
{

	int i;
	int FontSizeY;
	int y1Size;
	GUI_HWIN hButton;
	GUI_SetBkColor(0xAA000000 | GUI_BLACK);
	GUI_Clear();
	GUI_SetColor(0x88000000 | GUI_WHITE);
	GUI_SetFont(&FONT_BARMENU);
	GUI_SetTextMode(GUI_TM_TRANS);
	y1Size = GUI_GetFontDistY() + 10;
	for (i = 0; i < _aArpa->menunum; i++)
	{
		GUI_DispStringHCenterAt(_aArpa->apTextSingle[i], _aArpa->xSize / 2, i * y1Size + 5);
		hButton = BUTTON_CreateAsChild(0, i * y1Size, _aArpa->xSize, y1Size, hWin, ID_BUTTON_0 + i, WM_CF_SHOW | WM_CF_HASTRANS);
		WM_SetCallback(hButton, _cbButtonMenu);
	}
}

static void _DeleteMenuWidnow(void)
{
	if (_hWinBarMenu == 0)
	{
		return;
	}
	//	GUI_SOFTLAYER_Refresh();
	GUI_MEMDEV_FadeOutWindow(_hWinBarMenu, 100);
	WM_DeleteWindow(_hWinBarMenu);
	_hWinBarMenu = 0;
	_IndexBarMenu = -1;
}

/*********************************************************************************
*
*		Can be call function
*
********************************************************************************/

void CreateEnetActivity(void)
{
	int i;
	for (i = 0; i < GUI_COUNTOF(slect_button); i++)
	{
		pslect_button[i] = &slect_button[i];
	}
	if (Flag_EnetActivity == 1)
	{
		WM_ShowWindow(hWin_BgWindow);
		return;
	}
	_IndexBarMenu = -1;
	_IndexBackground = 0;
	vEmwinDataInit();
	_CreateWindow();
	_CreatButton();
	Flag_EnetActivity = 1;
}

void HideEnetActivity(void)
{
	if (Flag_EnetActivity == 0)
	{
		return;
	}
	WM_HideWindow(hWin_BgWindow);
}

void DeleteEnetActivity(void)
{
	if (Flag_EnetActivity == 0)
	{
		return;
	}
	WM_DeleteWindow(hWin_BgWindow);
}