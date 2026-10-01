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


void MYRTOS_Update_Sch_teble(void);

void MyRtos_IDLE_Task(void)
{
	while(1)
	{
		__asm("wfe");
		Idle_task_LED^=1;

	}
}

/* internal helper: TaskType -> TCB */
static Task_ref* Os_GetTask(TaskType TaskID)
{
	uint8 i;
	if (TaskID == INVALID_TASK)
	{
		return NULL_PTR;
	}
	for (i = 0; i < OS_Control.NoOfActiveTasks; i++)
	{
		if (OS_Control.OSTasks[i]->TaskID == TaskID)
			return OS_Control.OSTasks[i];   /* pointer moves, ID travels with it */
	}
	return NULL_PTR;
}

StatusType MYRTOS_Create_MainStack()
{
	StatusType Error_Status=E_OK;

	OS_Control._S_MSP_Task= ((uint32)&_estack);
	OS_Control._E_MSP_Task=(OS_Control._S_MSP_Task -MainStackSize);
	//allign 8 bytes space btw main task , psp tasks
	OS_Control.PSP_Task_Locator=(OS_Control._E_MSP_Task -8);

	if (OS_Control._E_MSP_Task < ((uint32)(&_end) + RESERVED_HEAP_STACK))
	{
		return E_OS_STACKFAULT;
	}
	else
	{

	}

	return Error_Status;


}

StatusType MyRTOS_Create_TaskStack(Task_ref* Tref)
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
		return E_OS_ID;
	}
	/*check if excedded of max number os tasks*/
	if (OS_Control.NoOfActiveTasks >= MAX_TASKS)
	{
		return E_OS_LIMIT;
	}
	Tref->Current_PSP =(uint32 *) Tref->_S_PSP_Task ;

	Tref->Current_PSP-- ;
	if(Tref->Current_PSP < (uint32*)Tref->_E_PSP_Task )
	{
		return E_OS_STACKFAULT;
	}
	else
	{
		*(Tref->Current_PSP) = 0x01000000;         //DUMMY_XPSR should T =1 to avoid BUS fault;//0x01000000
		Tref->Current_PSP-- ;
		if(Tref->Current_PSP < (uint32*)Tref->_E_PSP_Task)
		{
			return E_OS_STACKFAULT;

		}
		else
		{
			*(Tref->Current_PSP) = (unsigned int)Tref->p_TaskEntry ; //PC
			Tref->Current_PSP-- ; //LR = 0xFFFFFFFD (EXC_RETURN)Return to thread with PSP
			if(Tref->Current_PSP <  (uint32*)Tref->_E_PSP_Task)
			{
				return E_OS_STACKFAULT;

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
			return E_OS_STACKFAULT;
		}
		else
		{
			*(Tref->Current_PSP)  = 0 ;

		}

	}

	return E_OK;
}

