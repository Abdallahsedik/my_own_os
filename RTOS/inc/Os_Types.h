#ifndef OS_TYPES_H
#define OS_TYPES_H
#include "Std_Types.h"

typedef uint8 StatusType;

/* ---- Task identification ---- */
typedef uint8   TaskType;              /* index into OSTasks[] = task ID      */
typedef TaskType* TaskRefType;         /* for GetTaskID (OUT param)          */
#define INVALID_TASK  ((TaskType)0xFF)

/* ---- Task states 													 ---- */
typedef enum { SUSPENDED, READY, RUNNING, WAITING } TaskStateType;
typedef TaskStateType* TaskStateRefType;   /* for GetTaskState (OUT param)   */

/* ---- Resources ---- */
typedef uint8 ResourceType;

/* ---- Application mode (for StartOS / AUTOSTART) ---- */
typedef uint8 AppModeType;
#define OSDEFAULTAPPMODE ((AppModeType)0)

/* ---- Error codes: E_OK + the OSEK standard set + AUTOSAR extras ---- */
//#define E_OK               0x00
#define E_OS_ACCESS       		0x01   /* no right / wrong resource usage   */
#define E_OS_CALLEVEL     		0x02   /* called from wrong context (ISR)   */
#define E_OS_ID        		    0x03   /* bad TaskID/ResourceID  			*/
#define E_OS_LIMIT         		0x04   /* too many activations              */
#define E_OS_NOFUNC        		0x05
#define E_OS_RESOURCE      		0x06   /* terminating while holding resource*/
#define E_OS_STATE         		0x07   /* wrong task state                  */
#define E_OS_VALUE         		0x08
#define E_OS_STACKFAULT    		0x09   /* like  stack overflow case          */
#define E_OS_PROTECTION_MEMORY 	0x0A
#define E_OS_ILLEGAL 			0x0B

/* ---- Macros users put in their code (AUTOSAR look & feel) ---- */
#define TASK(name)          void name(void)
#define ISR(name)           void name(void)     /* Cat.2 ISR           */
#define DeclareTask(name)   extern TaskType Task_##name
#define DeclareResource(name) extern ResourceType Res_##name

#endif
