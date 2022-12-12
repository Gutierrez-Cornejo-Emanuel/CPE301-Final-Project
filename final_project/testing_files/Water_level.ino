void setup () {
  Serial.begin (9600);}
 
void loop() {
  // read the input on analog pin 0:
  int value = analogRead(A0);
  Serial.println(value);
  delay(100);
  
  }
