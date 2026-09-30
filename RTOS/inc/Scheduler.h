/*
 * Scheduler.h
 *
 *  Created on: Sep 27, 2026
 *      Author: pc
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_
#include "Cortexmx_OsPorting.h"

#define MAX_TASKS   16 /*max number of tasks*/

typedef enum{
	NoError                          = 0,
	Ready_Queue_init_error           = 1,
	Task_exceeded_StackSize          = 2,
	MutexisReacedToMaxNumberOfUsers  = 4,
	Task_Null_Pointer                = 8,
	Task_Invalid_State               = 16,
	Task_Not_Current                 = 32,
	Task_Limit_Exceeded				 =64
}MYRTOS_errorID;


typedef struct Task_ref{
	uint32 Stack_Size;
	uint8 priority;
	void (*p_TaskEntry)(void); //pointer to Tack C Function
	uint8 AutoStart ;
	uint32 _S_PSP_Task ;//Not Entered by the user
	uint32 _E_PSP_Task ;//Not Entered by the user
	uint32* Current_PSP ;//Not Entered by the user
	char TaskName[30] ;
	enum{
		Suspend,
		Running,
		Waiting,
		ready
	}TaskState	;//Not Entered by the user

	struct{
		enum{
			Enable,
			Disable
		}Blocking;
		uint32 Ticks_Count ;
	}TimingWaiting;
}Task_ref;

typedef struct {
	Task_ref* OSTasks[MAX_TASKS]; //Sch. Table
	uint32 _S_MSP_Task ;
	uint32 _E_MSP_Task ;
	uint32 PSP_Task_Locator ;
	uint32 NoOfActiveTasks ;
	Task_ref* CurrentTask ;
	Task_ref* NextTask ;
	enum{
		OSsuspend,
		OsRunning
	}OSmodeID;
}OS_Control_t;
extern OS_Control_t OS_Control;

typedef struct{
	uint8 CeilingPriority;     /*you need to define/calculate  Ceiling Priority before run os  */
	Task_ref* Owner;
	uint8 PreviousPriority;   /*owner's original periority without celing */
}Resource_t;




MYRTOS_errorID MYRTOS_Init();
MYRTOS_errorID MYRTOS_Create_task(Task_ref * TRef);
MYRTOS_errorID Activate_task(Task_ref * TRef);
MYRTOS_errorID Terminate_task(void);
MYRTOS_errorID Start_OS(void);
void MYRTOS_Update_TasksWaitingTime(void);
void MYRTOS_TaskWait(unsigned int NoTICKS,Task_ref* SelfTref);
void Decide_whatNext(void);
MYRTOS_errorID MyRTOS_GetTaskState(Task_ref* TRef, uint8* State);
MYRTOS_errorID GetResource(Resource_t* Res);
MYRTOS_errorID ReleaseResource(Resource_t* Res);
MYRTOS_errorID Activate_task_FromISR(Task_ref* TRef);

void MyRTOS_StackOverflowHook(Task_ref* FaultyTask);



#endif /* INC_SCHEDULER_H_ */
