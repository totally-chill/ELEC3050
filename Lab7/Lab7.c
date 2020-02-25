/*====================================================*/
/* Chris Hill & Mason Greathouse    		              */
/* ELEC 3040/3050 - Lab 7	                            */
/* PWM with timer                                     */
/*====================================================*/

/* Microcontroller information */
#include "STM32L1xx.h"

/* Define global variables */
unsigned char keyPressed;
unsigned char matrix_keypad[4][4] = {{1, 2, 3, 0xA},{4, 5, 6, 0xB},{7, 8, 9, 0xC},{0xE, 0, 0xF, 0xD}};
unsigned char keyEvent = 0;
/* Constants */
const uint16_t psc_value = 0x08;		//For 2.097MHZ clk, ~1ms //maybe change to E9 if not close enough. ~9x233
const uint16_t arr_value = 0xE8;		//For 2.097MHZ clk, ~1ms
//const uint16_t arr_value = 0x1062;		//100Hz
//const uint16_t psc_value = 0x4;		//100Hz
//const uint16_t psc_value = 0x1;		//10kHz
//const uint16_t arr_value = 0x68;		//10kHz
const uint8_t clear_keypress = 0xFF;


/* Array of masks to set/read the specific bits in BSRR/IDR */
//Column mask values correspond to BR bits of BSRR
const int column_mask[] = {0x10 << 16, 0x20 << 16, 0x40 << 16, 0x80 << 16};

//Row mask values correspond to bits of IDR
const int row_mask[] = {0x1, 0x2, 0x4, 0x8};

/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA1 = Keypad Interrupt */
/* PC3-0 = Led 3-0 */
/* PB7-4 = Keypad rows */
/* PB3-0 = Keypad columns */
/*---------------------------------------------------*/
void PinSetup(){
	/* Configure PA1 as input pins to read push button */
	RCC->AHBENR |= 0x01; 		// Enable GPIOA clock (bit 0)
	GPIOA->MODER &= ~(0x0000000C); 	// General purpose input mode PA1
	
	/* Configure PA6 for AF mode for TIM10 */
	GPIOA->MODER &= ~(0x00003000);	//Clear PA6 mode
	GPIOA->MODER |= 0x00002000;	//Alternate Functio mode PA6
	
	GPIOA->AFR[0] &= ~(0x0F000000); //Clear AFR for PA6
	GPIOA->AFR[0] |= 0x03000000;	//Set AFR to be TIM10_CH1
	
	/* Configure PC3-0 as output pins to drive LEDs */
	RCC->AHBENR |= 0x04; 		//Enable GPIOC clock (bit 2)
	GPIOC->MODER &= ~(0x000000FF); 	//Clear PC3-PC0 mode bits
	GPIOC->MODER |= 0x00000055; 	//General purpose output mode PC3-PC0
	
	/* KEYPAD I/O CONFIGURATION */
	/* Configure PB7-4 as output pins to control keypad columns */
	RCC->AHBENR |= 0x02; 		//Enable GPIOB clock (bit 1)
	GPIOB->MODER &= ~(0x0000FF00); 	//Clear PB7-4 mode bits for general purpose input
	GPIOB->MODER |= 0x00005500;	// General purpose output for PB7-4
	
	/* Configure PB3-0 as input to read keypad rows */
	GPIOB->MODER &= ~(0x000000FF); 	//Clear PB3-0 mode bits
	
	/* Configure Pull-Up resistor for keypad rows */
	GPIOB->PUPDR &= ~(0x000000FF);
	GPIOB->PUPDR |= 0x00000055;
}


