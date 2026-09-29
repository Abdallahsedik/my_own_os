/*
 * Cortexmx_OsPorting.c
 *
 *  Created on: Sep 27, 2026
 *      Author: pc
 */


#include "Cortexmx_OsPorting.h"

void HardFault_Handler(void){
	while(1);

}
void	MemManage_Handler(void){
	while(1);
}
void	BusFault_Handler(void){
	while(1);
}
void UsageFault_Handler(void){
	while(1);
}

__attribute__ ((naked)) void SVC_Handler(void)
{
	__asm("tst lr ,#4 \n\t"
			"ITE EQ   \n\t"
			"mrseq r0 ,msp \n\t"
			"mrsne r0 ,psp \n\t"
			"B OS_SVC");

}
void HW_Init()
{
	__NVIC_SetPriority(PendSV_IRQn,15);
}

void trigger_OS_PendSV(void)
{
	SCB->ICSR |=SCB_ICSR_PENDSVSET_Msk;

}
void Start_Ticker( )
{

	Systic_Config(8000);
}

