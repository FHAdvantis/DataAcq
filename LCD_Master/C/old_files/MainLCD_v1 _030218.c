
// ------- Preamble -------- //
#include <avr/io.h>            // Defines pins, ports, etc
#include <util/delay.h>        // Functions to waste time 
#include <stdlib.h>            //avr method for dtostrf() function for float to charray[]
#include <string.h>            //for strlen():Determine length of char in charray[]
#include <avr/interrupt.h>     // for interrupts


#define _rs_pin PB7
#define _rw_pin PB1
#define _enable_pin PB6

#define LCD_DDR DDRD
#define LCD_PORT PORTD

// Commands
#define EIGHTBIT 0x30
#define LCD_CLEARDISPLAY 0x01
#define LCD_DISPLAYCONTROL 0x08
#define LCD_HOME 0x02
#define LCD_ENTRYSET 0x04
#define LCD_CURSORSHIFT 0X10

// SubCommands
#define DISPLAYON 0x04
#define CURSORON 0x02
#define CURSORBLINK 0x01
#define ENTRYSET_INCREMENT 0X02
#define ENTRYSET_DISPSHIFT 0X01
#define DISPLAYSHIFT 0X08
#define SHIFTRIGHT 0X04

// Menu Characters
#define ARROW 0x3E
#define CHARDELETE 0x20
#define STAR 0x2A

//8bit pins used on Atmega for DDRAM//
uint8_t _data_pins[8];
unsigned char getdata,datalocation;
volatile int displayFlag;
int coursor,caseFlag;
/* unsigned char data; */


#define setBit(sfr, bit)     (_SFR_BYTE(sfr) |= (1 << bit))
#define clearBit(sfr, bit)   (_SFR_BYTE(sfr) &= ~(1 << bit))


//Define Methods//
void spi_init_master(void);
unsigned char spi_tranciever (unsigned char data);
void initPinChangeInterrupt(void);
void setup();
void menuDisplay();
void ClearDisplay();
void DAQStart();
void ReturnHome();
void SetCursor(uint8_t row, uint8_t col);
void DisplayOn();
void Stringtxt(char label[]);
void StringtxtErase(char label[]);
void Stringfloat(float floatvalue);
void Command(uint8_t CommandValue);
void Write(uint8_t WriteValue);
void write8bits(uint8_t value);
void pulseEnable();


int main(void) {
// -------- Inits --------- //
	initPinChangeInterrupt();
	spi_init_master();
	setup();
	
	DisplayOn();

	DDRC = 0x00;  //all pins C in input mode
	PORTC = 0xff; //all pins C pull-ups enabled
	DAQStart();
	menuDisplay();
	displayFlag = 0;
	coursor = 1;
	caseFlag = 1;
	PORTB = 0b01000100;
	while(1){
		
		switch (displayFlag)
		{
			case 1:
				if(caseFlag == 4){
					displayFlag = 0;
				}
				else if (caseFlag == 3){
					displayFlag = 0;
				}
				else{
					if (coursor == 1){
						SetCursor(2,1);
						Write(CHARDELETE);
						SetCursor(3,1);
						Write(ARROW);
						displayFlag = 0;
						coursor = 2;
					}

					else if (coursor == 2){
						SetCursor(3,1);
						Write(CHARDELETE);
						SetCursor(4,1);
						Write(ARROW);
						displayFlag = 0;
						coursor = 3;
					}
				}
				break;
			case 2:
				if (coursor == 3){
					SetCursor(4,1);
					Write(CHARDELETE);
					SetCursor(3,1);
					Write(ARROW);
					displayFlag = 0;
					coursor = 2;
				}
				else if (coursor == 2){
					SetCursor(3,1);
					Write(CHARDELETE);
					SetCursor(2,1);
					Write(ARROW);
					displayFlag = 0;
					coursor = 1;
				}	
				break;
			case 3:
				if (coursor ==1){
					menuDisplay();
					displayFlag = 0;
					caseFlag = 1;
				}
				break;
			case 4:
				if (coursor ==1){
					SetCursor(2,2);
					Write(STAR);
					_delay_ms(500);
					Write(CHARDELETE);
					ClearDisplay();
					SetCursor(2,1);
					Stringtxt("FWD SEAL TEMP:");
					Stringfloat(99.76);
					Stringtxt("F");
					SetCursor(3,1);
					Stringtxt("AFT SEAL TEMP:");
					Stringfloat(90.21);
					Stringtxt("F");
					displayFlag = 0;
					caseFlag = 4;
					getdata = 0x00;                    //Reset ACK in "data"
					datalocation = 0x00;
					//if spi_tranciver:1=TC 2 =Freq., ...
					datalocation = spi_tranciever('1');     //Send '0', receive G
					if(datalocation == 'a'){
						for(int i=0; i<8; i++){
							PORTB = 0b01000000;
							_delay_ms(10);
							getdata = spi_tranciever('3');     //Send '0', receive G
							PORTB = 0b01000100;
							SetCursor(4,i+1);
							Write(getdata);
						}
						SetCursor(4,9);
						Stringtxt("degF");
						
						for(int i=0; i<8; i++){
							PORTB = 0b01000000;
							_delay_ms(10);
							getdata = spi_tranciever('3');     //Send '0', receive G
							PORTB = 0b01000100;
							SetCursor(4,i+9);
							Write(getdata);
						}

					}

				}
				break;
				
			default:
				break;
			
		}

	}
	
	return 0;               
}


