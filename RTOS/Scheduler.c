/*
 * Scheduler.c
 *
 *  Created on: Sep 27, 2026
 *      Author: pc
 */


#include "Scheduler.h"
#include "MyRtos_FIFO.h"


OS_Control_t OS_Control;

typedef enum {
	SVC_Activatetask,
	SVC_terminateTask,
	SVC_TaskWaitingTime,
	SVC_AquireMutex,
	SVC_ReleaseMutex
}SVC_ID;

FIFO_Buf_t READY_QUEUE ;
Task_ref * READY_QUEUE_FIFO[MAX_TASKS];

Task_ref IDLE_Task_Instance;
Task_ref* IDLE_Task = &IDLE_Task_Instance;
uint8 Idle_task_LED=0;
void MyRtos_IDLE_Task(void)
{
	while(1)
	{
		__asm("wfe");
		Idle_task_LED^=1;

	}
}
MYRTOS_errorID MYRTOS_Create_MainStack()
{
	MYRTOS_errorID Error_Status=NoError;

	OS_Control._S_MSP_Task= ((uint32)&_estack);
	OS_Control._E_MSP_Task=(OS_Control._S_MSP_Task -MainStackSize);
	//allign 8 bytes space btw main task , psp tasks
	OS_Control.PSP_Task_Locator=(OS_Control._E_MSP_Task -8);

	if (OS_Control._E_MSP_Task < ((uint32)(&_end) + RESERVED_HEAP_STACK))
	{
		return Task_exceeded_StackSize;
	}
	else
	{

	}

	return Error_Status;


}

MYRTOS_errorID MyRTOS_Create_TaskStack(Task_ref* Tref)
{
	/*Task Frame
	 * ======
	 * XPSR
	 * PC (Next Task Instruction which should be Run)
	 * LR (return register which is saved in CPU while TASk1 running before TaskSwitching)
	 * r12
	 * r4
	 * r3
	 * r2
	 * r1
	 * r0
	 *====
	 *r5, r6 , r7 ,r8 ,r9, r10,r11 (Saved/Restore)Manual
	 */

	if(Tref == NULL_PTR)
	{
		return Task_Null_Pointer;
	}
	/*check if excedded of max number os tasks*/
	if (OS_Control.NoOfActiveTasks >= MAX_TASKS)
	{
			return Task_Limit_Exceeded;
	}
	Tref->Current_PSP =(uint32 *) Tref->_S_PSP_Task ;

	Tref->Current_PSP-- ;
	if(Tref->Current_PSP < (uint32*)Tref->_E_PSP_Task )
	{
		return Task_exceeded_StackSize;
	}
	else
	{
		*(Tref->Current_PSP) = 0x01000000;         //DUMMY_XPSR should T =1 to avoid BUS fault;//0x01000000
		Tref->Current_PSP-- ;
		if(Tref->Current_PSP < (uint32*)Tref->_E_PSP_Task)
		{
			return Task_exceeded_StackSize;

		}
		else
		{
			*(Tref->Current_PSP) = (unsigned int)Tref->p_TaskEntry ; //PC
			Tref->Current_PSP-- ; //LR = 0xFFFFFFFD (EXC_RETURN)Return to thread with PSP
			if(Tref->Current_PSP <  (uint32*)Tref->_E_PSP_Task)
			{
				return Task_exceeded_StackSize;

			}
			else
			{
				*(Tref->Current_PSP)  = 0xFFFFFFFD ;

			}

		}

	}




	for (int  j=0 ; j< 13 ; j++ )
	{
		Tref->Current_PSP-- ;
		if(Tref->Current_PSP < (uint32*)Tref->_E_PSP_Task)
		{
			return Task_exceeded_StackSize;
		}
		*(Tref->Current_PSP)  = 0 ;

	}

	OS_Control.OSTasks[OS_Control.NoOfActiveTasks] = Tref;
	OS_Control.NoOfActiveTasks++;


	return NoError;
}

MYRTOS_errorID MYRTOS_Create_task(Task_ref * TRef)
{

	MYRTOS_errorID Error_Status=NoError;
	/*create psp stack for the task
	 *chack stack size not exceeded the psp stack */
	TRef->_S_PSP_Task=OS_Control.PSP_Task_Locator;
	TRef->_E_PSP_Task= (TRef->_S_PSP_Task - TRef->Stack_Size);
	if (TRef->_E_PSP_Task < ((uint32)(&_end) + RESERVED_HEAP_STACK))
	{
		return Task_exceeded_StackSize;
	}
	/*alligned 8 bytes*/
	OS_Control.PSP_Task_Locator = (TRef->_E_PSP_Task - 8);
	/*Init psp task stack */
	Error_Status= MyRTOS_Create_TaskStack(TRef);

	/*update sch table and no of active tasks */
	//	OS_Control.OSTasks[OS_Control.NoOfActiveTasks]=TRef;
	//	OS_Control.NoOfActiveTasks++;
	/*task state update -> suspended */
	TRef->TaskState=Suspend;


	return Error_Status;

}
/*this function used when stack  overflow */
/*in nextversion  i will add terminate the task */
void MyRTOS_StackOverflowHook(Task_ref* FaultyTask)
{
    __disable_irq();

    /*save the fucdtion in variable */
    static Task_ref* LastOverflowedTask;
    LastOverflowedTask = FaultyTask;

    while(1);/*stuck in this point */
}

