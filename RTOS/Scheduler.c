/*
 * Scheduler.c
 *
 *  Created on: Sep 27, 2026
 *      Author: pc
 */


#include "Scheduler.h"
#include "MyRtos_FIFO.h"
struct {
	Task_ref* OSTasks[100]; //Sch. Table
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
}OS_Control;

typedef enum {
	SVC_Activatetask,
	SVC_terminateTask,
	SVC_TaskWaitingTime,
	SVC_AquireMutex,
	SVC_ReleaseMutex
}SVC_ID;

FIFO_Buf_t READY_QUEUE ;
Task_ref * READY_QUEUE_FIFO[100];

Task_ref IDLE_Task_Instance;
Task_ref* IDLE_Task = &IDLE_Task_Instance;

void MyRtos_IDLE_Task(void)
{
	while(1)
	{
		__asm("nop");

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

void MyRTOS_Create_TaskStack(Task_ref* Tref)
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
	Tref->Current_PSP =(uint32 *) Tref->_S_PSP_Task ;

	Tref->Current_PSP-- ;
	*(Tref->Current_PSP) = 0x01000000;         //DUMMY_XPSR should T =1 to avoid BUS fault;//0x01000000

	Tref->Current_PSP-- ;
	*(Tref->Current_PSP) = (unsigned int)Tref->p_TaskEntry ; //PC

	Tref->Current_PSP-- ; //LR = 0xFFFFFFFD (EXC_RETURN)Return to thread with PSP
	*(Tref->Current_PSP)  = 0xFFFFFFFD ;

	for (int  j=0 ; j< 13 ; j++ )
	{
		Tref->Current_PSP-- ;
		*(Tref->Current_PSP)  = 0 ;

	}


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
	MyRTOS_Create_TaskStack(TRef);

	/*task state update -> suspended */
	TRef->TaskState=Suspend;

	return Error_Status;

}

MYRTOS_errorID MYRTOS_Init()
{
	MYRTOS_errorID Error_Status=NoError;
	//update os mode
	OS_Control.OSmodeID=OSsuspend;

	//specify the main stack for os
	Error_Status=MYRTOS_Create_MainStack();
	//create os ready queue
	if(FIFO_init(&READY_QUEUE, READY_QUEUE_FIFO, 100)!= FIFO_NO_ERROR)
	{
		Error_Status+=Ready_Queue_init_error;

	}
	//cofig idle task


	IDLE_Task->priority=255;
	IDLE_Task->p_TaskEntry=MyRtos_IDLE_Task;
	IDLE_Task->Stack_Size=300;

	Error_Status += MYRTOS_Create_task(IDLE_Task);
	return Error_Status;
}

//to execute specific os service
void OS_SVC(uint32 * Stack_Frame)
{
	//r0,r1,r3,r12, lr , return address(pc) and xpsr
	uint8 SVC_Number=0;
	SVC_Number= *((uint8 *)((uint8 *)Stack_Frame[6])-2);
	switch(SVC_Number)
	{
	case 0://activate task
		break;
	case 1://terminate task
		break;


	}
}

void PendSV_Handler()
{

}
void OS_SVC_Set(uint32 svc_id)
{
	switch (svc_id) {
	case 0://activate task
		__asm("svc #0x00");
		break;
	case 1://terminate task
		__asm("svc #0x01");
		break;
	case 2://os pendsv
		__asm("svc #0x02");
		break;


	}
}
