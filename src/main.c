#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_uart.h"
#endif

#include <cr_section_macros.h>

typedef enum{
	micro = 0,
	mili,
	cuarto_segundo,
	medio_segundo,
	segundo
}Tiempo_Type;

uint32_t j = 0, adcv = 0, dac = 0;									//Globales de Control
float volts = 0, temperature = 0;
uint32_t Tiempo[5]={25,25000,6250000,12500000,25000000};	//Periodos [micro,mili,segundo/2,segundo]
uint8_t pinMatch;											//Global de control del pin match

void configADC();
void configPin();
void configTimer();
void configDAC();
void configUART();
void UART3_SendByte(uint8_t);

int main(void) {
	configPin();
	configADC();
	configDAC();
	configUART();
	configTimer();
    while(1);
    return 0;
}


// Configurations

void configTimer(){
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER0, CLKPWR_PCLKSEL_CCLK_DIV_4);

	TIM_MATCHCFG_Type match;
	match.MatchChannel = 1;
	match.IntOnMatch = ENABLE;
	match.StopOnMatch = DISABLE;
	match.ResetOnMatch = ENABLE;
	match.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	match.MatchValue = (Tiempo[cuarto_segundo]-1)/2;
	TIM_ConfigMatch(LPC_TIM0, &match);
	LPC_TIM0->EMR |= 3<<6;
	LPC_TIM0->PR = 0;

	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	NVIC_EnableIRQ(TIMER0_IRQn);

	TIM_Cmd(LPC_TIM0, ENABLE);
	TIM_ResetCounter(LPC_TIM0);
}

void configADC(){
	LPC_SC->PCONP 			|= 1<<12;		// Enciendo ADC
	LPC_SC->PCLKSEL0		|= 3<<24;		// f = 12.5MHz
	LPC_ADC->ADCR 			|= 1<<8;		// Divisor por 1
	LPC_ADC->ADCR 			|= 0<<16;		// Modo burst = FALSE
	LPC_ADC->ADCR			|= (4<<24);		// Start = 100 -> Start on MATCH 0.1
	//LPC_ADC->ADINTEN		|= 1<<0;		// El canal 0 produce interrupciones
	LPC_ADC->ADINTEN		&= ~(1<<8);		// Interrupciones por los canales individuales-> VA SI O SI

	LPC_ADC->ADCR			|= 1<<21;		//PDN = 1
	NVIC_EnableIRQ(ADC_IRQn);
}

void configDAC(){
	DAC_Init(LPC_DAC);
	/* Limita la corriente maxima que consume el DAC a 350uA
	 * Frecuencia de conversion p/ bias en True es de 400KHz */
	DAC_SetBias(LPC_DAC, 1);
}

void configUART(){
    LPC_SC->PCONP |= (1 << 25);  // UART3 ON

    // Configurar PCLK para UART3
    CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_UART3, CLKPWR_PCLKSEL_CCLK_DIV_4);

    UART_CFG_Type uart;
    UART_ConfigStructInit(&uart); // Inicialización con valores predeterminados

    // Configurar los parámetros específicos
    uart.Baud_rate = 4;            // 4 baudios
    uart.Databits = UART_DATABIT_8; // 8 bits por dato
    uart.Parity = UART_PARITY_NONE; // Sin bit de paridad
    uart.Stopbits = UART_STOPBIT_1; // 1 bit de confirmación (stop)

    // Inicializar UART3
    UART_Init(LPC_UART3, &uart);

    UART_FIFO_CFG_Type fifo;
    UART_FIFOConfigStructInit(&fifo); // Inicialización con valores predeterminados

    // Configurar FIFO
    fifo.FIFO_DMAMode = DISABLE;
    fifo.FIFO_Level = UART_FIFO_TRGLEV0; // Nivel de disparador de FIFO (trigger level)
    fifo.FIFO_ResetRxBuf = ENABLE;       // Resetear buffer RX
    fifo.FIFO_ResetTxBuf = ENABLE;       // Resetear buffer TX

    // Aplicar configuración FIFO
    UART_FIFOConfig(LPC_UART3, &fifo);

    // Habilitar transmisión
    UART_TxCmd(LPC_UART3, ENABLE);
}


void configPin(){
	PINSEL_CFG_Type pin;

	// Sensor de Temperatura LM35 - ADC00
	pin.Portnum = 0;
	pin.Pinnum = 23;
	pin.Funcnum = 1;
	pin.Pinmode = PINSEL_PINMODE_PULLDOWN;

	PINSEL_ConfigPin(&pin);

	// Match Pin MATCH 0.1
	pin.Portnum = 1;
	pin.Pinnum = 29;
	pin.Funcnum = 3;

	PINSEL_ConfigPin(&pin);

	// Output DAC - DAC Pin P0.26
	pin.Portnum = 0;
	pin.Pinnum = 26;
	pin.Funcnum = 2;

	PINSEL_ConfigPin(&pin);

	// UART3 TX - Pin P0.25
	pin.Portnum = 0;
	pin.Pinnum = 25;
	pin.Funcnum = 3;

	PINSEL_ConfigPin(&pin);
}

// Interruptions

/*void ADC_IRQHandler(){
	i++;
	ADC_ChannelGetData(LPC_ADC, 0);
	ADC_ChannelGetStatus(LPC_ADC, 0, ADC_DATA_DONE);
}*/

void TIMER0_IRQHandler(){
	j++;
	//ADC_StartCmd(LPC_ADC, ADC_START_NOW);
	pinMatch = (LPC_TIM0->EMR & 2) >> 1;
	if (pinMatch == 0){
		adcv = (LPC_ADC->ADDR0 & 4095<<4)>>4;
		volts = 3.3 * adcv / 4095;
		temperature = volts / 0.01 - 2;
		//dac = volts * 1024 / 0.42; // Convertimos volts valor del dac
		if (temperature < 27) {
			dac = 0;
		} else if(temperature < 45) {
			dac = 600;
		}else{
			dac = 1023;
		}

		//uint8_t serial_temperature = temperature;
		uint8_t valor = 0b10000000;
		UART3_SendByte(valor);
		DAC_UpdateValue(LPC_DAC, dac);
	}else{
		if (temperature > 27 && temperature < 45){
			DAC_UpdateValue(LPC_DAC, 0);
		}
	}

	TIM_ResetCounter(LPC_TIM0);
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

}


void UART3_SendByte(uint8_t data) {
    // Buffer para enviar
    uint8_t txBuffer[1] = {data};

    // Enviar 1 byte
    UART_Send(LPC_UART3, txBuffer, 1, BLOCKING);
}
