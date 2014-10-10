/**
 *	Copyright (c) 2013 James Walmsley
 *
 *
 **/

#include <bitthunder.h>
#include "bt_types.h"

#include <FreeRTOS.h>
#include <task.h>
#include <portmacro.h>
#include <nvic.h>
#include <scb.h>


struct _BT_OPAQUE_HANDLE {
	BT_HANDLE_HEADER h;
};

/* For backward compatibility, ensure configKERNEL_INTERRUPT_PRIORITY is
defined.  The value should also ensure backward compatibility.
FreeRTOS.org versions prior to V4.4.0 did not include this definition. */
#ifndef configKERNEL_INTERRUPT_PRIORITY
	#define configKERNEL_INTERRUPT_PRIORITY 255
#endif


static BT_ERROR tick_isr_handler(BT_u32 ulIRQ, void *pParam) {
	xTaskIncrementTick();

#if configUSE_PREEMPTION == 1
	vTaskSwitchContext();
#endif

	// Timer interrupt status is handled automatically by BitThunder driver.
	return BT_ERR_NONE;
}

/* Constants required to manipulate the NVIC. */
#define portNVIC_PENDSVSET			0x10000000

/* Constants required to setup the task context. */
/* System mode, ARM mode, interrupts enabled. */
#define portINITIAL_XPSR			( 0x01000000 )

/* Constants required to manipulate the NVIC. */
#define portNVIC_SYSTICK_CTRL		( ( volatile unsigned long *) 0xe000e010 )
#define portNVIC_SYSTICK_LOAD		( ( volatile unsigned long *) 0xe000e014 )
#define portNVIC_INT_CTRL			( ( volatile unsigned long *) 0xe000ed04 )
#define portNVIC_SYSPRI2			( ( volatile unsigned long *) 0xe000ed20 )
#define portNVIC_SYSTICK_CLK		0x00000004
#define portNVIC_SYSTICK_INT		0x00000002
#define portNVIC_SYSTICK_ENABLE		0x00000001
#define portNVIC_PENDSVSET			0x10000000
#define portNVIC_PENDSV_PRI			( ( ( unsigned long ) configKERNEL_INTERRUPT_PRIORITY ) << 16 )
#define portNVIC_SYSTICK_PRI		( ( ( unsigned long ) configKERNEL_INTERRUPT_PRIORITY ) << 24 )
#define portNVIC_AIRCR_CTRL			( ( volatile unsigned long *) 0xe000ed0c )

/*
 * Exception handlers.
 */
void BT_NVIC_PendSV_Handler( void ) __attribute__ (( naked ));
void BT_NVIC_SysTick_Handler( void );
void BT_NVIC_SVC_Handler( void ) __attribute__ (( naked ));


static unsigned portBASE_TYPE uxCriticalNesting = 0xaaaaaaaa;

/**
 *	Initialise the stack of a task to look as if a call to
 *	portSAVE_CONTEXT has been called.
 *
 **/
/*
 * See header file for description.
 */
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{
	/* Simulate the stack frame as it would be created by a context switch
	interrupt. */
	pxTopOfStack--; /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts. */
	*pxTopOfStack = portINITIAL_XPSR;	/* xPSR */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) pxCode;	/* PC */
	pxTopOfStack--;
	*pxTopOfStack = 0;	/* LR */
	pxTopOfStack -= 5;	/* R12, R3, R2 and R1. */
	*pxTopOfStack = ( portSTACK_TYPE ) pvParameters;	/* R0 */
	pxTopOfStack -= 8;	/* R11, R10, R9, R8, R7, R6, R5 and R4. */

	return pxTopOfStack;
}


void vPortExitTask( void ) {
	vTaskDelete(NULL);
}


static void prvSetupTimerInterrupt(void) {

	BT_ERROR Error;
	BT_MACHINE_DESCRIPTION *pMachine = BT_GetMachineDescription(&Error);
	if(!pMachine) {
		return;
	}

	const BT_INTEGRATED_DRIVER *pDriver;
	BT_HANDLE hTimer = NULL;

	pDriver = BT_GetIntegratedDriverByName(pMachine->pSystemTimer->name);
	if(!pDriver) {
		return;
	}

	hTimer = pDriver->pfnProbe(pMachine->pSystemTimer, &Error);
	if(!hTimer) {
		return;
	}

	BT_SetSystemTimerHandle(hTimer);

	const BT_DEV_IF_SYSTIMER *pTimerOps = hTimer->h.pIf->oIfs.pDevIF->unConfigIfs.pSysTimerIF;
	pTimerOps->pfnSetFrequency(hTimer, configTICK_RATE_HZ);
	pTimerOps->pfnRegisterInterrupt(hTimer, tick_isr_handler, NULL);
	pTimerOps->pfnEnableInterrupt(hTimer);
	pTimerOps->pfnStart(hTimer);
}


extern void enable_irq(void);

void vPortStartFirstTask( void )
{
	__asm volatile(
					" ldr r0, =0xE000ED08 	\n" /* Use the NVIC offset register to locate the stack. */
					" ldr r0, [r0] 			\n"
					" ldr r0, [r0] 			\n"
					" msr msp, r0			\n" /* Set the msp back to the start of the stack. */
					" cpsie i				\n" /* Globally enable interrupts. */
					" svc 0					\n" /* System call to start first task. */
					" nop					\n"
				);
}

