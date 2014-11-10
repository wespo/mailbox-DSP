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


	if(!digitalRead(MS))
		{
			receive();
		};
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
 	//#define DALY 5
   unsigned int header[4] = {0}; //header 0 = '$', header[1] = checksum, header[2,3] = message length.
   SPI_Class::read(header, 1);
   int count = 0;
   while(header[0] != '$')
   {
   		count++;
   		if(count > 255)
   			break;
   }
   SPI_Class::read(header+1, 3);
//   unsigned int messageLength = 0;
   if(header[0] == '$')
   {
     inboxSize = (header[2] << 8) + header[3];
     if(inboxSize > 2048)
     {
       return; //safety check against ram murder.
     }
     unsigned int recvChecksum = header[1];
     
     inbox = (char*) realloc(inbox, inboxSize);
     //free(inbox);
     //inbox = (char*) malloc(inboxSize);

     //SPI_Class::read((unsigned int*) inbox, inboxSize);  
     for(unsigned int i = 0; i < inboxSize; i++)
     {
     	SPI_Class::read((unsigned int *) &inbox[i], 1);
     	delayMicroseconds(5); //we can potentially overwhelm the arduino with fast reads. Omitting this delay will not break the protocol but it will increase the error rate.
     }
     //compute checksum here
     unsigned int compChecksum = 0;
     for(unsigned int i = 0; i < inboxSize; i++)
     {
       compChecksum += inbox[i];
     }
     compChecksum &= 0x00FF;
     if(compChecksum != recvChecksum)
     {
		//Serial.println("bad checksum"); //you probably don't want to call the callback on a broken packer, but here it is.
     	//receiveCallback();
     	unsigned int thisByte = MB_BAD;
		SPI_Class::write(&thisByte,1);
     	return 0;
     }
     else
     {
     	unsigned int thisByte = MB_ACK;
		SPI_Class::write(&thisByte,1);
		receiveCallback();
		return 1;
     }
   }
   else
   {
     //Serial.println("bad header");
   	return 0;
   }

}
interrupt void INT1_isr(void)
{
	// 	shieldMailbox.receive();
	// 	delayMicroseconds(50);

	// while(!digitalRead(MS))
	// {
	// 	shieldMailbox.receive();
	// 	delayMicroseconds(50);
	// };
	// IRQ_clear(INTERRUPT_INT1);

 //    CSL_CPU_REGS->IFR0 = CSL_CPU_REGS->IFR0;
 //    disableInterrupt(INTERRUPT_INT1);
	// IRQ_globalDisable();
 //    // Read all the input pin values to clear the intterupts
 //    int pin2Flag = digitalRead(MS);
 //    if(!pin2Flag)
 //    {
 //      shieldMailbox.receive();      
 //    }
 //    enableInterrupt(INTERRUPT_INT1);
 //    int curTime = 0;
 //    while(!digitalRead(MS)) //if master select stays low for more than a second after we did a recieve, we probably missed a packet edge.
	// {
	// 	//delay(10);
	// 	curTime++;
	// 	if(curTime > 100)
	// 	{
	// 		shieldMailbox.receive();
	// 		delayMicroseconds(20);
	// 		if(digitalRead(MS))
	// 		{
	// 			IRQ_globalEnable();
	// 			return;
	// 		}
	// 		curTime = 0;
	// 	}
	// };
	// IRQ_globalEnable();
}