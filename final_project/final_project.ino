#include <LiquidCrystal.h>
#include "helper.h"

volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 
volatile unsigned char* pin_b  = (unsigned char*) 0x23; 

//assuming red led at pin 6 (port b)
const unsigned char RED_LED = 6;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(5, 6, 7, 8, 9, 10);

void setup() 
{
  //set PB7 to OUTPUT
  set_pin_as_output(ddr_b,RED_LED);
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");
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
  
/* LCD example code */
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis() / 1000);  
}
