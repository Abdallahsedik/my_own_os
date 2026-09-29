/*
 * Scheduler.h
 *
 *  Created on: Sep 27, 2026
 *      Author: pc
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_
#include "Cortexmx_OsPorting.h"


typedef enum{
	NoError                          = 0,
	Ready_Queue_init_error           = 1,
	Task_exceeded_StackSize          = 2,
	MutexisReacedToMaxNumberOfUsers  = 4,
	Task_Null_Pointer                = 8,
	Task_Invalid_State               = 16,
	Task_Not_Current                 = 32
}MYRTOS_errorID;


typedef struct{
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

#define element_type Task_ref*

MYRTOS_errorID MYRTOS_Init();
MYRTOS_errorID MYRTOS_Create_task(Task_ref * TRef);
MYRTOS_errorID Activate_task(Task_ref * TRef);
MYRTOS_errorID Terminate_task(void);
MYRTOS_errorID Start_OS(void);

void Decide_whatNext(void);


#endif /* INC_SCHEDULER_H_ */
