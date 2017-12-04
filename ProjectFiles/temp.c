//

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom_map.h"

#include "temp.h"

#ifndef __SysCtlClockGet
#define __SysCtlClockGet()	\
SysCtlClockFreqSet( 			\
	SYSCTL_XTAL_25MHZ	| 		\
	SYSCTL_OSC_MAIN 	| 		\
	SYSCTL_USE_PLL 		| 		\
	SYSCTL_CFG_VCO_480, 		\
	120000000)
#endif

#define TMP006_CONFIG     0x02

#define TMP006_CFG_RESET    0x8000
#define TMP006_CFG_MODEON   0x7000
#define TMP006_CFG_1SAMPLE  0x0000
#define TMP006_CFG_2SAMPLE  0x0200
#define TMP006_CFG_4SAMPLE  0x0400
#define TMP006_CFG_8SAMPLE  0x0600
#define TMP006_CFG_16SAMPLE 0x0800
#define TMP006_CFG_DRDYEN   0x0100
#define TMP006_CFG_DRDY     0x0080

/* TEMPERATURE SENSOR REGISTER DEFINITIONS */

#define TMP006_I2CADDR 0x40
#define TMP006_MANID 0xFE
#define TMP006_DEVID 0xFF
#define TMP006_VOBJ  0x00
#define TMP006_TAMB 0x01

#define I2C_WRITE false
#define I2C_READ 	true

static uint32_t g_ui32SysClock;
static uint16_t mid, did;
/* Calibration constant for TMP006 */
static long double S0 = 0;

static void 
write16(uint8_t add, uint16_t data){
	
	uint8_t data_low  =  data & 0x00FF;
	uint8_t data_high = (data & 0xFF00)>>8;
	while(I2CMasterBusBusy(I2C0_BASE));
	I2CMasterSlaveAddrSet(I2C0_BASE, TMP006_I2CADDR, I2C_WRITE);
	I2CSlaveIntClear(I2C0_BASE);
	while(I2CMasterBusBusy(I2C0_BASE));
	I2CMasterDataPut(I2C0_BASE, add);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);	
	while(I2CMasterBusBusy(I2C0_BASE));
	I2CMasterDataPut(I2C0_BASE, data_high);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);	
	while(I2CMasterBusBusy(I2C0_BASE));
	I2CMasterDataPut(I2C0_BASE, data_low);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);	
	while(I2CMasterBusBusy(I2C0_BASE));
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);


}

static uint16_t 
read16(uint8_t add){
	uint16_t data;
	while(I2CMasterBusy(I2C0_BASE));
	I2CMasterSlaveAddrSet(I2C0_BASE, TMP006_I2CADDR, I2C_WRITE);
	I2CMasterDataPut(I2C0_BASE, add);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);
	I2CSlaveIntClear(I2C0_BASE);	
	while(I2CMasterBusy(I2C0_BASE));
	I2CMasterBurstLengthSet(I2C0_BASE, 3);
	I2CMasterSlaveAddrSet(I2C0_BASE, TMP006_I2CADDR, I2C_READ);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
	I2CSlaveIntClear(I2C0_BASE);	
	while(I2CMasterBusBusy(I2C0_BASE));
	data = (I2CMasterDataGet(I2C0_BASE));
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
	while(I2CMasterBusy(I2C0_BASE));
	data |= I2CMasterDataGet(I2C0_BASE) << 8;
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
	return data;
}

int16_t temp_read(){
	return read16(TMP006_DEVID);
}

int16_t temp_read_vobj(){

	return read16(TMP006_VOBJ);
}

int16_t temp_read_temp(){
	
	return read16(TMP006_TAMB);
}

long double TMP006_getTemp(void)
{
    volatile int Vobj = 0;
    volatile int Tdie = 0;
		volatile long double Tobj=0.0;
		volatile long double fObj =0.0;
		long double Vos=0.0;
		long double S=0.0;
		long double a1;
    long double a2 ;
    long double b0;
    long double b1;
    long double b2 ;
    long double c2;
    long double Tref ;
	  long double Vobj2;
    long double Tdie2;
	

    Vobj = read16(TMP006_DEVID);

    /* Read the object voltage */
    Vobj = temp_read_vobj();

    /* Read the ambient temperature */
    Tdie = temp_read_temp();
    Tdie = Tdie >> 2;

    /* Calculate TMP006. This needs to be reviewed and calibrated */
    Vobj2 = (double)Vobj*.00000015625;
    Tdie2 = (double)Tdie*.03525 + 273.15;

    /* Initialize constants */
    S0 = 6 * pow(10, -14);
    a1 = 1.75*pow(10, -3);
    a2 = -1.678*pow(10, -5);
    b0 = -2.94*pow(10, -5);
    b1 = -5.7*pow(10, -7);
    b2 = 4.63*pow(10, -9);
    c2 = 13.4;
    Tref = 298.15;

    /* Calculate values */
    S = S0*(1+a1*(Tdie2 - Tref)+a2*pow((Tdie2 - Tref),2));
		Vos = b0 + b1*(Tdie2 - Tref) + b2*pow((Tdie2 - Tref),2);	
		fObj = (Vobj2 - Vos) + c2*pow((Vobj2 - Vos),2);
    Tobj = pow(pow(Tdie2,4) + (fObj/S),.25);
    Tobj = (9.0/5.0)*(Tobj - 273.15) + 32;

    /* Return temperature of object */
    return (Tobj);
}


void 
temp_init(){
	uint16_t temp = 0;
	g_ui32SysClock = __SysCtlClockGet();
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
	
	SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C0) 	& 
				!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)	&
				!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOP));
	
	//GPIOPinTypeGPIOInput(GPIO_PORTP_BASE, GPIO_PIN_2);
	
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);
	GPIOPinConfigure(GPIO_PB3_I2C0SDA);
	
	GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
	GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
	
	//I2CMasterTimeoutSet(I2C0_BASE, 0x09FF);
	//I2CMasterGlitchFilterConfigSet
	I2CMasterInitExpClk(I2C0_BASE, g_ui32SysClock, false);
	
	HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;
	
	I2CMasterEnable(I2C0_BASE);
	
	write16(TMP006_CONFIG, TMP006_CFG_RESET);
	SysCtlDelay(5000);
	write16(TMP006_CONFIG, TMP006_CFG_MODEON | TMP006_CFG_2SAMPLE);
	SysCtlDelay(5000);
//	write16(TMP006_CONFIG, TMP006_CFG_MODEON | TMP006_CFG_DRDYEN | TMP006_CFG_8SAMPLE);
//	write16(TMP006_CONFIG, TMP006_CFG_MODEON | TMP006_CFG_DRDYEN | TMP006_CFG_8SAMPLE);

//	mid = read16(TMP006_MANID);
	did = read16(TMP006_DEVID);
}