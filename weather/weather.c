#include "stdio.h"
#include "stdlib.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "cJSON.h"
#include "ip_addr.h"
#include "../lwip/src/include/lwip/dns.h"
#include "../lwip/src/include/lwip/api.h"
#include "../lwip/src/include/lwip/sockets.h"
#include "weather.h"
//#pragma comment(lib, "ws2_32.lib")
extern void dns_init(void);

char *GetJSONdata(char * buffer, int bufsize);


cJSON *ReadPage(char* host)
{
	cJSON *json = NULL;
	int i;
	char *buf_1;
	char request[1024] = "GET /v3/weather/daily.json?key=0fpi3okdsa2oqlhs&location=shenzhen&language=en&unit=c&start=0&days=5 HTTP/1.1\r\nHost:";
	int sock;
	struct sockaddr_in si;
	const int bufsize = 2018;
	si.sin_family = AF_INET;
	si.sin_port = htons(80);
  si.sin_addr.s_addr = inet_addr("114.55.186.19");
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	connect(sock, (struct sockaddr *)&si, sizeof(si));
	if (sock == -1 || sock == -2)
		return 0;
	strcat(request, host);
	strcat(request, "\r\nConnection:Close\r\n\r\n");
	int ret = send(sock, request, strlen(request), 0);

	char* buf = (char *)calloc(bufsize, 1);
	ret = recv(sock, buf, bufsize - 1, 0);
	buf_1 = GetJSONdata(buf, bufsize);
	json = cJSON_Parse(buf_1);
	free(buf);
	free(buf_1);
	close(sock);
	return json;
}

char *GetJSONdata(char * buffer, int bufsize)
{
	int json_start = 0, json_end = 0, json_len = 0;
	int i = 0;
	char *json_buffer = NULL;
	for (i = 0; i < bufsize; i++)
	{
		if (buffer[i] == '{')
		{
			json_start = i;
			break;
		}
	}
	for (i = 0; i < bufsize; i++)
	{
		if ('}' == buffer[i])
		{
			json_end = i;
		}
	}
	json_len = json_end - json_start + 1;
	json_buffer = (char *)calloc(json_len, 1);
	for (i = 0; i < json_len; i++)
	{
		json_buffer[i] = buffer[json_start + i];
	}
	return json_buffer;
}

Weather_data *GetJSONWeather(cJSON *json)
{
	Weather_data *Weather;
	Weather = (Weather_data *)malloc(sizeof(Weather_data) * 3);
	cJSON *json_tm;
	cJSON *json_tx;
	cJSON *json_daily;
	int i;
	if (json)
	{
		json_tm = cJSON_GetObjectItem(json, "results");
		if (json_tm)
		{
			json_tm = cJSON_GetArrayItem(json_tm, 0);
			if (json_tm)
			{
				json_tx = cJSON_GetObjectItem(json_tm, "location");
				if (json_tx)
				{
					json_tx = cJSON_GetObjectItem(json_tx, "name");
					if (json_tx)
					{
						memcpy(Weather[0].city, json_tx->valuestring,10);
						memcpy(Weather[1].city, json_tx->valuestring,10);
						memcpy(Weather[2].city, json_tx->valuestring,10);
					}
				}
				for (i = 0; i < 3; i++)
				{
					json_daily = cJSON_GetObjectItem(json_tm, "daily");
					if (json_daily)
					{
						json_daily = cJSON_GetArrayItem(json_daily, i);
						if (json_daily)
						{
							/* ??????? */
							json_tx = cJSON_GetObjectItem(json_daily, "date");
							if (json_tx)
							{
								memcpy(Weather[i].date, json_tx->valuestring, 10);
								Weather[i].date[10] = '\0';
							}
							/* ???? */
							json_tx = cJSON_GetObjectItem(json_daily, "text_day");
							if (json_tx)
							{
								memcpy(Weather[i].weather_day, json_tx->valuestring, 10);
							}
							/* ????????? */
							json_tx = cJSON_GetObjectItem(json_daily, "code_day");
							if (json_tx)
							{
								Weather[i].code_day = atoi(json_tx->valuestring);
							}
							/* ????????? */
							json_tx = cJSON_GetObjectItem(json_daily, "code_night");
							if (json_tx)
							{
								Weather[i].code_night = atoi(json_tx->valuestring);
							}
							/* ????????? */
							json_tx = cJSON_GetObjectItem(json_daily, "high");
							if (json_tx)
							{
								memcpy(Weather[i].temp_high, json_tx->valuestring, 4);
							}
							/* ????????? */
							json_tx = cJSON_GetObjectItem(json_daily, "low");
							if (json_tx)
							{
								memcpy(Weather[i].temp_low, json_tx->valuestring, 4);
							}
						}
					}
				}
			}
		}
	}
	free(json_tm);
	free(json_tx);
	return Weather;
}