StatusType MYRTOS_Create_task(Task_ref * TRef)
{

	if(TRef==NULL_PTR)
	{
		return E_OS_ID;

	}
	else if(OS_Control.NoOfActiveTasks>= MAX_TASKS)
	{
		return E_OS_LIMIT  ;
	}
	else
	{

	}
	StatusType Error_Status=E_OK;

	/*create psp stack for the task
	 *chack stack size not exceeded the psp stack */
	TRef->TaskID =(TaskType)OS_Control.NoOfActiveTasks;
	TRef->_S_PSP_Task=OS_Control.PSP_Task_Locator;
	TRef->_E_PSP_Task= (TRef->_S_PSP_Task - TRef->Stack_Size);
	if (TRef->_E_PSP_Task < ((uint32)(&_end) + RESERVED_HEAP_STACK))
	{
		return E_OS_STACKFAULT;
	}
	/*alligned 8 bytes*/
	OS_Control.PSP_Task_Locator = (TRef->_E_PSP_Task - 8);
	/*Init psp task stack */
	Error_Status= MyRTOS_Create_TaskStack(TRef);

	/*update sch table and no of active tasks */
	//	OS_Control.OSTasks[OS_Control.NoOfActiveTasks]=TRef;
	//	OS_Control.NoOfActiveTasks++;
	/*task state update -> suspended */
	if(Error_Status ==E_OK)
	{

		OS_Control.OSTasks[OS_Control.NoOfActiveTasks] = TRef;
		OS_Control.NoOfActiveTasks++;
		TRef->TaskState=SUSPENDED;
	}
	else
	{

	}

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


StatusType MYRTOS_Init(void)
{
	StatusType Error_Status=E_OK;
	//update os mode
	OS_Control.OSmodeID=OSsuspend;

	//specify the main stack for os
	Error_Status=MYRTOS_Create_MainStack();
	//create os ready queue
	if(FIFO_init(&READY_QUEUE, READY_QUEUE_FIFO, MAX_TASKS)!= FIFO_NO_ERROR)
	{
		/*fatal karnel inveriant broken -> shutdown */
		__disable_irq();
		ShutdownOS(E_OS_ILLEGAL);
		//		Error_Status+=E_OS_ILLEGAL;
		while(1);
	}
	//cofig idle task

	strcpy(IDLE_Task->TaskName, "IdleTask");
	IDLE_Task->priority=255;
	IDLE_Task->p_TaskEntry=MyRtos_IDLE_Task;
	IDLE_Task->Stack_Size=300;

	Error_Status = MYRTOS_Create_task(IDLE_Task);
	if (Error_Status != E_OK)
	{
		return Error_Status;   /* stop here  */
	}

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



//StatusType Activate_task(Task_ref * TRef)
StatusType ActivateTask(TaskType TaskID)
{

	Task_ref * TRef =Os_GetTask(TaskID);
	/*  NULL check */
	if (TRef == NULL_PTR)
	{
		return E_OS_ID;
	}
	/* already activated (not Suspended */
	else if (TRef->TaskState != SUSPENDED)
	{
		return E_OS_LIMIT;
	}
	else
	{
		if(MyRTOS_Create_TaskStack(TRef) !=E_OK)
		{
			return E_OS_STACKFAULT;
		}
		else {

		}


	}
	TRef->TaskState = READY;

	if(__get_IPSR() !=0u)
	{		/*handler mode*/
		/*we are in Handler mode/privileged  */
		MYRTOS_Update_Sch_teble();

		if (OS_Control.OSmodeID == OsRunning)
		{
			Decide_whatNext();
			trigger_OS_PendSV();   /*now we will trigger pendsv */
		}


	}
	else
	{		/*thread mode */
		/*update sch. table
		 * svc interrupt/ handler (update ready queue)
		 * set pendsv (decide what next (dequeue) ,then switch context)
		 * */
		MYRTOS_OS_SVC_Set(SVC_Activatetask);
	}


	return E_OK;
}

//StatusType Terminate_task(void)
//{
//	if (OS_Control.CurrentTask == NULL_PTR)
//		return E_OS_ID;
//
//	OS_Control.CurrentTask->TaskState = SUSPENDED;
//	MYRTOS_OS_SVC_Set(SVC_terminateTask);
//	return E_OK;
//}
StatusType TerminateTask(void)
{
	Task_ref* T = OS_Control.CurrentTask;

	if (__get_IPSR() != 0u)        /* task level only, not from an ISR */
	{
		return E_OS_CALLEVEL;
	}
	else if (T == NULL_PTR)
	{		return E_OS_STATE;
	}
	//	if (T->HeldRes != NULL_PTR)    /*  may not terminate holding a resource */
	//		return E_OS_RESOURCE;

	T->TaskState = SUSPENDED;
	MYRTOS_OS_SVC_Set(SVC_terminateTask);

	/* ===== OSEK: this point is unreachable on success =====
	 * (next activation gets a fresh stack frame). If we get here,
	 * no context switch happened = kernel bug. Fail loudly: */
	//    ErrorHook(E_OS_ILLEGAL);
	__disable_irq();
	while (1);
}
//void MYRTOS_TaskWait(unsigned int NoTICKS,Task_ref* SelfTref)
//{
//	SelfTref->TimingWaiting.Blocking = Enable ;
//	SelfTref->TimingWaiting.Ticks_Count = NoTICKS ;
//	// Task Should be blocked
//	SelfTref->TaskState = WAITING ;
//	//to be SUSPENDED immediately
//	MYRTOS_OS_SVC_Set(SVC_terminateTask);
//
//}

StatusType TaskWait(TaskType TaskID, uint32 NoTicks)
{
	Task_ref* T = Os_GetTask(TaskID);

	/* unknown task ID (replaces the old NULL check) */
	if (T == NULL_PTR)
	{
		return E_OS_ID;
	}else if (__get_IPSR() != 0u)
	{
		/* an ISR can never block — there is no task context to suspend */
		return E_OS_CALLEVEL;
	}
	else if (OS_Control.OSmodeID != OsRunning)
	{
		/* services are only valid between StartOS and ShutdownOS */
		return E_OS_CALLEVEL;
	}
	else if (NoTicks == 0u)
	{
		/* parameter out of range */
		return E_OS_VALUE;
	}
	else if(T==IDLE_Task)
	{
		/* the idle task must NEVER wait — it's the fallback */
		return E_OS_STATE;
	}


	if (T == OS_Control.CurrentTask)
	{
		/*========== branch 1: block MYSELF ==========*/
		T->TimingWaiting.Blocking    = Enable;
		T->TimingWaiting.Ticks_Count = NoTicks;
		T->TaskState = WAITING;                 /* freeze — NO frame rebuild! */

		MYRTOS_OS_SVC_Set(SVC_terminateTask);   /* switch away immediately */

		return E_OK;
	}

	/*========== branch 2: block ANOTHER task ==========*/
	if (T->TaskState != READY)
	{
		return E_OS_STATE;   /* must sit in the queue */
	}
	else
	{
		T->TimingWaiting.Blocking    = Enable;
		// Task Should be blocked
		T->TimingWaiting.Ticks_Count = NoTicks;
		T->TaskState = WAITING;
	}
	/* rebuild ready queue via syscall (drops the task we just blocked).
	 * No context switch needed — the RUNNING task keeps the CPU. */
	MYRTOS_OS_SVC_Set(SVC_TaskWaitingTime);

	return E_OK;
}

StatusType Os_Delay(uint32 NoTicks)
{
	if (OS_Control.CurrentTask == NULL_PTR)
	{
		return E_OS_STATE;
	}
	return TaskWait(OS_Control.CurrentTask->TaskID, NoTicks);
}
StatusType MyRTOS_GetTaskState(Task_ref* TRef, uint8* State)
{
	if (TRef == NULL_PTR || State == NULL_PTR)
	{
		return E_OS_ID;
	}

	*State = TRef->TaskState;
	return E_OK;
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

		if (Ptask->TaskState != SUSPENDED)
		{
			if (iterator == OS_Control.NoOfActiveTasks - 1)
			{
				FIFO_enqueue(&READY_QUEUE, Ptask);
				Ptask->TaskState = READY;
				break;
			}

			PnextTask = OS_Control.OSTasks[iterator + 1];

			if (PnextTask->TaskState == SUSPENDED)
			{
				FIFO_enqueue(&READY_QUEUE, Ptask);
				Ptask->TaskState = READY;
				break;
			}
			else if (Ptask->priority < PnextTask->priority)
			{
				FIFO_enqueue(&READY_QUEUE, Ptask);
				Ptask->TaskState = READY;
				break;
			}
			else if (Ptask->priority == PnextTask->priority)
			{
				FIFO_enqueue(&READY_QUEUE, Ptask);
				Ptask->TaskState = READY;
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

	//if Ready Queue is empty && OS_Control->currentTask != SUSPENDED
	if (READY_QUEUE.counter == 0 && OS_Control.CurrentTask->TaskState != SUSPENDED) //FIFO_EMPTY
	{
		OS_Control.CurrentTask->TaskState = RUNNING ;
		//add the current task again(round robin)
		FIFO_enqueue(&READY_QUEUE, OS_Control.CurrentTask);
		OS_Control.NextTask = OS_Control.CurrentTask ;
	}else
	{
		FIFO_dequeue(&READY_QUEUE, &OS_Control.NextTask);
		OS_Control.NextTask->TaskState = RUNNING ;
		//update Ready queue (to keep round robin Algo. happen)
		if ((OS_Control.CurrentTask->priority == OS_Control.NextTask->priority )&&(OS_Control.CurrentTask->TaskState != SUSPENDED))
		{
			FIFO_enqueue(&READY_QUEUE, OS_Control.CurrentTask);
			OS_Control.CurrentTask->TaskState = READY ;
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

		/*update sch table, READY queue
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

StatusType Start_OS(void)
{

	OS_Control.OSmodeID=OsRunning;
	/*set default task (current task =idle task*/
	OS_Control.CurrentTask = IDLE_Task;
	OS_Control.CurrentTask->TaskState = RUNNING;
	/*start ticker 1 ms*/
	HW_Init();/*privilage*/
	Start_Ticker();/*privilage*/

	OS_SET_PSP(OS_Control.CurrentTask->Current_PSP);
	/*switch thread mode sp from msp to psp  */
	OS_SWITCH_SP_TO_PSP;

	OS_SWITCH_TO_UNPRIVILEGED;

	OS_Control.CurrentTask->p_TaskEntry();

	return E_OK;
}

void MYRTOS_Update_TasksWaitingTime()
{
	for (int i =0; i < OS_Control.NoOfActiveTasks ; i++  )
	{
		if (OS_Control.OSTasks[i]->TaskState == SUSPENDED) //it is blocking until meet the time line
		{
			if (OS_Control.OSTasks[i]->TimingWaiting.Blocking == Enable)
			{
				OS_Control.OSTasks[i]->TimingWaiting.Ticks_Count-- ;
				if (OS_Control.OSTasks[i]->TimingWaiting.Ticks_Count == 0)
				{
					OS_Control.OSTasks[i]->TimingWaiting.Blocking = Disable ;
					OS_Control.OSTasks[i]->TaskState = READY ;
					//					MYRTOS_OS_SVC_Set(SVC_TaskWaitingTime);
				}
			}
		}
	}
	MYRTOS_Update_Sch_teble();
}

StatusType GetResource(Resource_t* Res)
{
	if (Res == NULL_PTR || OS_Control.CurrentTask == NULL_PTR)
		return E_OS_ID;


	/*check if the task is the owner of the resource */
	if (Res->Owner == OS_Control.CurrentTask)
	{
		return E_OS_STATE;
	}


	if (Res->Owner != NULL_PTR)
	{
		return E_OS_STATE;
	}

	/*Ceiling Priority*/
	Res->Owner = OS_Control.CurrentTask;
	Res->PreviousPriority = OS_Control.CurrentTask->priority;

	if (OS_Control.CurrentTask->priority > Res->CeilingPriority)
	{
		OS_Control.CurrentTask->priority = Res->CeilingPriority;
	}

	return E_OK;
}
StatusType ReleaseResource(Resource_t* Res)
{
	if (Res == NULL_PTR || Res->Owner != OS_Control.CurrentTask)
		return E_OS_STATE;

	/* recovre the orignal priority for the task */
	OS_Control.CurrentTask->priority = Res->PreviousPriority;
	Res->Owner = NULL_PTR;


	MYRTOS_OS_SVC_Set(SVC_ReleaseMutex);

	return E_OK;
}



void ShutdownOS(StatusType Error)
{
	OS_Control.OSmodeID=OSsuspend;
	Stop_Ticker();
	__disable_irq();
	while(1);

}
