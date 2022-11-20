#include "helper.h"
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 
volatile unsigned char* pin_b  = (unsigned char*) 0x23; 

//assuming red led at pin 6 (port b)
const unsigned char RED_LED = 6;

void setup() 
{
  //set PB7 to OUTPUT
  set_pin_as_output(ddr_b,RED_LED);
}

void loop() 
{
  // drive PB6 HIGH
  write_to_pin(port_b, RED_LED,1);
  // wait 300ms
  delay(300);
  // drive PB6 LOW
  write_to_pin(port_b, RED_LED, 0);
  // wait 300ms
  delay(300);
}