MYRTOS_errorID MYRTOS_Init()
{
	MYRTOS_errorID Error_Status=NoError;
	//update os mode
	OS_Control.OSmodeID=OSsuspend;

	//specify the main stack for os
	Error_Status=MYRTOS_Create_MainStack();
	//create os ready queue
	if(FIFO_init(&READY_QUEUE, READY_QUEUE_FIFO, MAX_TASKS)!= FIFO_NO_ERROR)
	{
		Error_Status+=Ready_Queue_init_error;

	}
	//cofig idle task

	strcpy(IDLE_Task->TaskName, "IdleTask");
	IDLE_Task->priority=255;
	IDLE_Task->p_TaskEntry=MyRtos_IDLE_Task;
	IDLE_Task->Stack_Size=300;

	Error_Status += MYRTOS_Create_task(IDLE_Task);
	return Error_Status;
}

void MYRTOS_OS_SVC_Set(SVC_ID svc_id)
{
	switch (svc_id) {
	case SVC_Activatetask:/*activate task*/
		__asm("svc #0x00");
		break;
	case SVC_terminateTask:/*terminate task*/
		__asm("svc #0x01");
		break;
	case SVC_TaskWaitingTime:/* task waiting time*/
		__asm("svc #0x02");
		break;
	case SVC_AquireMutex:/* task Aquire Mutex*/
		__asm("svc #0x03");
		break;
	case SVC_ReleaseMutex:/* task Release Mutex*/
		__asm("svc #0x04");
		break;




	}
}

MYRTOS_errorID Activate_task(Task_ref * TRef)
{
	/*  NULL check */
	if (TRef == NULL_PTR)
	{
		return Task_Null_Pointer;
	}

	/* check if task Suspended */
	if (TRef->TaskState != Suspend)
	{
		return Task_Invalid_State;
	}

	TRef->TaskState = ready;

	/*update sch. table
	 * svc interrupt/ handler (update ready queue)
	 * set pendsv (decide what next (dequeue) ,then switch context)
	 * */
	MYRTOS_OS_SVC_Set(SVC_Activatetask);

	return NoError;
}

MYRTOS_errorID Terminate_task(void)
{
	if (OS_Control.CurrentTask == NULL_PTR)
		return Task_Null_Pointer;

	OS_Control.CurrentTask->TaskState = Suspend;
	MYRTOS_OS_SVC_Set(SVC_terminateTask);
	return NoError;
}
void MYRTOS_TaskWait(unsigned int NoTICKS,Task_ref* SelfTref)
{
	SelfTref->TimingWaiting.Blocking = Enable ;
	SelfTref->TimingWaiting.Ticks_Count = NoTICKS ;
	// Task Should be blocked
	SelfTref->TaskState = Suspend ;
	//to be suspended immediately
	MYRTOS_OS_SVC_Set(SVC_terminateTask);

}
MYRTOS_errorID MyRTOS_GetTaskState(Task_ref* TRef, uint8* State)
{
	if (TRef == NULL_PTR || State == NULL_PTR)
	{
		return Task_Null_Pointer;
	}

	*State = TRef->TaskState;
	return NoError;
}


/*handler */
void bubbleSort()
{
	unsigned int i, j , n;
	Task_ref* temp ;
	n = OS_Control.NoOfActiveTasks ;
	for (i = 0; i < n - 1; i++)

		// Last i elements are already in place
		for (j = 0; j < n - i - 1; j++)
			if (OS_Control.OSTasks[j]->priority > OS_Control.OSTasks[j + 1]->priority)
			{
				temp = OS_Control.OSTasks[j] ;
				OS_Control.OSTasks[j] = OS_Control.OSTasks[j + 1 ] ;
				OS_Control.OSTasks[j + 1] = temp ;
			}

}

