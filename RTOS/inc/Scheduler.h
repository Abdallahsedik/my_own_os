/*
 * Scheduler.h
 *
 *  Created on: Sep 27, 2026
 *      Author: pc
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_
#include "Cortexmx_OsPorting.h"
#include "Os_Types.h"

#define MAX_TASKS   16 /*max number of tasks*/



typedef struct Task_ref{
	TaskType  TaskID;
	uint32 Stack_Size;
	uint8 priority;
	void (*p_TaskEntry)(void); //pointer to Tack C Function
	uint8 AutoStart ;
	uint32 _S_PSP_Task ;//Not Entered by the user
	uint32 _E_PSP_Task ;//Not Entered by the user
	uint32* Current_PSP ;//Not Entered by the user
	char TaskName[30] ;
	TaskStateType TaskState;	//Not Entered by the user
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



StatusType MYRTOS_Init();
StatusType MYRTOS_Create_task(Task_ref * TRef);
//StatusType Activate_task(Task_ref * TRef);
StatusType ActivateTask(TaskType);
StatusType TerminateTask(void);
StatusType Start_OS(void);
StatusType TaskWait(TaskType TaskID, uint32 NoTicks);
StatusType Os_Delay(uint32 NoTicks);



void MYRTOS_Update_TasksWaitingTime(void);
StatusType MyRTOS_GetTaskState(Task_ref* TRef, uint8* State);
StatusType GetResource(Resource_t* Res);
StatusType ReleaseResource(Resource_t* Res);
void ShutdownOS(StatusType Error);

void MyRTOS_StackOverflowHook(Task_ref* FaultyTask);



void Decide_whatNext(void);


#endif /* INC_SCHEDULER_H_ */