//Initialize SPI Master Device
void spi_init_master(){
  DDRB = (1<<5)|(1<<3)|(1<<2);  //Set SCK, MOSI and SS as Output
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);  //Enable SPI, Set as Master and Prescaler
}

//Function to send and recieve data
unsigned char spi_tranciever (unsigned char data){
  SPDR = data;
  while(!(SPSR & (1<<SPIF) ));
  return(SPDR);
}


void initPinChangeInterrupt(){
	PCICR |= (1<<PCIE1);
	PCMSK1 |= (1<<PCINT13)|(1<<PCINT12)|(1<<PCINT11)|(1<<PCINT10);
	sei();
}

ISR(PCINT1_vect){
	if (bit_is_clear(PINC,PC5)){
		displayFlag = 1;
	}
	else if (bit_is_clear(PINC,PC4)){
		displayFlag = 2;
	}
	else if (bit_is_clear(PINC,PC3)){
		displayFlag = 3;
	}
	else if (bit_is_clear(PINC,PC2)){
		displayFlag = 4;
	}
}


void setup(){
	//Set up 8bit Pin Location Registers
	_data_pins[1] = PD1;
	_data_pins[2] = PD2;
	_data_pins[3] = PD3; 
	_data_pins[4] = PD4;
	_data_pins[5] = PD5;
	_data_pins[6] = PD6;
	_data_pins[7] = PD7;
	
	//We set rs, rw and enable to output pins*/
	DDRB = 0b11101110;  //set SS as OUTPUT for SPI
	//set all _data_pins to output
	DDRD = 0xFF;
	
	//********************************************************************************
	// Initializing by Instruction pg. 45
	//********************************************************************************

	_delay_ms(100); //Delay for Main LCD Power
	
	// Now we pull both RS (PB7) and R/W (PB1) low to begin commands
	PORTB = 0b01000000;
	
	//**ENSURE we set 8 bits
	// Send function set command sequence (8bits ONLY) #1
	PORTD = 0b00110000;
	PORTB = 0b00000000;
	//required delay
	_delay_ms(4.5);
	
	// Send function set command sequence (8bits ONLY) #2
	PORTB = 0b01000000;
	PORTD = 0b00110000;
	PORTB = 0b00000000;
	//required delay
	_delay_ms(15);
	
	// Send function set command sequence (8bits ONLY) #3
	PORTB = 0b01000000;
	PORTD = 0b00110000;
	PORTB = 0b00000000;
	//required delay
	_delay_ms(15);
	//**********************
	PORTB = 0b01000000;
	PORTD = 0b00111000; // Function Set Command: Specifiy
	PORTB = 0b00000000;
	_delay_ms(15);
	PORTB = 0b01000000;
	PORTD = 0b00001000; // Display off
	PORTB = 0b00000000;
	_delay_ms(15);
	PORTB = 0b01000000;
	PORTD = 0b00000001; // Display clear
	PORTB = 0b00000000;
	_delay_ms(15);
	PORTB = 0b01000000;
	PORTD = 0b00000110; // Entry mode Set
	PORTB = 0b00000000;
	_delay_ms(15);
}


void menuDisplay(){
	ClearDisplay();
	SetCursor(2,1);
	Write(ARROW);

	_delay_ms(100);
	SetCursor(1,3);
	Stringtxt("----Main Menu----");
	SetCursor(2,3);
	Stringtxt("SEAL TEMP");
	SetCursor(3,3);
	Stringtxt("BEARING TEMP");
	SetCursor(4,3);
	Stringtxt("FLUID TEMP");
}


void ClearDisplay(){
	Command(LCD_CLEARDISPLAY);
}

