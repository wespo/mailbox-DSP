/* 
* July 13, 2015
* Bill Esposito
* Reid Kovacs
*
* This sketch recieves a number from and arduino, doubles it, and returns it. Make sure the arduino has MailboxCallResponse_Arduino.ino running.
*/

#include <mailbox.h>
#include <SPI.h>
#include "messageStateMachine.h" //extension for the mailbox library that enables the reading of a message one byte at a time instead of all at once.

int val = 0;
int outMessage[1];
int inMessage[1];

void setup()
{
  shieldMailbox.begin(SPI_MASTER);
  shieldMailbox.attachHandler(doubleNum);
}

void doubleNum()
{
  int recvNum = (shieldMailbox.inbox[1] << 8) + shieldMailbox.inbox[0]; //reconstruct int from two byts sent by Arduino.
  recvNum *=2; //multiply by two
  shieldMailbox.transmit(&recvNum, 1); //DSP Sends back two bytes anyway.
}

void loop()//resending is issue or recieving on arduino side
{
  shieldMailbox.receive();
}
