 /* Ultrasound Sensor
 *------------------
 *
 * Reads values (00014-01199) from an ultrasound sensor (3m sensor)
 * and writes the values to the serialport.
 *
 * http://www.xlab.se | http://www.0j0.org
 * copyleft 2005 Mackie for XLAB | DojoDave for DojoCorp
 *
 */

const byte ultraSoundSignal = 7; // Ultrasound signal pin
int val = 0;
int timecount = 0; // Echo counter
int threshold = 250;
boolean isOpen = false;
boolean wasOpen = false;
const byte openPin = 9;
const byte doorPin = 5;
unsigned long timeOpen = 0;
unsigned const long autoClose = 0;

void setup() {
  pinMode(openPin, OUTPUT);  //the pin with the LED for notification
  attachInterrupt(0, changeDoorState, RISING);  //located on digital pin 2
  pinMode(doorPin, OUTPUT);
  digitalWrite(doorPin, LOW);
}

void loop() {
  readDoorSensor();
  delay(100);
}

void readDoorSensor(){
  timecount = 0;
  val = 0;
  pinMode(ultraSoundSignal, OUTPUT); // Switch signalpin to output

  /* Send low-high-low pulse to activate the trigger pulse of the sensor
   * -------------------------------------------------------------------
   */

  digitalWrite(ultraSoundSignal, LOW); // Send low pulse
  delayMicroseconds(2); // Wait for 2 microseconds
  digitalWrite(ultraSoundSignal, HIGH); // Send high pulse
  delayMicroseconds(5); // Wait for 5 microseconds
  digitalWrite(ultraSoundSignal, LOW); // Holdoff

  /* Listening for echo pulse
   * -------------------------------------------------------------------
   */

  pinMode(ultraSoundSignal, INPUT); // Switch signalpin to input
  val = digitalRead(ultraSoundSignal); // Append signal value to val
  while(val == LOW) { // Loop until pin reads a high value
    val = digitalRead(ultraSoundSignal);
  }

  while(val == HIGH) { // Loop until pin reads a high value
    val = digitalRead(ultraSoundSignal);
    timecount = timecount +1;            // Count echo pulse time
  }

  /* Writing out values to the serial port
   * -------------------------------------------------------------------
   */
  if(timecount > threshold){  //we are closed
    if(wasOpen){
      wasOpen = false;
    }
    isOpen = false;
    digitalWrite(openPin, LOW); //turn light off the LED if it's open
  }
  else{  //we are open
    if(wasOpen == false){
      timeOpen = millis();
      wasOpen = true;
    }else{
      if(timeOpen+autoClose < millis()){
        changeDoorState();
      }
    }
    isOpen = true;
    digitalWrite(openPin, HIGH);  //light up the LED if it's open
  }

  /* Delay of program
   * -------------------------------------------------------------------
   */
  delay(10000); 
}

/*This should get called if the button is pressed in order to close 
 *the garrage door using an interrupt.
*/
void changeDoorState(){
  digitalWrite(doorPin, HIGH);
  delay(250);
  digitalWrite(doorPin, LOW);
}