void DAQStart(){
	ClearDisplay();
	SetCursor(1,4);
	Stringtxt("DURAMAX MARINE");
	SetCursor(2,1);
	Stringtxt("SHAFT MONITOR SYSTEM");
	
	SetCursor(4,1);
	Stringtxt("Init#");
	
	SetCursor(4,6);
	Stringtxt("|");
	SetCursor(4,20);
	Stringtxt("|");
	
	SetCursor(4,7);
	for(int i=0; i<13; i++){
		Write(ARROW);
		_delay_ms(200);
	}
	
}

void ReturnHome(){
	Command(LCD_HOME);
}


void SetCursor(uint8_t row, uint8_t col){
	ReturnHome();
	if((col <= 20) && (row ==1)){
		for(int i=0; i<(col-1); i++){
			Command(LCD_CURSORSHIFT|SHIFTRIGHT);
		}
	}
	else if((col <= 20) && (row ==2)){
		for (int j=0; j<row+1 ; j++){
			PORTB = 0b01000000; //RS : LOW and RW : LOW --> Command MODE
			PORTD = 0b11000000; // Set DDRAM=0xC0 Set cursor to line 2
			PORTB = 0b00000000; // Cycle Enable : HIGH/LOW to Read
			_delay_ms(1);
		}
		for(int i=0; i<(col-1); i++){
			Command(LCD_CURSORSHIFT|SHIFTRIGHT);
		}
	}
	else if((col <= 20) && (row ==3)){
		for (int j=0; j<row+1 ; j++){
			PORTB = 0b01000000; //RS : LOW and RW : LOW --> Command MODE
			PORTD = 0b10010100; // Set DDRAM=0x94 Set cursor to line 3
			PORTB = 0b00000000; // Cycle Enable : HIGH/LOW to Read
			_delay_ms(1);
		}
		for(int i=0; i<(col-1); i++){
			Command(LCD_CURSORSHIFT|SHIFTRIGHT);
		}
	}
	else {
		for (int j=0; j<row+1 ; j++){
			PORTB = 0b01000000; //RS : LOW and RW : LOW --> Command MODE
			PORTD = 0b11010100; // Set DDRAM=0xD4 Set cursor to line 4
			PORTB = 0b00000000; // Cycle Enable : HIGH/LOW to Read
			_delay_ms(1);
		}
		for(int i=0; i<(col-1); i++){
			Command(LCD_CURSORSHIFT|SHIFTRIGHT);
		}
	}
}


void DisplayOn(){
	Command(LCD_DISPLAYCONTROL|DISPLAYON);
}


void Stringtxt(char label[]){
	size_t len = strlen(label);
	char *labelPtr;
	labelPtr = label;
	for(int i = 0; i<len;i++){
		Write((uint8_t)*labelPtr++);	
	}
}


void StringtxtErase(char label2[]){
	size_t len2 = strlen(label2);
	for(int i=0; i<len2;i++){
		PORTB = 0b01000000;
		PORTD = 0b00000100; //Entry Set Mode [Decrement]
		PORTB = 0b00000000;
		_delay_ms(15);
		Write(CHARDELETE);
	}
	PORTB = 0b01000000;
	PORTD = 0b00000110; // Entry mode Set [Increment]
	PORTB = 0b00000000;
}


void Stringfloat(float floatvalue){
 	//convert float to char[] array
	char charray[20];
	dtostrf(floatvalue,2,5,charray);

 	//use pointer and typecast to convert char[] (from above) to uint_t
 	char *charPtr = charray;

	for(int i=0; i<5;i++){
		Write((uint8_t)*charPtr++);	
	}
}


void Command(uint8_t CommandValue){
	PORTB = 0b01000000; //RS : LOW and RW : LOW and enable : High --> Command MODE
	write8bits(CommandValue);
}

void Write(uint8_t WriteValue){
	PORTB = 0b11000000; //RS : HIGH and RW : LOW and enable : High -->  WRITE Mode
	write8bits(WriteValue);
}

void write8bits(uint8_t value) {
	for (int i = 0; i < 8; i++) {
		//Clear all PORTD pins
		clearBit(LCD_PORT,_data_pins[i]);
		int dummyVal = (value >> i) & 0x01;
		//Set each PORTD pin as either "HIGH"/"LOW"
		if (dummyVal == 1){
			setBit(LCD_PORT,_data_pins[i]);
		}
		else{
			clearBit(LCD_PORT,_data_pins[i]);
		}
	}
	pulseEnable();
}


void pulseEnable() {
	clearBit(PORTB,_enable_pin);
	_delay_ms(1);
	setBit(PORTB,_enable_pin);
	_delay_ms(1);
	setBit(PORTB,_enable_pin);
	_delay_ms(1);	
}
