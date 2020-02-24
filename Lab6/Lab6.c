/*====================================================*/
/* Chris Hill & Mason Greathouse    		      */
/* ELEC 3040/3050 - Lab 6	                      */
/* Stopwatch with internal interrupts                 */
/*====================================================*/

/* Microcontroller information */
#include "STM32L1xx.h"

/* Define global variables */
int secondCounter;
int decisecondCounter;
unsigned char keyPressed;
unsigned char matrix_keypad[4][4] = {{1, 2, 3, 0xA},{4, 5, 6, 0xB},{7, 8, 9, 0xC},{0xE, 0, 0xF, 0xD}};
	
/* Constants */
const uint16_t psc_value = 0x1FF;
const uint16_t arr_value = 0x198;
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
	
	/* Configure PC7-0 as output pins to drive LEDs */
	RCC->AHBENR |= 0x04; 		//Enable GPIOC clock (bit 2)
	GPIOC->MODER &= ~(0x0000FFFF); 	//Clear PC3-PC0 mode bits
	GPIOC->MODER |= 0x00005555; 	//General purpose output mode PC7-PC0
	
	/* KEYPAD I/O CONFIGURATION */
	/* Configure PB7-4 as output pins to control keypad columns */
	RCC->AHBENR |= 0x02; 		//Enable GPIOB clock (bit 1)
	GPIOB->MODER &= ~(0x0000FF00); 	// Clear PB7-4 mode bits for general purpose input
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
	TIM10->DIER |= TIM_DIER_UIE;		//Enable timer interrupt
}

/* Short delay to allow keypad to update values */
void short_delay(){
	int i,n;
	for(i=0; i<5; i++){ 
		n = i; //dummy operation
	}
}

/* Reset stopwatch */
void ResetWatch(){
	secondCounter = 0;
	decisecondCounter = 0;
	GPIOC->BSRR |= 0xFF << 16;	//Clear PC7-0
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
				GPIOB->BSRR |= 0x00F00000;		//Ground columns. PB7-4
				EXTI->PR |= 0x002;			//Clear pending
				NVIC_ClearPendingIRQ(EXTI1_IRQn);   	//Clear pending
				__enable_irq();				//Re-enable interrupts
				return;
			}
		}
	}
	
	GPIOB->BSRR |= 0x00F00000;		//Ground columns. PB7-4
	EXTI->PR |= 0x002; 			//Clear pending
	NVIC_ClearPendingIRQ(EXTI1_IRQn);	//Clear NVIC pending
	__enable_irq();				//Re-enable interrupts
}

//Timer 10 interrupt handler
void TIM10_IRQHandler(){
	__disable_irq();	//Disable CPU interrupts
	
	decisecondCounter = (decisecondCounter + 1) % 10;	//Count up 0-9
	GPIOC->BSRR |= 0x0F << 16;				//Clear PC3-0
	GPIOC->BSRR |= decisecondCounter;			//Set PC3-0
	
	if(decisecondCounter == 0){
		secondCounter = (secondCounter + 1) % 10; 	//Count up 0-9
		GPIOC->BSRR |= 0xF0 << 16; 			//Clear PC7-4
		GPIOC->BSRR |= secondCounter << 4;		//Set PC7-4
	}
	
	TIM10->SR &= ~TIM_SR_UIF;		//Clear timer update flag
	NVIC_ClearPendingIRQ(TIM10_IRQn);	//Clear NVIC pending
	__enable_irq();				//Enable CPU interrupts
}
/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
	PinSetup();		//Configure GPIO pins
	InterruptSetup();	//Configure Interrupts
	TimerSetup();		//Configure Timer
	
	secondCounter = 0; 		//Initialize count1
	decisecondCounter = 0;		//Initialize count2
	keyPressed = clear_keypress;	//Initialize keyPressed to value not on keyboard
	GPIOB->BSRR |= 0x00F00000;	//Ground columns. PB7-4
	__enable_irq();             	//Enable CPU interrupts
	
	unsigned char isCounting = 0;
	/* Endless loop */
	while (1) {
		isCounting = TIM10->CR1 & TIM_CR1_CEN; //Is the counter enabled?
		if(!isCounting && keyPressed == 0){
			TIM10->CR1 |= TIM_CR1_CEN;	//Enable counting
			keyPressed = clear_keypress;
		} else if(isCounting && keyPressed == 0){
			TIM10->CR1 &= ~(TIM_CR1_CEN);	//Disable counting
			keyPressed = clear_keypress;
		} else if(!isCounting && keyPressed == 1){
			ResetWatch();
			keyPressed = clear_keypress;
		}
	} /* repeat forever */
}
