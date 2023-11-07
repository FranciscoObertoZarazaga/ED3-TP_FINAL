#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_clkpwr.h"
#endif

#include <cr_section_macros.h>

uint32_t i = 0;
uint32_t j = 0;
uint32_t pinMatch;

void configADC();
void configPin();
void configTimer();

int main(void) {
	configPin();
	configADC();
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
	match.MatchValue = 24999999;
	TIM_ConfigMatch(LPC_TIM0, &match);
	LPC_TIM0->EMR |= 3<<6;

	/*TIM_TIMERCFG_Type tim;
	tim.PrescaleOption = TIM_PRESCALE_TICKVAL;
	tim.PrescaleValue = 0;
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &tim);*/
	LPC_TIM0->PR = 0;

	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	NVIC_EnableIRQ(TIMER0_IRQn);

	TIM_Cmd(LPC_TIM0, ENABLE);
	TIM_ResetCounter(LPC_TIM0);
}

void configADC(){
	ADC_Init(LPC_ADC, 200000); // Set 200KHz ADC Frequency
	ADC_ChannelCmd(LPC_ADC, 0, ENABLE); // ADC Channel 0
	ADC_BurstCmd(LPC_ADC, DISABLE); // Burst Mode OFF
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, SET); // Interruption Channel 0 ON
	ADC_ChannelGetStatus(LPC_ADC, 0, ADC_DATA_DONE); // Clean Flag
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01); // Start ADC Ch0 in Burst Mode
	NVIC_EnableIRQ(ADC_IRQn); // Enable NVIC
}

void configPin(){
	PINSEL_CFG_Type pin;

	// Sensor de Temperatura LM35 - ADC00
	pin.Portnum = 0;
	pin.Pinnum = 23;
	pin.Funcnum = 1;
	pin.Pinmode = PINSEL_PINMODE_PULLDOWN;

	PINSEL_ConfigPin(&pin);

	// Match Pin
	pin.Portnum = 1;
	pin.Pinnum = 29;
	pin.Funcnum = 3;

	PINSEL_ConfigPin(&pin);
}

// Interruptions

void ADC_IRQHandler(){
	i++;
	ADC_ChannelGetData(LPC_ADC, 0);
	ADC_ChannelGetStatus(LPC_ADC, 0, ADC_DATA_DONE);
}

void TIMER0_IRQHandler(){
	j++;
	//ADC_StartCmd(LPC_ADC, ADC_START_NOW);
	pinMatch = LPC_TIM0->EMR & 2;
	TIM_ResetCounter(LPC_TIM0);
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}
