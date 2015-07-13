#include <mailbox.h>
#include <SPI.h>
#include "OLED.h"
#include "messageStateMachine.h" //extension for the mailbox library that enables the reading of a message one byte at a time instead of all at once.
messageStateData arduinoMessageState; //struct with state data for message state machine.

int val = 0;
int outMessage[1];
int inMessage[1];

void setup()
{
  disp.oledInit();        /* Initialize the OLED module */
  disp.setOrientation(1); /* Set orientation of the LCD to horizontal */

  disp.clear();           /* Clears the entire display screen */
    
  shieldMailbox.begin(SPI_MASTER);
  arduinoMessageState = initMessageStateData(arduinoMessageState);
}

void doubleNum()
{
  int recvNum = (shieldMailbox.inbox[1] << 8) + shieldMailbox.inbox[0]; //reconstruct int from two byts sent by Arduino.
  recvNum *=2; //multiply by two
  shieldMailbox.transmit(&recvNum, 1); //DSP Sends back two bytes anyway.
}

void loop()//resending is issue or recieving on arduino side
{
  if(messageStateMachine(arduinoMessageState))
  {
    doubleNum();
  }
}

int getLength(int num){
  if(num > 9999)
  return 5;
else if(num > 999)
  return 4;
else if(num > 99)
  return 3;
else if(num > 9)
  return 2;
else
  return 1;
}
