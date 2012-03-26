//these imports allow us to use the ethernet shield
#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"

/*
*Below are the global variables and constants
*/
static uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x08, 0x0C };
static uint8_t ip[] = { 192, 168, 1, 20 };
#define PREFIX ""
WebServer webserver(PREFIX, 80);
const byte ULTRASOUNDSIGNAL = 7; // Ultrasound signal pin
int val = 0;
int timecount = 0; // Echo counter
const int THRESHOLD = 250;
boolean isOpen = false;
boolean wasOpen = false;
const byte OPENPIN = 9;
const byte DOORPIN = 5;
unsigned long timeOpen = 0;
unsigned const long AUTOCLOSE = 0;

void readDoorSensor(){
  timecount = 0;
  val = 0;
  pinMode(ULTRASOUNDSIGNAL, OUTPUT); // Switch signalpin to output

  /* Send low-high-low pulse to activate the trigger pulse of the sensor
   * -------------------------------------------------------------------
   */

  digitalWrite(ULTRASOUNDSIGNAL, LOW); // Send low pulse
  delayMicroseconds(2); // Wait for 2 microseconds
  digitalWrite(ULTRASOUNDSIGNAL, HIGH); // Send high pulse
  delayMicroseconds(5); // Wait for 5 microseconds
  digitalWrite(ULTRASOUNDSIGNAL, LOW); // Holdoff

  /* Listening for echo pulse
   * -------------------------------------------------------------------
   */

  pinMode(ULTRASOUNDSIGNAL, INPUT); // Switch signalpin to input
  val = digitalRead(ULTRASOUNDSIGNAL); // Append signal value to val
  while(val == LOW) { // Loop until pin reads a high value
    val = digitalRead(ULTRASOUNDSIGNAL);
  }

  while(val == HIGH) { // Loop until pin reads a high value
    val = digitalRead(ULTRASOUNDSIGNAL);
    timecount = timecount +1;            // Count echo pulse time
  }

  /* Writing out values to the serial port
   * -------------------------------------------------------------------
   */
  if(timecount > THRESHOLD){  //we are closed
    if(wasOpen){
      wasOpen = false;
    }
    isOpen = false;
    digitalWrite(OPENPIN, LOW); //turn light off the LED if it's open
  }
  else{  //we are open
    if(wasOpen == false){
      timeOpen = millis();
      wasOpen = true;
    }else{
      if(timeOpen+AUTOCLOSE < millis()){
        changeDoorState();
      }
    }
    isOpen = true;
    digitalWrite(OPENPIN, HIGH);  //light up the LED if it's open
  }

  /* Delay of program
   * -------------------------------------------------------------------
   */
}

/*This should get called if the button is pressed in order to close 
 *the garrage door using an interrupt.
*/
void changeDoorState(){
  digitalWrite(DOORPIN, HIGH);
  delay(250);
  digitalWrite(DOORPIN, LOW);
}
/**
*  this will be called whenever someone goes to our main site, 
*  and will display the current info about the garrage.  It will 
*  also allow you to close or open the garage door remotely
*/
void statusCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  server.httpSuccess(); //send the 200 saying we're good
  P(header) = "<html> \n<head>";
    P(title) = "<title>Garage Info</title>";
  P(endHeader) = "</head> \n<body>";
  P(endBody) = "</body> \n</html>";
  P(DoorStatus) = "<h1>Door Status: ";
  P(form) = "<form action=\"control.html\" method=\"post\"> \n";
  server.printP(header);
  server.printP(title);
  server.printP(endHeader);
  server.printP(DoorStatus);
  if(isOpen){
    server.print("Open </h1><br /> \n");
    //form to open/close the door
    server.printP(form);
    server.print("<input type=\"submit\" value=\"Close Door\" name=\"button\" /> \n");
    server.print("</form>");
  }else{
    server.print("Closed </h1><br /> \n");
    server.printP(form);
    server.print("<input type=\"submit\" value=\"Open Door\" name=\"button\" /> \n");
    server.print("</form>");
  }
  server.printP(endBody);
}

/**
*  This will take care of opening or closing the garage door if the button
*  on the main page is clicked.  In a good scinario this would require a password
*  however in ours it won't for simplicity reasons and because it will be local only
*/
void controlCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  server.httpSuccess();
  changeDoorState();
}

void setup() {
  pinMode(OPENPIN, OUTPUT);  //the pin with the LED for notification
  attachInterrupt(0, changeDoorState, RISING);  //located on digital pin 2
  pinMode(DOORPIN, OUTPUT);
  pinMode(8, OUTPUT);
  digitalWrite(DOORPIN, LOW);
  Ethernet.begin(mac, ip);
  webserver.setDefaultCommand(&statusCommand);
  webserver.addCommand("index.html", &statusCommand);
  webserver.addCommand("control.html", &controlCommand);
  webserver.begin();
}

void loop() {
  char buff[64];
  int len = 64;
  readDoorSensor();
  webserver.processConnection(buff, &len);
  delay(100);
}
