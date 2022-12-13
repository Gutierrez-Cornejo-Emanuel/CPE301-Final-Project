/*
CPE 301 Final Project Code
By Emanuel Gutierrez and Austin Hendricks

*/


#include <LiquidCrystal.h> //Import the LCD library
#include "DHT.h" //Temp / Humidity Sensor
#include <Wire.h>
#include "RTClib.h" // Real Time Clock

//Temperature/ Humidity Sensor
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//Stepper Motor Pins
#define STEPPER_PIN_1 50 //PB3
#define STEPPER_PIN_2 48 //PL1
#define STEPPER_PIN_3 46 //PL3
#define STEPPER_PIN_4 44 //PL5

//var for stepper motor sequence
int step_number = 0;

//flags set when transitioning states through ISR
volatile bool flag = false; 
volatile bool flag2 = false;

RTC_DS1307 rtc;

//Threshold to trigger error mode if water level goes below
const int WATER_LEVEL_THRESHOLD = 100;

//Turn on the fan if temperature in Fahrenheit is above this threshold
#define TEMP_THRESHOLD_F  65

//Initialize LCD with pins
LiquidCrystal lcd(12, 11, 10, 9, 8, 7); 

//set lcd contrast
const int contrast = 13;

//ddr, pin, and port variables

volatile unsigned char* port_a = (unsigned char*) 0x22; 
volatile unsigned char* ddr_a  = (unsigned char*) 0x21;

volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24;

volatile unsigned char* pin_c = (unsigned char*) 0x26; 
volatile unsigned char* ddr_c  = (unsigned char*) 0x27;

volatile unsigned char* pin_d = (unsigned char*) 0x29; 
volatile unsigned char* ddr_d  = (unsigned char*) 0x2A;

volatile unsigned char* ddr_e  = (unsigned char*) 0x2D; 
volatile unsigned char* port_e  = (unsigned char*) 0x2E;

volatile unsigned char* ddr_l  = (unsigned char*) 0x10A; 
volatile unsigned char* port_l  = (unsigned char*) 0x10B; 


//RGB LED pins
int red_light_pin= 22; //PA0
int green_light_pin = 24; //PA2
int blue_light_pin = 26; //PA4

//state variable, starts as disabled
char* state = "DISABLED";

//Start / Stop  Interrupt Button Pin
const int buttonPin1 = 18; //PD3

//vent control button pins
const int buttonPin2 = 30; //PC7
const int buttonPin3 = 32; //PC5

int buttonState2 = 0;  
int buttonState3 = 0;

void setup() {
    Serial.begin(9600);
    analogWrite(contrast, 130);
    // put your setup code here, to run once:
    lcd.begin(16, 2); //Tell the LCD that it is a 16x2 LCD
    dht.begin();
    set_PE_as_output(5); //enable
    set_PE_as_output(4); //DIRB
    set_PE_as_output(3); //DIRA

    set_pin_as_output(ddr_a, 0); //Red Light pin
    set_pin_as_output(ddr_a, 2); //Green Light pin
    set_pin_as_output(ddr_a, 4); //Blue Light pin

    set_pin_as_output(ddr_b, 3); //Stepper pin 1
    set_pin_as_output(ddr_l, 1); //Stepper pin 2
    set_pin_as_output(ddr_l, 3); //Stepper pin 3
    set_pin_as_output(ddr_l, 5); //Stepper pin 4

    set_pin_as_input(ddr_d, 3); //Start / Stop Button
    set_pin_as_input(ddr_c, 7); //Vent Direction 1 Button
    set_pin_as_input(ddr_c, 5); //Vent Direction 2 Button

    attachInterrupt(digitalPinToInterrupt(buttonPin1), handle_start_press , RISING);

    clock_setup();
}

