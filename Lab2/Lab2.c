/*====================================================*/
/* Chris Hill & Mason Greathouse */
/* ELEC 3040/3050 - Lab 2, Program 1 */
/* Count up/down from 0-9 based on input switches */
/*====================================================*/

#include "STM32L1xx.h" /* Microcontroller information */

/* Define global variables */
int count;

/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA1 = Switch 1  */
/* PA2 = Switch 2  */
/* PC3-0 = Led 3-0 */
/*---------------------------------------------------*/
void PinSetup () {
	/* Configure PA0 as input pin to read push button */
	RCC->AHBENR |= 0x01; /* Enable GPIOA clock (bit 0) */
	GPIOA->MODER &= ~(0x0000003C); /* General purpose input mode PA1 & PA2 */
 
	/* Configure PC8,PC9 as output pins to drive LEDs */
	RCC->AHBENR |= 0x04; /* Enable GPIOC clock (bit 2) */
	GPIOC->MODER &= ~(0x000000FF); /* Clear PC3-PC0 mode bits */
	GPIOC->MODER |= (0x00000055); /* General purpose output mode PC3-PC0*/
}

/*----------------------------------------------------------*/
/* Delay function - do nothing for about 0.5 second */
/*----------------------------------------------------------*/
void delay () {
	int i,j,n;
	for (i=0; i<20; i++) { //outer loop
		for (j=0; j<10000; j++) { //inner loop
			n = j; //dummy operation for single-step test
		} //do nothing
	}
}

void counter(unsigned char direction){
	if(direction == 0){	//Count down
		count--;
		if(count < 0){
			count = 9;			//Wrap around
		}
		
	} else { 						//Count up
		count++;
		if(count > 9){
			count = 0;			//Wrap around
		}
	}
	
	/* Set LEDs based on current value of count */
	switch(count){
		case 0:
			GPIOC->ODR = 0x00000000;
			break;
		case 1:
			GPIOC->ODR = 0x00000001;
			break;
		case 2:
			GPIOC->ODR = 0x00000002;
			break;
		case 3:
			GPIOC->ODR = 0x00000003;
			break;
		case 4:
			GPIOC->ODR = 0x00000004;
			break;
		case 5:
			GPIOC->ODR = 0x00000005;
			break;
		case 6:
			GPIOC->ODR = 0x00000006;
			break;
		case 7:
			GPIOC->ODR = 0x00000007;
			break;
		case 8:
			GPIOC->ODR = 0x00000008;
			break;
		case 9:
			GPIOC->ODR = 0x00000009;
			break;
	}
}

/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
	unsigned char sw1; //state of SW1
	unsigned char sw2; //state of SW2
	PinSetup(); //Configure GPIO pins
	count = 0; //Initialize count
	
	/* Endless loop */
	while (1) {
		sw1 = GPIOA->IDR & 0x02; //Read GPIOA and mask all but bit 1
		
		/* Wait in loop until SW1 pressed */
		while (sw1 == 0) { //Wait for SW1 = 1 on PA1
			sw1 = GPIOA->IDR & 0x02; //Read GPIOA and mask all but bit 1
		}
		
		sw2 = GPIOA->IDR & 0x04; //Read GPIOA and mask all but bit 2
		
		counter(sw2); //Count up or down based on value of SW2
		
		delay(); //Time delay for button release
	} /* repeat forever */
}
