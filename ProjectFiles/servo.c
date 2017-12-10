//

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "servo.h"

#ifndef __SysCtlClockGet
#define __SysCtlClockGet()	\
SysCtlClockFreqSet( 				\
	SYSCTL_XTAL_25MHZ	| 			\
	SYSCTL_OSC_MAIN 	| 			\
	SYSCTL_USE_PLL 		| 			\
	SYSCTL_CFG_VCO_480, 			\
	120000000)
#endif

//Acionamento por PPM
//-Per�odo Total: 20ms
//-Periodo Regime Maximo: 2ms
//-Periodo Regime Minimo: 1ms

//Formato de Onda: Regime M�ximo
//  ========== __________ ____...___ __________ __________
// |          |                                           |
// |<- 2 ms ->|                                           |
// |                                                      |
// |<----------------------- 20 ms ---------------------->|


//Formato de Onda: Regime M�nimo
//  =====______ __________ ___...____ __________ __________
// |     |                                                |
// |<1ms>|                                                |
// |                                                      |
// |<----------------------- 20 ms ---------------------->|

#define CLK_F (16000000)
#define MAX_F (500)

static uint32_t g_ui32SysClock;
static uint16_t g_ui16Period, g_ui16perMin;

void
servo_write(uint16_t angle){
	TimerMatchSet(TIMER3_BASE, TIMER_B, g_ui16perMin*angle/0xFFFF + g_ui16perMin);
}

void 
servo_init(){
	uint32_t duty_cycle;	
	
	//Configura/ Recupera Clock
	g_ui32SysClock = __SysCtlClockGet();
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER3) &
				!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM));

	GPIOPinConfigure(GPIO_PM3_T3CCP1);
	GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_3);

	//Frequencia 16MHz
	TimerClockSourceSet(TIMER3_BASE, TIMER_CLOCK_PIOSC);
			
	//Configura timer como par (A/B) e PWM
	TimerConfigure(TIMER3_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PWM);
	
	TimerControlLevel(TIMER3_BASE, TIMER_B, true);
	TimerUpdateMode(TIMER3_BASE, TIMER_B, TIMER_UP_MATCH_TIMEOUT);

	TimerPrescaleSet(TIMER3_BASE, TIMER_B, 4);

	//Per�odo 2ms
	g_ui16Period = 64000;
	//Periodo minimo 1ms = 2/2ms
	g_ui16perMin = 16000;
	duty_cycle = g_ui16perMin;
		
	TimerLoadSet(TIMER3_BASE, TIMER_B, g_ui16Period);
	TimerMatchSet(TIMER3_BASE, TIMER_B, duty_cycle);
	TimerEnable(TIMER3_BASE, TIMER_B);
}