void MYRTOS_Update_Sch_teble(void)
{
	Task_ref * temp =NULL_PTR;
	Task_ref* Ptask =NULL_PTR ;
	Task_ref* PnextTask =NULL_PTR;
	uint8 iterator=0;
	/*1-buble sort sch_table ->OS_Control->OSTasks[100]
	 * periority high then low
	 * 2-free ready queue
	 * 3-update ready queue
	 * */
	bubbleSort();
	while(FIFO_dequeue(&READY_QUEUE ,&temp)!=FIFO_EMPTY);


	while(iterator < OS_Control.NoOfActiveTasks)
	{
		Ptask = OS_Control.OSTasks[iterator];

		if (Ptask->TaskState != Suspend)
		{
			if (iterator == OS_Control.NoOfActiveTasks - 1)
			{
				FIFO_enqueue(&READY_QUEUE, Ptask);
				Ptask->TaskState = ready;
				break;
			}

			PnextTask = OS_Control.OSTasks[iterator + 1];

			if (PnextTask->TaskState == Suspend)
			{
				FIFO_enqueue(&READY_QUEUE, Ptask);
				Ptask->TaskState = ready;
				break;
			}
			else if (Ptask->priority < PnextTask->priority)
			{
				FIFO_enqueue(&READY_QUEUE, Ptask);
				Ptask->TaskState = ready;
				break;
			}
			else if (Ptask->priority == PnextTask->priority)
			{
				FIFO_enqueue(&READY_QUEUE, Ptask);
				Ptask->TaskState = ready;
			}
			else /* priority > */
			{
				break;
			}
		}
		iterator++;
	}
}


void Decide_whatNext(void)
{
	__disable_irq();

	//if Ready Queue is empty && OS_Control->currentTask != suspend
	if (READY_QUEUE.counter == 0 && OS_Control.CurrentTask->TaskState != Suspend) //FIFO_EMPTY
	{
		OS_Control.CurrentTask->TaskState = Running ;
		//add the current task again(round robin)
		FIFO_enqueue(&READY_QUEUE, OS_Control.CurrentTask);
		OS_Control.NextTask = OS_Control.CurrentTask ;
	}else
	{
		FIFO_dequeue(&READY_QUEUE, &OS_Control.NextTask);
		OS_Control.NextTask->TaskState = Running ;
		//update Ready queue (to keep round robin Algo. happen)
		if ((OS_Control.CurrentTask->priority == OS_Control.NextTask->priority )&&(OS_Control.CurrentTask->TaskState != Suspend))
		{
			FIFO_enqueue(&READY_QUEUE, OS_Control.CurrentTask);
			OS_Control.CurrentTask->TaskState = ready ;
		}
	}

	__enable_irq();
}

/*to execute specific os service
 * handler mode */
void OS_SVC(uint32 * Stack_Frame)
{
	//r0,r1,r3,r12, lr , return address(pc) and xpsr
	uint8 SVC_Number=0;
	SVC_Number= *((uint8 *)(((uint8 *)Stack_Frame[6])-2));
	switch(SVC_Number)
	{
	case SVC_Activatetask:/*activate task*/
	case SVC_terminateTask:/*terminate task*/

		/*update sch table, ready queue
		 * os is in running state
		 * decide what next
		 * trigger os_pendsv(switch context/restore )
		 *  */
		MYRTOS_Update_Sch_teble();
		if(OS_Control.OSmodeID == OsRunning)
		{
			/*idle task or not */
			if (strcmp(OS_Control.CurrentTask->TaskName,"IdleTask") != 0)
			{
				//Decide what Next
				Decide_whatNext();

				//trigger OS_pendSV (Switch Context/Restore)
				trigger_OS_PendSV();
			}
		}

		break;
	case SVC_TaskWaitingTime:/* task waiting time*/
		MYRTOS_Update_Sch_teble();
		break;
	case SVC_AquireMutex:/* task Aquire Mutex*/
		break;
	case SVC_ReleaseMutex:/* task Release Mutex*/
		MYRTOS_Update_Sch_teble();
		if (OS_Control.OSmodeID == OsRunning)
		{
			if (strcmp(OS_Control.CurrentTask->TaskName, "IdleTask") != 0)
			{
				Decide_whatNext();
				trigger_OS_PendSV();
			}
		}
		break;
	}

}
/*this function must be assembly to be compatable with optimezation levels -01, -02,-03
 *  */
