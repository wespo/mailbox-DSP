/* Arduino Bidirectional SPI Mailbox Library	*/
/* W. Esposito 6/2014	R01						*/
/************************************************/
/* This library is for bidirectional 			*/
/* communication between two arduinos, one 		*/
/* configured as an SPI master and one as an 	*/
/* SPI slave. In addtion to the normal three 	*/
/* wire spi interface, this method will require */
/* the fourth signal line to request a transmit	*/
/* event from the 'master' to the 'slave' 		*/
/* device be pin 10.							*/
/************************************************/
/* Pins Required: 10 (SS), 11(MOSI), 12(MISO),	*/
/* 13 (SCK), 2(Master Select / MS).				*/
/************************************************/

#include "core.h"
#include "mailbox.h"
//#include <Arduino.h>

// Definition of interrupt names
//#include <avr/io.h>
// ISR interrupt service routine
//#include <avr/interrupt.h>
#include "messageStateMachine.h" //extension for the mailbox library that enables the reading of a message one byte at a time instead of all at once.
messageStateData arduinoMessageState; //struct with state data for message state machine.

mailbox shieldMailbox;

int mailbox::begin(bool mode) {
	begin(mode, nullMailboxCallback); //initialize with a do nothing callback!
}
int mailbox::begin(bool mode, void (*callbackFunction)()) {
	inbox = (char*) malloc(10);
	newMessage = false; //clear new message flags.
	messageIndex = 0;
	receiveCallback = callbackFunction; //set the callback function.
	spiMode = SPI_MASTER; //DSP Shield can only be an SPI Master due to hardware limitation.
	
    SPI_Class::begin(); //initialize SPI
	SPI_Class::setClockDivider(SPI_CLOCK_DIV128);

	pinMode(SPI_RX_SEL, OUTPUT);
    pinMode(ARD_SPI_EN, OUTPUT);
    digitalWrite(SPI_RX_SEL, LOW);
    digitalWrite(ARD_SPI_EN, HIGH);
	
	//attach master select interrupt
	pinMode(MS, INPUT);
    digitalWrite(MS, HIGH);
    // Init interrupt module
    //initInterrupt(0x0000);

    // Plugin the ISR to vector table and enbale interrupts 	
    //attachInterrupt(INTERRUPT_INT1, (INTERRUPT_IsrPtr)INT1_isr, 0);    
    //enableInterrupt(INTERRUPT_INT1);

    //initialize mailbox per byte state machine.
    arduinoMessageState = initMessageStateData(arduinoMessageState);

	// if(!digitalRead(MS))
	// 	{
	// 		receive();
	// 	};
	return 1; //""success""
}
int mailbox::end() {
	//Kill SPI
	SPI_Class::end();
	
	//Detach GPIO Interrupt.
	
	
	return 1;
}

int mailbox::attachHandler(void (*callbackFunction)()) {
	receiveCallback = callbackFunction; //set the interrupt handler to the specified callback function
}
int mailbox::detachHandler() {
	receiveCallback = nullMailboxCallback; //set the interrupt handler to the null callback function
}
void mailbox::nullMailboxCallback() //this is a do nothing function
{
	//Serial.println("null callcback");
}
int mailbox::transmit(int *vector, unsigned int vectorSize) {
	//vector = pointer to data to transmit
	//vectorSize = vector length in 16 bit WORDS
	//message format:
	//‘$’<checksum byte><16 bit length><message words> = 2 words + message;
	//Serial.println("Transmitting, please wait.");
	
	//define the length of the message to send (this can be 1 word longer than the messageLength)
	unsigned int messageLength = vectorSize;
	// if(messageLength % 2) //message is odd length.
	// {
	// 	messageLength++; //send last word
	// }
	unsigned int* data = (unsigned int*) malloc(messageLength + 2); //create the array to transmit
	memcpy(data + 2, vector, vectorSize);
	// if(vectorSize % 2) //if we are sending a dummy last word, make sure its zeroed.
	// {
	// 	data[vectorSize+2] = 0;
	// }
	char checksum = 0;
	for(unsigned int i = 0; i < vectorSize; i++)
	{
		//Serial.println((int) data[i+2], HEX);
		checksum += data[i+2];
	}
	//construct array
	data[0] = '$';	//start sentinel
	data[0] <<= 8;
	checksum &= 0x00FF;
	data[0] |= checksum;
	data[1] = vectorSize*2; //vector length is in bytes.
	
	//transmit array.
	for(unsigned int i = 0; i < messageLength+2; i++)
	{
		unsigned int thisByte = data[i]>>8;
		SPI_Class::write(&thisByte,1);
		//delayMicroseconds(10);
		thisByte = data[i];	
		SPI_Class::write(&thisByte,1);	
		//delayMicroseconds(10);
	}
	

	free(data);
	data = NULL;
	return 1;
}
int mailbox::receive() {
  if(messageStateMachine(arduinoMessageState))
  {
    receiveCallback();
  }

}
interrupt void INT1_isr(void)
{
	
}