void loop() {

    if (flag) {
      Serial.print("Transitioned to IDLE on ");
      print_time();
      flag = false;
    }
    if (flag2) {
      Serial.print("Transitioned to DISABLED on ");
      print_time();
      flag2 = false; 
    }
    if (state == "DISABLED") {
      DISABLE();
    }
    else if (state == "IDLE") {
      IDLE();
    }
    else if (state == "ERROR") {
      ERROR();
    }
    else if (state == "RUNNING") {
      RUNNING();
    }

}

void DISABLE(){
  //turn off fan
  write_pe(5, 0);
  //clear display
  lcd.clear();
  RGB_color(255, 255, 0); // Yellow
  //No temp or water level monitoring
  //monitor start button using ISR, if start is pressed go to IDLE
}

void IDLE(){
  float f = dht.readTemperature(true);
  float h = dht.readHumidity();
  RGB_color(0, 255, 0); // Green
  print_to_lcd(f,h); //update display
  //monitor water level
  if (getWaterLevel() < WATER_LEVEL_THRESHOLD) {
    lcd.clear();
    state = "ERROR";
  }
  //monitor temperature
  if (f > TEMP_THRESHOLD_F) {
    state = "RUNNING";
    Serial.print("Transitioned to RUNNING on ");
    print_time();
  }
  //Vent Control Using Buttons


  //if button 2 is pressed (PC7)
  if (*pin_c & 0x80) {
    for (int i = 0; i < 100; i++) {
      OneStep(false);
      delay(5);
    }
  }
  //if button 3 is pressed (PC5)
  if (*pin_c & 0x20) {
    for (int i = 0; i < 100; i++) {
      OneStep(true);
      delay(5);
    }
  }  

  //if stop is pressed, go to DISABLED
}
void ERROR(){
    //– Motor should be off and not start regardless of temperature
    write_pe(5, 0); 
    // A reset button should trigger a change to the IDLE stage if the water level is above the threshold (done through interrupt & start button)
    //– Error message should be displayed on LCD
    lcd.setCursor(0, 0);
    lcd.print("ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Water level LOW");
    //RED LED ON
    RGB_color(255, 0, 0);
}
void RUNNING() {
  handle_fan();
  float f = dht.readTemperature(true);
  float h = dht.readHumidity();
  RGB_color(0, 0, 255); // Green
  print_to_lcd(f,h);
  if (f < TEMP_THRESHOLD_F) {
    write_pe(5, 0);
    state = "IDLE";
    Serial.print("Transitioned to IDLE on ");
    print_time();
  }
  if (getWaterLevel() < WATER_LEVEL_THRESHOLD) {
    lcd.clear();
    Serial.print("Transitioned to ERROR on ");
    print_time();
    state =  "ERROR";
  }

  //vent control:

  //if button 2 is pressed (PC7)
  if (*pin_c & 0x80) {
    for (int i = 0; i < 100; i++) {
      OneStep(false);
      delay(5);
    }
  }
  //if button 3 is pressed (PC5)
  if (*pin_c & 0x20) {
    for (int i = 0; i < 100; i++) {
      OneStep(true);
      delay(5);
    }
  }  

}
void print_to_lcd(float temp, float hum){

  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(" F");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(hum);
  lcd.print("%");
  for (int l = 51; l > -1; l--) { 
    analogWrite(6, l * 5);
  }
  for (int l = 0; l < 51; l++) { 
    analogWrite(contrast, l * 5);
  }
  lcd.setCursor(0, 0);
}

void set_PE_as_output(unsigned char pin_num)
{
    *ddr_e |= 0x01 << pin_num;
}

void write_pe(unsigned char pin_num, unsigned char state)
{
  if(state == 0)
  {
    *port_e &= ~(0x01 << pin_num);
  }
  else
  {
    *port_e |= 0x01 << pin_num;
  }
}

void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
 {
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}

void handle_fan() {
    write_pe(5, 1); // enable on
    for (int i=0;i<5;i++) {
        write_pe(3, 1); //one way
        write_pe(4, 0);
    }
}

