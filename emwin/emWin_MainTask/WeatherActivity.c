#include "SocketActivity.h"
#include "DIALOG.h"
#include "weather.h"
#include "Dandelion.c"
#include "cJSON.h"
#include "weather_icon/weather_icon.h"

/******************************************** Define ?? **************************************/
#define HEIGHT	232
#define WIDTH	480

#define ID_WINDOW_01		(GUI_ID_USER + 0x00)
#define ID_BUTTON_UPDATE	(GUI_ID_USER + 0x10)

#define SUNNY						0
#define CLEAR						1
#define FAIR						2
#define FAIR_N						3
#define CLOUDY						4
#define PARTLY_CLOUDY				5
#define PARTLY_CLOUDY_N				6
#define MOSTLY_CLOUDY				7
#define MOSTLY_CLOUDY_N				8
#define OVERCAST					9
#define SHOWER						10
#define THUNDERSHOWER				11
#define THUNDERSHOWERWITHHAIL		12
#define LIGHTRAIN					13
#define MODERATERAIN				14
#define HEAVYRAIN					15
#define STORM						16
#define UNKNOWN						17


/******************************************** ???? *****************************************/

extern GUI_HWIN hWin_Background;

extern GUI_CONST_STORAGE GUI_BITMAP bmChanceOfStorm_40x40;
extern GUI_CONST_STORAGE GUI_BITMAP bmCloudLighting_40x40;
extern GUI_CONST_STORAGE GUI_BITMAP bmPartlyCloudyDay_40x40;
extern GUI_CONST_STORAGE GUI_BITMAP bmRain_40x40;
extern GUI_CONST_STORAGE GUI_BITMAP bmSun_40x40;

/********************************************* ???? ****************************************/

static GUI_HWIN hWin_Weather = 0;

static int weacode = UNKNOWN;

static cJSON *json = NULL;
Weather_data *Weather = NULL;


static const unsigned char *Weather_Icon[] = {
	_ac0,
	_ac1,
	_ac2,
	_ac3,
	_ac4,
	_ac5,
	_ac6,
	_ac7,
	_ac8,
	_ac9,
	_ac10,
	_ac11,
	_ac12,
	_ac13,
	_ac14,
	_ac15,
	_ac16,
	_ac99,
};

static const unsigned char *Weather_Icon_180x180[] = {
	_ac0_180x180,
	_ac1_180x180,
	_ac2_180x180,
	_ac3_180x180,
	_ac4_180x180,
	_ac5_180x180,
	_ac6_180x180,
	_ac7_180x180,
	_ac8_180x180,
	_ac9_180x180,
	_ac10_180x180,
	_ac11_180x180,
	_ac12_180x180,
	_ac13_180x180,
	_ac14_180x180,
	_ac15_180x180,
	_ac16_180x180,
	_ac99_180x180,
};
/******************************************** ???? ***************************************/

static void _cbWindow_WeatherActivity(WM_MESSAGE *pMsg);
static void _cbButton_Update(WM_MESSAGE *pMsg);

static void _CreateActivityWeather(void);





/******************************************** Static ?? ***************************************/

static void _cbWindow_WeatherActivity(WM_MESSAGE *pMsg)
{
	int NCode, Id;
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		GUI_SetBkColor(GUI_WHITE);
		GUI_Clear();
		GUI_SetTextMode(GUI_TEXTMODE_TRANS);
		GUI_SetColor(GUI_BLUE);
		GUI_JPEG_Draw(_acDanblue , 1024 * 50, 0, 0);
		GUI_SetColor(0x66000000);
		GUI_FillRect(0, 130, 480, 210);
		GUI_PNG_Draw(Weather_Icon_180x180[Weather[0].code_day], 1024 * 200, 100, 0);
		GUI_PNG_Draw(Weather_Icon[Weather[1].code_day], 1024 * 200, 40, 140);
		GUI_PNG_Draw(Weather_Icon[Weather[2].code_day], 1024 * 200, 220, 140);
		GUI_SetTextMode(GUI_TEXTMODE_TRANS);
		GUI_SetColor(GUI_WHITE);
		GUI_SetFont(&GUI_Font16_1);
		GUI_DispStringHCenterAt(Weather[0].city, 310, 30);
		GUI_DispStringHCenterAt(Weather[0].temp_low, 290, 60);
		GUI_DispStringHCenterAt("~", 310, 60);
		GUI_DispStringHCenterAt(Weather[0].temp_high, 330, 60);
		GUI_DispStringHCenterAt(Weather[0].date, 310, 90);
		GUI_DispStringHCenterAt("`C", 345, 60);
		
		GUI_DispStringHCenterAt(Weather[1].date, 70, 190);
		GUI_DispStringHCenterAt(Weather[2].date, 250, 190);

		//GUI_DispStringHCenterAt(Weather.StringWeather, 20, 60);
		break;
	case WM_NOTIFY_PARENT:
		NCode = pMsg->Data.v;
		Id = WM_GetId(pMsg->hWinSrc);
		switch (NCode)
		{
		case WM_NOTIFICATION_RELEASED:
			json = ReadPage("api.thinkpage.cn");
			Weather = GetJSONWeather(json);
			WM_InvalidateWindow(hWin_Weather);
			break;
		}
		break;
	default:
		WM_DefaultProc(pMsg);
	}
}

static void _cbButton_Update(WM_MESSAGE *pMsg)
{
	switch (pMsg->MsgId)
	{
	case WM_PAINT:
		break;
	default:
		BUTTON_Callback(pMsg);
		break;
	}
}

static void _CreateActivityWeather(void)
{
	GUI_HWIN hButton;
	hWin_Weather = WINDOW_CreateUser(0, 0, WIDTH, HEIGHT, hWin_Background, WM_CF_SHOW | WM_CF_HASTRANS, 0, ID_WINDOW_01, _cbWindow_WeatherActivity, 0);
	hButton = BUTTON_CreateUser(0, 160, 480, 50, hWin_Weather, WM_CF_SHOW | WM_CF_HASTRANS, 0, ID_BUTTON_UPDATE, 0);
	WM_SetCallback(hButton, _cbButton_Update);
	json = ReadPage("api.thinkpage.cn");
	Weather = GetJSONWeather(json);
	//Weather.StringCity = (char *)malloc(10);
	//Weather.StringTemp = (char *)malloc(2);
	//GUI__memset(Weather.StringCity, 0, 10);
	//GUI__memset(Weather.StringTemp, 0, 2);

}


/*********************************************** ??????? **********************************/
void CreateActivityWeather(void)
{
	if (0 != hWin_Weather)
	{
		WM_ShowWindow(hWin_Weather);
		return;
	}
	_CreateActivityWeather();
}

void DeleteActivityWeather(void)
{
	if (0 == hWin_Weather)
	{
		return;
	}
	WM_DeleteWindow(hWin_Weather);
	hWin_Weather = 0;
}

void HideActivityWeather(void)
{
	WM_HideWindow(hWin_Weather);
}