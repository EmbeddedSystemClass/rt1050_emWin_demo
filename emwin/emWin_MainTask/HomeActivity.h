#ifndef  __HOMEACTIVITY_H_
#define  __HOMEACTIVITY_H_


#define		ENETSTATUS_LOADING		1
#define   	ENETSTATUS_ACCESS		0


void CreateHomeActivity(void);
void HideHomeActivity(void);
void DeleteHomeActivity(void);
void HomeActivity_EnetStatus(int flag);

#endif
