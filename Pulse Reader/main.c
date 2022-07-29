#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"


void TIM3_IRQHandler(void);
void TIM2_IRQHandler(void);
uint16_t Rise = 0;
uint16_t Fall = 0;
uint16_t Freq = 0;
uint16_t Duty = 0;
uint16_t Ov = 0;
uint16_t fl = 0;

void TIM2_IRQHandler(void)
{
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);
		fl=0;
	  TIM_ClearFlag(TIM2, TIM_FLAG_Update);
		TIM_Cmd(TIM2, DISABLE);
}

void TIM3_IRQHandler(void){
	uint16_t Temp;
	if(TIM_GetFlagStatus(TIM3, TIM_IT_CC2) == SET)
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_11);
		Temp = TIM_GetCapture2(TIM3);
		Freq = (256*1000)/(256*Ov+Temp-Rise); 
		Rise = Temp;
		Ov = 0;
		if(Freq>100 && fl==0){
			fl = 1;
			GPIO_SetBits(GPIOA, GPIO_Pin_2);
			TIM_Cmd(TIM2, ENABLE);
		}
	}
	else if(TIM_GetFlagStatus(TIM3, TIM_IT_CC1) == SET)
	{
		Temp = TIM_GetCapture1(TIM3);
		Fall = 256*Ov+Temp-Rise;
	}
	else //Flag Update
	{
		Ov = Ov + 1;
		TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	}
	
}
 

int main(void){
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM2, ENABLE);
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
	TIM_TimeBaseInitTypeDef TIM_InitStruct;
	TIM_OCInitTypeDef TIM_OCInitStruct;
	TIM_ICInitTypeDef TIM_ICInitStruct;
	GPIO_InitTypeDef GPIO_InitStruct;
  NVIC_InitTypeDef NVIC_InitStruct;
	USART_ClockInitTypeDef USART_ClockInitStruct;
	
	// PA15 Reset
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA,&GPIO_InitStruct);
	GPIO_ResetBits(GPIOA, GPIO_Pin_15);
  
  //PA7(PWM1): Capture
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA,&GPIO_InitStruct);
	
	//PA2(Tx), PA9(PWM2: Output)
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_11 | GPIO_Pin_13;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA,&GPIO_InitStruct);
	
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA,&GPIO_InitStruct);
	
	//Clock
	USART_ClockInitStruct.USART_Clock = USART_Clock_Disable;
  USART_ClockInitStruct.USART_CPOL = USART_CPOL_Low;
  USART_ClockInitStruct.USART_CPHA = USART_CPHA_1Edge;
  USART_ClockInitStruct.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(USART2, &USART_ClockInitStruct);
	
	//Interrupt
	//TIM3
  NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStruct);
	//TIM2
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x02;
	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_Init(&NVIC_InitStruct);
	
	//TIM1 Basic Config
	TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStruct.TIM_Prescaler = 120 - 1; // Freq = 720000/Prescaler - 1
	TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_InitStruct.TIM_Period = 100 - 1; // Auto Reload (Period = 100)
	TIM_TimeBaseInit(TIM1, &TIM_InitStruct);
	
	
	//TIM1 Output Compare Config (A9; PWM2)
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_Pulse = 42; // CCR Register value (Pulse width)
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC2Init(TIM1, &TIM_OCInitStruct);
	
	//TIM2 Basic Config (UART Transimt Timing)
	TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStruct.TIM_Prescaler = 600 - 1; // Freq = 1 Hz
	TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_InitStruct.TIM_Period = 10000 - 1; 
	TIM_TimeBaseInit(TIM2, &TIM_InitStruct);
	
	//TIM3 Basic Config (Capture)
	//Freq = 1KHz
	TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStruct.TIM_Prescaler = 23 - 1; // = 72000/256 - 1
	TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_InitStruct.TIM_Period = 256 - 1; // Auto Reload
	TIM_TimeBaseInit(TIM3, &TIM_InitStruct);
	
	//TIM3 Input Capture Config, A7(PWM1)
	TIM_ICInitStruct.TIM_Channel = TIM_Channel_2; //IC2(Freq)
  TIM_ICInitStruct.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStruct.TIM_ICFilter = 0x00;
	TIM_ICInit(TIM3, &TIM_ICInitStruct);
	
	TIM_ICInitStruct.TIM_Channel = TIM_Channel_1; //IC1(Duty)
  TIM_ICInitStruct.TIM_ICPolarity = TIM_ICPolarity_Falling;
  TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_IndirectTI;
	TIM_ICInit(TIM3, &TIM_ICInitStruct);
	
	
	TIM_ITConfig(TIM3, TIM_IT_Update | TIM_FLAG_CC1 | TIM_FLAG_CC2, ENABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	TIM_Cmd(TIM1, ENABLE);
  USART_ITConfig(USART2, USART_IT_TC, ENABLE);
	
  while(1){
		GPIO_SetBits(GPIOA, GPIO_Pin_13);
		GPIO_SetBits(GPIOA, GPIO_Pin_11);
  }
}
