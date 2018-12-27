/*********************************************************************
*                SEGGER MICROCONTROLLER SYSTEME GmbH                 *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2011  SEGGER Microcontroller Systeme GmbH        *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

***** emWin - Graphical user interface for embedded applications *****
emWin is protected by international copyright laws.   Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with a license and should not be re-
distributed in any way. We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : MOVIE_ShowFromFS.c
Purpose     : Shows how to play a movie directly from a file system.
Requirements: WindowManager - ( )
			  MemoryDevices - ( )
			  AntiAliasing  - ( )
			  VNC-Server    - ( )
			  PNG-Library   - ( )
			  TrueTypeFonts - ( )

			  Requires either a MS Windows environment or emFile.
----------------------------------------------------------------------
*/

#ifndef SKIP_TEST

#include "GUI.h"
#include "Output.c"

int _Play;
/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
//
// Recommended memory to run the sample with adequate performance
//

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _cbNotify
*
* Function description
*   Callback function for movie player. Uses multiple buffering if
*   available to avoid tearing effects.
*/
void _cbNotify(GUI_HMEM hMem, int Notification, U32 CurrentFrame) {
	switch (Notification) {
	case GUI_MOVIE_NOTIFICATION_PREDRAW:
		GUI_MULTIBUF_Begin();
		break;
	case GUI_MOVIE_NOTIFICATION_POSTDRAW:
		GUI_MULTIBUF_End();
		break;
	case GUI_MOVIE_NOTIFICATION_START:
		_Play = 1;
		break;
	case GUI_MOVIE_NOTIFICATION_STOP:
		_Play = 0;
		break;
	}
}

/*********************************************************************
*
*       _GetData
*
* Function description
*   Reading data directly from file system
*/


/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       _DrawSmilie
*/
static void _DrawSmilie(int xPos, int yPos, int r) {
	int d;

	GUI_SetColor(GUI_YELLOW);
	GUI_FillCircle(xPos, yPos, r);
	GUI_SetColor(GUI_BLACK);
	GUI_SetPenSize(1);
	GUI_DrawCircle(xPos, yPos, r);
	d = (r * 2) / 5;
	GUI_FillCircle(xPos - d, yPos - d, r / 10);
	GUI_FillCircle(xPos + d, yPos - d, r / 10);
	GUI_DrawVLine(xPos, yPos - d / 2, yPos + d / 2);
	GUI_DrawArc(xPos, yPos + r + d, r, r, 70, 110);
}

/*********************************************************************
*
*       MainTask
*/
void MainTask(void) {
	GUI_MOVIE_INFO   Info;
	GUI_MOVIE_HANDLE hMovie;
	int              xSize, ySize;
	GUI_RECT         Rect;
	int              FontDistY;
	GUI_Init();
	//
	// Check if recommended memory for the sample is available
	//

	//
	// Get display size
	//
	xSize = LCD_GetXSize();
	ySize = LCD_GetYSize();
	//
	// Create file handle
	//

	//
	// Get physical size of movie
	//
	if (GUI_MOVIE_GetInfo(_ac_480,sizeof(_ac_480), &Info) == 0) {
		//
		// Check if display size fits
		//
		if ((Info.xSize <= xSize) && (Info.ySize <= ySize)) {
			//
			// Create and play movie
			//
			hMovie = GUI_MOVIE_Create(_ac_480, sizeof(_ac_480), _cbNotify);
			if (hMovie) {
				GUI_MOVIE_Show(hMovie, (xSize - Info.xSize) / 2, (ySize - Info.ySize) / 2, 0);
				do {
					GUI_Exec();
				} while (_Play);
			}
		}
		else {
			//
			// Error message
			//
			GUI_SetFont(GUI_FONT_13_ASCII);
			GUI_DispStringHCenterAt("Video can not be shown.\n\nDisplay size too small.", xSize / 2, (ySize - GUI_GetFontSizeY()) / 2);
		}
	}
	while (1) {
		GUI_Exec();
		GUI_Delay(1);
	}
}

#endif

/*************************** End of file ****************************/
