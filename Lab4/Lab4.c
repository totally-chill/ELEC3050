/*====================================================*/
/* Chris Hill & Mason Greathouse     */
/* ELEC 3040/3050 - Lab 4, Program 1 */
/* 2 decade counters                 */
/*====================================================*/

#include "STM32L1xx.h" /* Microcontroller information */

/* Define global variables */
int count1,count2;
unsigned char direction;

/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA0 = User button on board  */
/* PA1 = Static I/0 button  */
/* PC7-0 = Led 7-0 */
/*---------------------------------------------------*/
void PinSetup(){
	/* Configure PA0 & PA1 as input pins to read push button */
	RCC->AHBENR |= 0x01; /* Enable GPIOA clock (bit 0) */
	GPIOA->MODER &= ~(0x0000000F); /* General purpose input mode PA0 & PA1 */
 
	/* Configure PC8,PC9 as output pins to drive LEDs */
	RCC->AHBENR |= 0x04; /* Enable GPIOC clock (bit 2) */
	GPIOC->MODER &= ~(0x000FFFFF); /* Clear PC9-PC0 mode bits */
	GPIOC->MODER |= (0x00055555); /* General purpose output mode PC9-PC0*/
}


//Interrupt Setup
void InterruptSetup(){
	SYSCFG->EXTICR[0] &= ~(0x000F); //Clear EXTI0 (set for PA0)
	SYSCFG->EXTICR[0] &= ~(0x00F0); //Clear EXTI1 (set for PA1)
	
	EXTI->RTSR |= 0x0001; //Set rising trigger for PA0
	EXTI->RTSR |= 0x0002; //Set rising trigger for PA1
	
	EXTI->IMR |= 0x0001; //Enable interrupt PA0
	EXTI->IMR |= 0x0002; //Enable interrupt PA1
	
	EXTI->PR |= 0x001; //Clear pending
	EXTI->PR |= 0x002; //Clear pending
	
	NVIC_EnableIRQ(EXTI0_IRQn); //Enable interrupt for PA0
	NVIC_EnableIRQ(EXTI1_IRQn); //Enable interrupt for PA1
	
	NVIC_ClearPendingIRQ(EXTI0_IRQn); //Clear NVIC pending for PA0
	NVIC_ClearPendingIRQ(EXTI1_IRQn); //Clear NVIC pending for PA1
	
}

/*----------------------------------------------------------*/
/* Delay function - do nothing for about 0.5 second */
/*----------------------------------------------------------*/
void delay() {
	int i,j,n;
	for (i=0; i<20; i++) { //outer loop
		for (j=0; j<8750; j++) { //inner loop
			n = j; //dummy operation for single-step test
		} //do nothing
	}
}

//Counter for LED3-LED0
void counter1(){
	count1++;
	if(count1 > 9){
		count1 = 0;
	}
	/* Set LEDs based on current value of count */
	GPIOC->ODR |= (count1 & 0x0F);
}

//Counter for LED7-LED4
void counter2(){
	if(direction == 0){
		count2--;
		if(count2 < 0){
			count2 = 9;
		}
	} else {
		count2++;
		if(count2 > 9){
			count2 = 0;
		}
	}
	
	GPIOC->ODR |= (count2 & 0x0F) << 4;
	
}

//Interrupt Service Routine
void EXTI0_IRQHandler(){
	direction = 0;
	GPIOC->BSRR = 0x0100;							//Turn on PC8
	GPIOC->BSRR = 0x0200 << 16; 			//Turn off PC9
	EXTI->PR |= 0x001; 								//Clear pending
	NVIC_ClearPendingIRQ(EXTI0_IRQn); //Clear pending
}

void EXTI1_IRQHandler(){
	direction = 1;
	GPIOC->BSRR = 0x0100 << 16;				//Turn off PC8
	GPIOC->BSRR = 0x0200; 						//Turn on PC9
	EXTI->PR |= 0x002; 								//Clear pending
	NVIC_ClearPendingIRQ(EXTI1_IRQn); //Clear pending
}

/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
	PinSetup(); 				//Configure GPIO pins
	InterruptSetup();		//Configure Interrupts
	count1 = 0; 				//Initialize count1
	count2 =0; 					//Initialize count2
	direction = 1;			//Initialize direction to count up
	
	__enable_irq();
	
	/* Endless loop */
	while (1) {
		counter1();
		counter2();
		delay();
		counter1(); //Go ahead and update so that counter1 experiences half of the total delay.
		delay();
	} /* repeat forever */
}
