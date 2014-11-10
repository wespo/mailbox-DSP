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

#ifndef _MAILBOX_H_INCLUDED	//prevent mailbox library from being invoked twice and breaking the namespace
#define _MAILBOX_H_INCLUDED
#include <SPI.h>

//for timer interrupt mode
#include <Timers.h>
#include "csl_gpt.h"
#include "csl_intc.h"
#include <csl_general.h>

#ifdef __cplusplus
extern "C" void INT1_isr(void);
//required hardware SPI pins. These cannot be changed if using hardware SPI, so they have not been made variables 
#define SS 10			//SLAVE SELECT
#define MS 2			//MASTER SELECT
#define MOSI 11			//MASTER OUT, SLAVE IN
#define MISO 12			//MASTER IN, SLAVE OUT
#define SCK 13			//SPI CLOCK
#define SPI_MASTER true //MASTER MODE
#define SPI_SLAVE false //SLAVE MODE
#define MB_ACK 0xBE
#define MB_BAD 0x44
#define RETRIES 8
class mailbox : public SPI_Class {
public:
	int begin(bool mode, void (*callbackFunction)()); //start as master or slave
	int begin(bool mode); //start as master or slave
	int transmit(int *vector, unsigned int vectorSize); //transmit vector
	int receive(); //wait for new inbox buffer
	int end(); //stop
	int attachHandler(void (*callbackFunction)()); //start as master or slave
	int detachHandler(); //start as master or slave
	bool newMessage; //flag to alert that a new message has been received.
	friend void INT1_isr();	//allow MS interrupt to access mailbox private variables.
	unsigned int inboxSize;
	char *inbox;  //inbox buffer
private:
	bool spiMode;	//determine if the library has been initialized in master or slave mode.
	unsigned int messageIndex;
	void (*receiveCallback)();
	static void nullMailboxCallback();
	char checksum;
	bool success;
};

extern "C" {

	extern mailbox shieldMailbox;
}


#else
	typedef struct mailbox mailbox;

#endif
#endif