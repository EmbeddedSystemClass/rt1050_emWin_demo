#ifndef  _WEATHER_H_
#define	 _WEATHER_H_
#include "cJSON.h"


typedef struct {
	int code_day;
	int code_night;
	char temp_high[4];
	char temp_low[4];
	char date[12];
	char weather_day[11];
	char city[12];
}Weather_data;


cJSON *ReadPage(char* host);
//char *GetJSONCity(cJSON *json);
//int GetJSONWeather(cJSON *json);
Weather_data *GetJSONWeather(cJSON *json);


#endif // ! _WEATHER_H__