/* Interrupt Setup */
void InterruptSetup(){
	SYSCFG->EXTICR[0] &= ~(0x00F0); //Clear EXTI1 (set for PA1)
	
	EXTI->FTSR |= 0x0002;	//Set falling trigger for PA1
	EXTI->IMR |= 0x0002; 	//Enable interrupt PA1
	EXTI->PR |= 0x002; 	//Clear pending
	
	NVIC_EnableIRQ(EXTI1_IRQn); 		//Enable interrupt for PA1
	NVIC_ClearPendingIRQ(EXTI1_IRQn);	//Clear NVIC pending for PA1
	
	NVIC_EnableIRQ(TIM10_IRQn);		//Enable timer interrupt in NVIC
	NVIC_ClearPendingIRQ(TIM10_IRQn); 	//Clear pending interrupt in NVIC
}

/* Timer Setup */
void TimerSetup() {
	RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;	//Enable TIM10
	TIM10->PSC = psc_value;			//Set timer prescale
	TIM10->ARR = arr_value;			//Set timer auto reload
	TIM10->CCR1 = 0x0;			//Set compare value for duty cycle
	//Set up output compare for TIM10_CH1
	TIM10->CCMR1 &= ~(0x0F);	//Clear register. Sets to output compare
	TIM10->CCMR1 |= 0x60;		//Set Output Compare to PWM1 mode
	
	TIM10->CCER &= ~(0x03);		//Clear CH1 CC1E and CC1P.  CC1P now Active High
	TIM10->CCER |= 0x01;		//Set CC1E to drive output pin
	
	TIM10->CR1 |= TIM_CR1_CEN;	//Enable counting on timer 10
}


/* Update LEDs based on value (only dealing with LED3-0) */
void UpdateLED(unsigned char value){
	GPIOC->ODR = value;	//Set LEDs based on button
}

/* Short delay to allow keypad to update values */
void short_delay(){
	int i,n;
	for(i=0; i<5; i++){ 
		n = i; //dummy operation
	}
}

/* Interrupt Service Routine */
void EXTI1_IRQHandler(){
	__disable_irq();	//Prevent other interrupts during interrupt
	
	for(int i = 0; i < 4; i++){
		GPIOB->BSRR |= 0xF0;		//Pull all columns high PB7-4
		GPIOB->BSRR |= column_mask[i];	//Pulls down column i 
		short_delay();
		
		for(int j = 0; j < 4; j++){
			unsigned char read_row = GPIOB->IDR & row_mask[j];	//Read row j from IDR
			
			if(read_row == 0){
				keyPressed = matrix_keypad[j][i];
				if(keyPressed < 11){
					UpdateLED(keyPressed);		//Update LEDs based on key
				}
				keyEvent = 1;
				GPIOB->BSRR |= 0x00F00000;		//Ground columns. PB7-4
				EXTI->PR |= 0x002;			//Clear pending
				NVIC_ClearPendingIRQ(EXTI1_IRQn);	//Clear pending
				__enable_irq();				//Re-enable interrupts
				return;
			}
		}
	}
	
	//Need to add some delay for bounce
	GPIOB->BSRR |= 0x00F00000;		//Ground columns. PB7-4
	EXTI->PR |= 0x002; 			//Clear pending
	NVIC_ClearPendingIRQ(EXTI1_IRQn);	//Clear NVIC pending
	__enable_irq();				//Re-enable interrupts
}

/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
	PinSetup();		//Configure GPIO pins
	InterruptSetup();	//Configure Interrupts
	TimerSetup();		//Configure Timer
	
	keyPressed = clear_keypress;	//Initialize keyPressed to value not on keyboard
	GPIOB->BSRR |= 0x00F00000;	//Ground columns. PB7-4
	__enable_irq();             	//Enable CPU interrupts
	
	int dutyCycle = 0;
	/* Endless loop */
	while (1) {
		if(keyEvent == 1 && keyPressed < 11){
			dutyCycle = keyPressed * (TIM10->ARR + 1) / 10;	//Calculate duty cycle from keypress
			TIM10->CCR1 = dutyCycle;												//Set Compare to duty cycle value
			keyEvent = 0;																		//Reset event.  Hopefully this can help with bounce
		}
	} /* repeat forever */
}