__attribute__  ((naked))void PendSV_Handler()
{

	//====================================
	//Save the Context of the Current Task
	//====================================
	//Get the Current Task "Current PSP from CPU register" as CPU Push XPSR,.....,R0
	OS_GET_PSP(OS_Control.CurrentTask->Current_PSP);

	//using this Current_PSP (Pointer) tp store (R4 to R11)
	OS_Control.CurrentTask->Current_PSP-- ;
	__asm volatile("mov %0,r4 " : "=r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP-- ;
	__asm volatile("mov %0,r5 " : "=r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP-- ;
	__asm volatile("mov %0,r6 " : "=r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP-- ;
	__asm volatile("mov %0,r7 " : "=r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP-- ;
	__asm volatile("mov %0,r8 " : "=r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP-- ;
	__asm volatile("mov %0,r9 " : "=r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP-- ;
	__asm volatile("mov %0,r10 " : "=r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP-- ;
	__asm volatile("mov %0,r11 " : "=r" (*(OS_Control.CurrentTask->Current_PSP))  );

	//save the current Value of PSP
	//already saved in Current_PSP



	//====================================
	//Restore the Context of the Next Task
	//====================================
	if (OS_Control.NextTask != NULL){
		OS_Control.CurrentTask = OS_Control.NextTask ;
		OS_Control.NextTask = NULL ;
	}

	__asm volatile("mov r11,%0 " : : "r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP++ ;
	__asm volatile("mov r10,%0 " : : "r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP++ ;
	__asm volatile("mov r9,%0 " : : "r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP++ ;
	__asm volatile("mov r8,%0 " : : "r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP++ ;
	__asm volatile("mov r7,%0 " : : "r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP++ ;
	__asm volatile("mov r6,%0 " : : "r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP++ ;
	__asm volatile("mov r5,%0 " : : "r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP++ ;
	__asm volatile("mov r4,%0 " : : "r" (*(OS_Control.CurrentTask->Current_PSP))  );
	OS_Control.CurrentTask->Current_PSP++ ;

	//update PSP and exit
	OS_SET_PSP(OS_Control.CurrentTask->Current_PSP);
	__asm volatile("BX LR");

}

MYRTOS_errorID Start_OS(void)
{

	OS_Control.OSmodeID=OsRunning;
	/*set default task (current task =idle task*/
	OS_Control.CurrentTask = IDLE_Task;
	OS_Control.CurrentTask->TaskState = Running;
	/*start ticker 1 ms*/
	HW_Init();/*privilage*/
	Start_Ticker();/*privilage*/

	OS_SET_PSP(OS_Control.CurrentTask->Current_PSP);
	/*switch thread mode sp from msp to psp  */
	OS_SWITCH_SP_TO_PSP;

	OS_SWITCH_TO_UNPRIVILEGED;

	OS_Control.CurrentTask->p_TaskEntry();

	return NoError;
}

void MYRTOS_Update_TasksWaitingTime()
{
	for (int i =0; i < OS_Control.NoOfActiveTasks ; i++  )
	{
		if (OS_Control.OSTasks[i]->TaskState == Suspend) //it is blocking until meet the time line
		{
			if (OS_Control.OSTasks[i]->TimingWaiting.Blocking == Enable)
			{
				OS_Control.OSTasks[i]->TimingWaiting.Ticks_Count-- ;
				if (OS_Control.OSTasks[i]->TimingWaiting.Ticks_Count == 0)
				{
					OS_Control.OSTasks[i]->TimingWaiting.Blocking = Disable ;
					OS_Control.OSTasks[i]->TaskState = ready ;
					//					MYRTOS_OS_SVC_Set(SVC_TaskWaitingTime);
				}
			}
		}
	}
	MYRTOS_Update_Sch_teble();
}

MYRTOS_errorID GetResource(Resource_t* Res)
{
	if (Res == NULL_PTR || OS_Control.CurrentTask == NULL_PTR)
		return Task_Null_Pointer;


	/*check if the task is the owner of the resource */
	if (Res->Owner == OS_Control.CurrentTask)
	{
		return Task_Invalid_State;
	}


	if (Res->Owner != NULL_PTR)
	{
		return Task_Invalid_State;
	}

	/*Ceiling Priority*/
	Res->Owner = OS_Control.CurrentTask;
	Res->PreviousPriority = OS_Control.CurrentTask->priority;

	if (OS_Control.CurrentTask->priority > Res->CeilingPriority)
	{
		OS_Control.CurrentTask->priority = Res->CeilingPriority;
	}

	return NoError;
}
MYRTOS_errorID ReleaseResource(Resource_t* Res)
{
	if (Res == NULL_PTR || Res->Owner != OS_Control.CurrentTask)
		return Task_Invalid_State;

	/* recovre the orignal priority for the task */
	OS_Control.CurrentTask->priority = Res->PreviousPriority;
	Res->Owner = NULL_PTR;


	MYRTOS_OS_SVC_Set(SVC_ReleaseMutex);

	return NoError;
}


MYRTOS_errorID Activate_task_FromISR(Task_ref* TRef)
{
	if (TRef == NULL_PTR)
	{
		return Task_Null_Pointer;
	}

	if (TRef->TaskState != Suspend)
	{
		return Task_Invalid_State;
	}

	TRef->TaskState = ready;

	/*we are in Handler mode/privileged  */
	MYRTOS_Update_Sch_teble();

	if (OS_Control.OSmodeID == OsRunning)
	{
		Decide_whatNext();
		trigger_OS_PendSV();   /*now we will trigger pendsv */
	}

	return NoError;
}