void OneStep(bool dir){
    if(!dir){
        switch(step_number){
            case 0:
                write_to_pin(port_b, 3, 1);
                write_to_pin(port_l, 1, 0);
                write_to_pin(port_l, 3, 0);
                write_to_pin(port_l, 5, 0);
                break;
            case 1:
                write_to_pin(port_b, 3, 0);
                write_to_pin(port_l, 1, 1);
                write_to_pin(port_l, 3, 0);
                write_to_pin(port_l, 5, 0);
                break;
            case 2:
                write_to_pin(port_b, 3, 0);
                write_to_pin(port_l, 1, 0);
                write_to_pin(port_l, 3, 1);
                write_to_pin(port_l, 5, 0);
                break;
            case 3:
                write_to_pin(port_b, 3, 0);
                write_to_pin(port_l, 1, 0);
                write_to_pin(port_l, 3, 0);
                write_to_pin(port_l, 5, 1);
              break;
        } 
    }
    else{
        switch(step_number){
            case 0:
                write_to_pin(port_b, 3, 0);
                write_to_pin(port_l, 1, 0);
                write_to_pin(port_l, 3, 0);
                write_to_pin(port_l, 5, 1);
              break;
            case 1:
                write_to_pin(port_b, 3, 0);
                write_to_pin(port_l, 1, 0);
                write_to_pin(port_l, 3, 1);
                write_to_pin(port_l, 5, 0);
                break;
            case 2:
                write_to_pin(port_b, 3, 0);
                write_to_pin(port_l, 1, 1);
                write_to_pin(port_l, 3, 0);
                write_to_pin(port_l, 5, 0);
                break;
            case 3:
                write_to_pin(port_b, 3, 1);
                write_to_pin(port_l, 1, 0);
                write_to_pin(port_l, 3, 0);
                write_to_pin(port_l, 5, 0);
                break;
		} 
    }
	step_number++;
    if(step_number > 3){
    step_number = 0;
  }
}


void handle_start_press(){
  if (state == "DISABLED") {
    //Serial.print("Transitioned to IDLE on ");
    //print_time();
    state = "IDLE";
    flag = true;
  }
  else {
    state = "DISABLED";
    flag2 = true;
    //Serial.print("Transitioned to DISABLED on ");
    //print_time();
  }
}

float getWaterLevel() {
  return analogRead(A0);
}

void clock_setup () {
 if (! rtc.begin()) {
   Serial.println("Couldn't find RTC");
   while (1);
 }
 else {
    rtc.adjust(DateTime(2022, 12, 10, 15, 7, 0));
 }
 if (! rtc.isrunning()) {
   Serial.println("RTC is NOT running!");
   rtc.adjust(DateTime(2022, 12, 10, 15, 7, 0));
 }
}

void print_time () {
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
 DateTime now = rtc.now();
 Serial.print(now.year(), DEC);
 Serial.print('/');
 Serial.print(now.month(), DEC);
 Serial.print('/');
 Serial.print(now.day(), DEC);
 Serial.print(" (");
 Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
 Serial.print(") ");
 Serial.print(now.hour(), DEC);
 Serial.print(':');
 Serial.print(now.minute(), DEC);
 Serial.print(':');
 Serial.print(now.second(), DEC);
 Serial.println();
}
void set_pin_as_output(volatile unsigned char* ddr_n, unsigned char pin_num)
{
    *ddr_n |= 0x01 << pin_num;
}
void set_pin_as_input(volatile unsigned char* ddr_n, unsigned char pin_num)
{
    *ddr_n &= (~(0x01 << (pin_num - 1)));
}
void write_to_pin(volatile unsigned char* port_address, unsigned char pin_num, unsigned char state) {
  if(state == 0)
  {
    *port_address &= ~(0x01 << pin_num);
  }
  else
  {
    *port_address |= 0x01 << pin_num;
  }
}