portBASE_TYPE xPortStartScheduler(void) {

	BT_SetInterruptPriority(14, 31);		//PENDSVC
	BT_SetInterruptPriority(15, 31);		//SysTick

	// Setup Hardware Timer!
	prvSetupTimerInterrupt();
	// Start first task

	uxCriticalNesting = 0;

	vPortStartFirstTask();

	// Should not get here!
	return 0;
}

/*
 *	Critical Sections.
 *
 **/

/* The code generated by the GCC compiler uses the stack in different ways at
different optimisation levels.  The interrupt flags can therefore not always
be saved to the stack.  Instead the critical section nesting level is stored
in a variable, which is then saved as part of the stack context. */
void vPortEnterCritical( void )
{
	/* Disable interrupts as per portDISABLE_INTERRUPTS(); 					*/
	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;
}

void vPortExitCritical( void )
{
	uxCriticalNesting--;
	if(!uxCriticalNesting) {
		portENABLE_INTERRUPTS();
	}
}


void vPortEndScheduler( void )
{
	/* It is unlikely that the ARM port will require this function as there
	is nothing to return to.  */
}




/*
 * FreeRTOS bottom-level IRQ vector handler
 */
void vFreeRTOS_IRQInterrupt ( void ) __attribute__((naked));
void vFreeRTOS_IRQInterrupt ( void )
{

	unsigned portLONG ulDummy;

	/* If using preemption, also force a context switch. */
	//#if configUSE_PREEMPTION == 1
	//	SCB->ICSR = portNVIC_PENDSVSET;
	//#endif

	ulDummy = portSET_INTERRUPT_MASK_FROM_ISR();
	{
		xTaskIncrementTick();
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( ulDummy );

	/* Save the context of the interrupted task. */
	//portSAVE_CONTEXT();

	//ulCriticalNesting++;

	//__asm volatile( "clrex" );

	/* Call the handler provided with the standalone BSP */
	//__asm volatile( "bl  BT_ARCH_ARM_GIC_IRQHandler" );

	//ulCriticalNesting--;

	/* Restore the context of the new task. */
	//portRESTORE_CONTEXT();
}

void BT_NVIC_SVC_Handler( void )
{
	__asm volatile (
					"	ldr	r3, pxCurrentTCBConst2		\n" /* Restore the context. */
					"	ldr r1, [r3]					\n" /* Use pxCurrentTCBConst to get the pxCurrentTCB address. */
					"	ldr r0, [r1]					\n" /* The first item in pxCurrentTCB is the task top of stack. */
					"	ldmia r0!, {r4-r11}				\n" /* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
					"	msr psp, r0						\n" /* Restore the task stack pointer. */
					"	mov r0, #0 						\n"
					"	msr	basepri, r0					\n"
					"	orr r14, #0xd					\n"
					"	bx r14							\n"
					"									\n"
					"	.align 2						\n"
					"pxCurrentTCBConst2: .word pxCurrentTCB				\n"
				);
}


void BT_NVIC_PendSV_Handler( void )
{
	/* This is a naked function. */

	__asm volatile
	(
	"	mrs r0, psp							\n"
	"										\n"
	"	ldr	r3, pxCurrentTCBConst			\n" /* Get the location of the current TCB. */
	"	ldr	r2, [r3]						\n"
	"										\n"
	"	stmdb r0!, {r4-r11}					\n" /* Save the remaining registers. */
	"	str r0, [r2]						\n" /* Save the new top of stack into the first member of the TCB. */
	"										\n"
	"	stmdb sp!, {r3, r14}				\n"
	"	mov r0, %0							\n"
	"	msr basepri, r0						\n"
	"	bl vTaskSwitchContext				\n"
	"	mov r0, #0							\n"
	"	msr basepri, r0						\n"
	"	ldmia sp!, {r3, r14}				\n"
	"										\n"	/* Restore the context, including the critical nesting count. */
	"	ldr r1, [r3]						\n"
	"	ldr r0, [r1]						\n" /* The first item in pxCurrentTCB is the task top of stack. */
	"	ldmia r0!, {r4-r11}					\n" /* Pop the registers. */
	"	msr psp, r0							\n"
	"	bx r14								\n"
	"										\n"
	"	.align 2							\n"
	"pxCurrentTCBConst: .word pxCurrentTCB	\n"
	::"i"(configMAX_SYSCALL_INTERRUPT_PRIORITY)
	);
}

void BT_NVIC_SysTick_Handler(void)
{
	unsigned long ulDummy;

	/* If using preemption, also force a context switch. */
	#if configUSE_PREEMPTION == 1
		*(portNVIC_INT_CTRL) = portNVIC_PENDSVSET;
	#endif

	ulDummy = portSET_INTERRUPT_MASK_FROM_ISR();
	{
		xTaskIncrementTick();
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( ulDummy );
}


void vPortYieldFromISR( void )
{
	/* Set a PendSV to request a context switch. */
	*(portNVIC_INT_CTRL) = portNVIC_PENDSVSET;

	__asm volatile ("dsb");
	__asm volatile ("isb");
}

void vPortReset( void )
{
	*(portNVIC_AIRCR_CTRL) = SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
    while(1);
}
