/*====================================================*/
/* Chris Hill & Mason Greathouse     */
/* ELEC 3040/3050 - Lab 3, Program 1 */
/* 2 decade counters                 */
/*====================================================*/

#include "STM32L1xx.h" /* Microcontroller information */

/* Define global variables */
int count1,count2;

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
	GPIOC->MODER &= ~(0x0000FFFF); /* Clear PC7-PC0 mode bits */
	GPIOC->MODER |= (0x00005555); /* General purpose output mode PC3-PC0*/
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
	uint16_t output;
	
	if(direction == 0){	//Count1 down, Count2 up
		count1--;
		count2++;
		if(count1 < 0){
			count1 = 9;			//Wrap around count1
		}
		
		if(count2 > 9){
			count2 = 0;			//Wrap around count2
		}
		
	} else { 						//Count1 up, Count2 down
		count1++;
		count2--;
		if(count1 > 9){
			count1 = 0;			//Wrap around count1
		}
		
		if(count2 < 0){
			count2 = 9;			//Wrap around count2
		}
	}
	
	/* Set LEDs based on current value of count */
	output = (count2 & 0x0F) << 4;
	output += (count1 & 0x0F);
	GPIOC->ODR = output;
}

/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
	unsigned char sw1; 	//state of SW1
	unsigned char sw2; 	//state of SW2
	PinSetup(); 				//Configure GPIO pins
	count1 = 0; 				//Initialize count1
	count2 =0; 					//Initialize count2
	
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
