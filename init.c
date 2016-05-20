#include "stm32f303xx.h"
void init(void);
void Default_Handler(void);

// The following are 'declared' in the linker script
extern unsigned char  INIT_DATA_VALUES;
extern unsigned char  INIT_DATA_START;
extern unsigned char  INIT_DATA_END;
extern unsigned char  BSS_START;
extern unsigned char  BSS_END;
extern int main();
extern void SysTick(void);
extern void isr_usart2(void);
// the section "vectors" is placed at the beginning of flash 
// by the linker script
const void * Vectors[] __attribute__((section(".vectors"))) ={
	(void *)0x10001000, 	/* Top of stack in CCM*/ 
	init,   				/* Reset Handler */
    Default_Handler,		/* NMI */
	Default_Handler,		/* Hard Fault */
	Default_Handler,	  	/* MemManage */
	Default_Handler,      	/* BusFault  */
	Default_Handler,        /* UsageFault */
	Default_Handler, 		/* Reserved */ 
	Default_Handler, 		/* Reserved */
	Default_Handler, 		/* Reserved */
	Default_Handler, 		/* Reserved */
	Default_Handler,        /* SVCall */
	Default_Handler, 		/* Reserved */
	Default_Handler, 		/* Reserved */
	Default_Handler,        /* PendSV */
	SysTick,      			/* SysTick */	
/* External interrupt handlers follow */
	Default_Handler, 	/* WWDG */
	Default_Handler, 	/* PVD */
	Default_Handler, 	/* TAMP_SAMP */
	Default_Handler, 	/* RTC_WKUP */
	Default_Handler, 	/* FLASH */
	Default_Handler, 	/* RCC */
	Default_Handler, 	/* EXTI0 */
	Default_Handler, 	/* EXTI1 */
	Default_Handler, 	/* EXTI2 and TSC */
	Default_Handler, 	/* EXTI3 */
	Default_Handler, 	/* EXTI4 */
	Default_Handler, 	/* DMA_CH1 */
	Default_Handler, 	/* DMA_CH2 */
	Default_Handler, 	/* DMA_CH3 */
	Default_Handler, 	/* DMA_CH4 */
	Default_Handler, 	/* DMA_CH5 */
	Default_Handler, 	/* DMA_CH6 */
	Default_Handler, 	/* DMA_CH7 */
	Default_Handler, 	/* ADC1_2 */
	Default_Handler, 	/* USB_HP/CAN_TX */
	Default_Handler, 	/* USB_LP/CAN_RX0 */
	Default_Handler, 	/* CAN_RX1 */
	Default_Handler, 	/* CAN_SCE */
	Default_Handler, 	/* EXTI9_5 */
	Default_Handler, 	/* TIM1_BRK/TIM15 */
	Default_Handler, 	/* TIM1_UP/TIM16 */
	Default_Handler, 	/* TIM1_TRG/TIM17 */
	Default_Handler, 	/* TIM1_CC */
	Default_Handler, 	/* TIM2 */
	Default_Handler, 	/* TIM3 */
	Default_Handler, 	/* TIM4 */
	Default_Handler, 	/* I2C1_EV_EXTI23 */
	Default_Handler, 	/* I2C1_ER */
	Default_Handler, 	/* I2C2_EV_EXTI24 */
	Default_Handler, 	/* I2C2_ER */
	Default_Handler, 	/* SPI1 */
	Default_Handler, 	/* SPI2 */
	Default_Handler, 	/* USART1_EXTI25 */
	isr_usart2, 		/* USART2_EXTI26 */
	Default_Handler, 	/* USART3_EXTI28 */
	Default_Handler, 	/* EXTI15_10 */
	Default_Handler, 	/* RTCAlarm */
	Default_Handler, 	/* USB_WKUP */
	Default_Handler, 	/* TIM8_BRK */
	Default_Handler, 	/* TIM8_UP */
	Default_Handler, 	/* TIM8_TRG_COM */
	Default_Handler, 	/* TIM8_CC */
	Default_Handler, 	/* ADC3 */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* SPI3 */
	Default_Handler, 	/* UART4_EXTI34 */
	Default_Handler, 	/* UART5_EXTI35 */
	Default_Handler, 	/* TIM6_DACUNDER */
	Default_Handler, 	/* TIM7 */
	Default_Handler, 	/* DMA2_CH1 */
	Default_Handler, 	/* DMA2_CH2 */
	Default_Handler, 	/* DMA2_CH3 */
	Default_Handler, 	/* DMA2_CH4 */
	Default_Handler, 	/* DMA2_CH5 */
	Default_Handler, 	/* ADC4 */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* COMP123 */
	Default_Handler, 	/* COMP456 */
	Default_Handler, 	/* COMP7 */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* USB_HP */
	Default_Handler, 	/* USB_LP */
	Default_Handler, 	/* USB_WKUP */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* FPU */
};
void initFPU()
{
	// turn on FPU by setting bits 20 to 21 in CPACR
	CPAC |= BIT20 | BIT21 | BIT22 | BIT23;	
	// Wait until this instruction completes
	asm(" DSB ");
	asm(" ISB ");
	
}
	
void init()
{
// do global/static data initialization
	unsigned char *src;
	unsigned char *dest;
	unsigned len;
	src= &INIT_DATA_VALUES;
	dest= &INIT_DATA_START;
	len= &INIT_DATA_END-&INIT_DATA_START;
	while (len--)
		*dest++ = *src++;
// zero out the uninitialized global/static variables
	dest = &BSS_START;
	len = &BSS_END - &BSS_START;
	while (len--)
		*dest++=0;
	initFPU();
	main();
}

void Default_Handler()
{
	while(1);
}
