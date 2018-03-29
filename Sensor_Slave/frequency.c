/*

Using 16-bit Timer/Counter1 to evaluate (flow meter) square wave frequencies directly.
Connect signal pin ot ICP1 (PB0)
Using 1Mhz internal clock speed

*/
//1MHz Internal Cyrstal 
#define F_CPU 1000000
#define BaudRate 9600
#define MYUBRR F_CPU/16/BaudRate-1

// ------- Preamble -------- //
#include <avr/io.h>         /* Defines pins, ports, etc */
#include <util/delay.h>     /* Functions to waste time */
#include <avr/interrupt.h>
#include "pinDefines.h"
#include "USART.h"
#include <stdio.h>
#include <stdlib.h>        // avr method for dtostrf()

volatile unsigned long rising_edge, falling_edge, ov_counter;
volatile unsigned long pulse_clocks,counter1;
volatile float frequency;
volatile int statusCounter, countCounter;
char TCVal1[10];
unsigned char data,dataloc;
 
//Define Methods//
void USART_Init(unsigned int ubrr);
void USART_Transmit(unsigned char data3);
void send_string(char s[]);
unsigned char USART_Receive(void);
void spi_init_slave(void);
unsigned char spi_tranceiver (unsigned char data);
unsigned char Stringfloat(float floatvalue);
static inline void initTimer(void);
ISR(TIMER1_CAPT_vect);
ISR(TIMER1_OVF_vect);


int main(void) {
// -------- Inits --------- //
	initTimer();


	DDRD = 0b10000000;
	//Setting all PINB to input
	DDRB = 0x00;  
	spi_init_slave();
	statusCounter = 0;
	countCounter = 0;

// ------ Event loop ------ // 
	while (1) {
		
		dataloc = spi_tranceiver('a');
		
		if (dataloc == '1'){
/* 			data = Stringfloat(frequency);  //SPI */
			data = Stringfloat(101.24);     //SPI
			frequency = 0;
		}
		else if(dataloc == '2'){
			data = Stringfloat(356.98);
		}
		else if(dataloc == '3'){
			data = Stringfloat(98.54);
		}
	}
	return 0;                  
}


//***This group of subprograms will be for USART (SERIAL) Communication***

//Initialize USART
void USART_Init(unsigned int ubrr)
{
  //Set baud rate
  UBRR0H = (unsigned char)(ubrr>>8);
  UBRR0L = (unsigned char)ubrr;
  //Enable reciever and transmetter
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  //Set frame format: 8data, 1stop bit
  UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

void USART_Transmit(unsigned char data3)
{
  //Wait for empty transmit buffer
  while (!(UCSR0A & (1<<UDRE0)));

  //Put data into buffer, sends the data
  UDR0 = data3;
}

unsigned char USART_Receive(void)
{
  //Wait for data to be recieved
  while (!(UCSR0A & (1<<RXC0)));

  //Get and return recieved data from buffer
  return UDR0;
}

//Constructs the character array to be sent by USART
void send_string(char s[])
{
   int i=0;
   while (s[i] !=0x00)
   {
      USART_Transmit(s[i]);
      i++;
   }
}
//*************************************************************


//***This group of subprograms will be for SPI Communication***

void spi_init_slave(void){
    DDRB=(1<<4);                                 //MISO as OUTPUT
    SPCR=(1<<SPE);                               //Enable SPI
}
 
//Function to send and receive data
unsigned char spi_tranceiver (unsigned char data){
    SPDR = data;                                  //Load data into buffer
    while(!(SPSR & (1<<SPIF) ));                  //Wait until transmission complete
    return(SPDR);                                 //Return received data
}


unsigned char Stringfloat(float floatvalue){
 	//convert float to char[] array
	static char charray[8] = {'\0'};
	static unsigned char data2;
	dtostrf(floatvalue,5,2,charray);
	
	//check if charray from dtostrf float conversion, if any within the [8] are null.
	for(int i=0; i<8;i++){
		if (charray[i] == '\0'){
			charray[i] = ' ';
		}
	}

 	//use pointer and typecast to convert char[] (from above) to uint_t
 	char *charPtr = charray;
	
	for(int i=0; i<8;i++){
		data2 = spi_tranceiver((uint8_t)*charPtr++);
	}
	
	return 0; /* data2 */;
}
//**************************************************************


static inline void initTimer(void) {
	/*Disable  all waveform and timer counter mode to Normal (max at 65535)*/
	TCCR1A = 0x00; 
	/* CPU clock / 8 */	
	TCCR1B = (1<<ICNC1)|(1<<ICES1)|(1 << CS11);
	/*sets Input Cap. and OCIE1A interrupt*/
	TIMSK1 = (1<<ICIE1)|(1<<TOIE1); 
	// initialize counter
	TCNT1 = 0;
	// initialize overflow counter variable
	ov_counter = 0;
	// enable global interrupts
	sei();
}

ISR(TIMER1_CAPT_vect){
	cli();
	
	if(counter1 >= 3){
	
		if(statusCounter == 0){
			rising_edge = ICR1;
			ov_counter = 0;
			statusCounter = 1;
		}
		else{
			falling_edge = ICR1;
			pulse_clocks = ((falling_edge+(ov_counter*65535))-rising_edge);
			frequency = 1/(pulse_clocks*0.000008);
			statusCounter = 0;
			counter1=0;
			countCounter=1;
		}
	}
	sei();
}
	

ISR(TIMER1_OVF_vect){
	ov_counter++;
	counter1++;
}
