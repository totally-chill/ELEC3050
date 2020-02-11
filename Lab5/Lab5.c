/*====================================================*/
/* Chris Hill & Mason Greathouse     */
/* ELEC 3040/3050 - Lab 5, Program 1 */
/* Matrix keypad interrupts          */
/*====================================================*/

#include "STM32L1xx.h" /* Microcontroller information */

/* Define global variables */
int count;

/* Define structure for keypad. */
/* Keypad will be a 2-dimensional array to determine key value */
struct{
	//unsigned char row;
	//unsigned char column;
	unsigned char display_delay;
	const unsigned char row1[4];
	const unsigned char row2[4];
  const unsigned char row3[4];
	const unsigned char row4[4];
	const unsigned char* keys[];
}typedef keypad;

/* Construct instance of keypad */
keypad keypad1 = {
		//.row = ~0,
		//.column = ~0,
		.display_delay = 0,
		.row1 = {1, 2, 3, 0xA},
		.row2 = {4, 5, 6, 0xB},
		.row3 = {7, 8, 9, 0xC},
		.row4 = {0xE, 0, 0xF, 0xD},
		.keys = {keypad1.row1, keypad1.row2, keypad1.row3, keypad1.row4}
};

/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA1 = Keypad Interrupt */
/* PC3-0 = Led 3-0 */
/* PB7-4 = Keypad rows */
/* PB3-0 = Keypad columns */
/*---------------------------------------------------*/
void PinSetup(){
	/* Configure PA1 as input pins to read push button */
	RCC->AHBENR |= 0x01; /* Enable GPIOA clock (bit 0) */
	GPIOA->MODER &= ~(0x0000000C); /* General purpose input mode PA1 */
	
	/* Configure PC3-0 as output pins to drive LEDs */
	RCC->AHBENR |= 0x04; /* Enable GPIOC clock (bit 2) */
	GPIOC->MODER &= ~(0x000000FF); /* Clear PC3-PC0 mode bits */
	GPIOC->MODER |= 0x00000055; /* General purpose output mode PC9-PC0*/
	
	/* Configure PB7-4 as input pins to read keypad rows */
	RCC->AHBENR |= 0x02; /* Enable GPIOB clock (bit 1) */
	GPIOB->MODER &= ~(0x0000FF00); /* Clear PB7-4 mode bits for general purpose input */
	
	/* Configure PB3-0 as output to read keypad columns*/
	GPIOB->MODER &= ~(0x000000FF); /* Clear PB3-0 mode bits */
	GPIOB->MODER |= 0x00000055; /* General purpose output mode PB3-0 */
	
	/* Configure Pull-Up resistor for keypad rows */
	GPIOB->PUPDR &= ~(0x000000FF);
	GPIOB->PUPDR |= 0x00000055;
	
}


//Interrupt Setup
void InterruptSetup(){
	SYSCFG->EXTICR[0] &= ~(0x00F0); //Clear EXTI1 (set for PA1)
	
	EXTI->FTSR |= 0x0002; //Set rising trigger for PA1
	
	EXTI->IMR |= 0x0002; //Enable interrupt PA1
	
	EXTI->PR |= 0x002; //Clear pending
	
	NVIC_EnableIRQ(EXTI1_IRQn); //Enable interrupt for PA1
	
	NVIC_ClearPendingIRQ(EXTI1_IRQn); //Clear NVIC pending for PA1
	
}

/*----------------------------------------------------------*/
/* Delay function - do nothing for about 0.5 second */
/*----------------------------------------------------------*/
void Delay() {
	int i,j,n;
	for (i=0; i<20; i++) { //outer loop
		for (j=0; j<8750; j++) { //inner loop
			n = j; //dummy operation for single-step test
		} //do nothing
	}
}

/* Short delay to allow keypad to update values */
void short_delay(){
	int i,n;
	for(i=0; i<5; i++){ 
		n = i; //dummy operation
	}
}

/* Increment count variable */
void Counter(){
	count++;
	if(count > 9){
		count = 0;
	}
}

/* Updates LEDS based on count */
void UpdateLED(unsigned char value){
	GPIOC->ODR = value;	//Set output register to value of count
}

/* Interrupt Service Routine */
void EXTI1_IRQHandler(){
	__disable_irq();	//Prevent other interrupts during interrupt
	
	/* Array of masks to set/read the specific bits in BSRR/IDR */
	//Column mask values correspond to BR bits of BSRR
	const int column_mask[] = {0x10 << 16, 		
														 0x20 << 16, 
														 0x40 << 16, 
														 0x80 << 16};
	
	//Row mask values correspond to bits of IDR
	const int row_mask[] = {0x1,
												  0x2,
												  0x4,
												  0x8};
	
	for(int i = 0; i < 4; i++){
		GPIOB->BSRR |= 0xF0;						//Pull all columns high PB7-4
		GPIOB->BSRR |= column_mask[i];	//Pulls down column i 
		short_delay();									
		for(int j = 0; j < 4; j++){
			unsigned char read_row = GPIOB->IDR & row_mask[j]; //Read row j from IDR
			if(read_row == 0){
				keypad1.display_delay = 4;
				UpdateLED(keypad1.keys[j][i]);			//Set LEDs to value at Row j, Column i
				GPIOB->BSRR |= 0x00F00000;					//Ground columns. PB7-4
				EXTI->PR |= 0x002; 									//Clear pending
				NVIC_ClearPendingIRQ(EXTI1_IRQn);   //Clear pending
				__enable_irq();											//Re-enable interrupts
				return;
			}
		}
	}
	
	GPIOB->BSRR |= 0x00F00000;					//Ground columns. PB7-4
	EXTI->PR |= 0x002; 									//Clear pending
	NVIC_ClearPendingIRQ(EXTI1_IRQn);   //Clear pending
	__enable_irq();											//Re-enable interrupts
}

/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
	PinSetup(); 								//Configure GPIO pins
	InterruptSetup();						//Configure Interrupts
	count = 0; 									//Initialize count1
	GPIOB->BSRR |= 0x00F00000;	//Ground columns. PB7-4
	__enable_irq();             //Enable CPU interrupts
	
	/* Endless loop */
	while (1) {
		Counter();
		
		/* Check if display_delay is not 0, which means an interrupt has occurred recently */
		/* This allows counter to continue running, but not be displayed for ~5 seconds */
		if(keypad1.display_delay != 0){
			keypad1.display_delay--;
		} else {
			UpdateLED(count);
		}
		
		/* Delay() has been optimized to be almost exactly 0.5s */
		/* Simply run it twice to be as close as possible to 1s without further testing */
		for(int i = 0; i < 2; i++){
			Delay();
		}
	} /* repeat forever */
}
