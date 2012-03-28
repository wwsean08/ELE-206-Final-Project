//these imports allow us to use the ethernet shield
//the imports below are used for the ethernet shield
#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"
#include <SD.h>  //to deal with the sd card
//the imports below are used for the temperature and time sensor
#include "Wire.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 8
#define DS1307_I2C_ADDRESS 0x68  // This is the I2C address
#define PREFIX "" //tells the web server to start at the root

/*
*Below are the global variables and constants
 */
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);  //the teperature sensor
static uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x08, 0x0C }; //the mac address of the arduino
static uint8_t ip[] = { 192, 168, 1, 20 }; //the ip address of the arduino
WebServer webserver(PREFIX, 80);
const byte ULTRASOUNDSIGNAL = 7; // Ultrasound signal pin
int val = 0;
int timecount = 0; // Echo counter
const int THRESHOLD = 250;  //the threshold to determine if the door is open or closed
boolean isOpen = false;
boolean wasOpen = false;
const byte OPENPIN = 9;  //the pin which controls the LED in the house
const byte DOORPIN = 5;  //the pin which will swap the transistor to open/close the door
unsigned long timeOpen = 0;
unsigned const long AUTOCLOSE = 7200000; //2 hours in miliseconds  

/**
 *  This method reads wether or not the garage door is open and
 *  saves that information to the isOpen/wasOpen variables as
 *  updates the LED in the house, and will autoclose the door
 *  if it's been open too long
 *
 *  Much of the code in this method is used from the sample code
 *  for the ping ultrasonic sensor
 */
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
    }
    else{
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
 *the garrage door using an interrupt or if the user hits the button
 *on the website to close the garage door
 */
void changeDoorState(){
  digitalWrite(DOORPIN, HIGH);
  delay(250);
  digitalWrite(DOORPIN, LOW);
}

/**
 * get's the current temperature of the garage
 */
int getTemp(){
  sensors.requestTemperatures();
  return (int)sensors.getTempFByIndex(0);
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
  P(buttonClose) = "<input type=\"submit\" value=\"Close Door\" name=\"button\" /> \n";
  P(buttonOpen) = "<input type=\"submit\" value=\"Open Door\" name=\"button\" /> \n";
  P(form) = "<form action=\"control.html\" method=\"post\"> \n";
  P(redirrect) = "<meta http-equiv=\"refresh\" content=\"300; /index.html\">";
  server.printP(header);
  server.printP(title);
  server.printP(redirrect);
  server.printP(endHeader);
  server.printP(DoorStatus);
  if(isOpen){
    server.print("Open </h1><br /> \n");
  }
  else{
    server.print("Closed </h1><br /> \n");
  }
  server.print("<h1>Current Temprature: ");
  server.print(getTemp());
  server.print("˚</h1><br /> \n");
  if(isOpen){
    server.printP(form);
    server.printP(buttonClose);
    server.print("</form>\n");
  }
  else{
    server.printP(form);
    server.printP(buttonOpen);
    server.print("</form>\n");
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
  File control = SD.open("control.html", FILE_READ);
  while(control.available()){
    server.print(control.read());
  }
  control.close();
}

/**
* will load up the configuration website from the sd card and display it to the user
*/
void configCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  server.httpSuccess();
  File config = SD.open("config.html", FILE_READ);
  while(config.available()){
    server.print(config.read());
  }
  config.close();
}

/**
* this will take care of any changes to the configuration file, updating the memory, and writing them to a file
*/
void changeConfigCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  
}

/**
 *  The initial setup of the program, it is only called once to initialize everything
 */
void setup() {
  pinMode(OPENPIN, OUTPUT);  //the pin with the LED for notification
  attachInterrupt(0, changeDoorState, RISING);  //located on digital pin 2
  pinMode(DOORPIN, OUTPUT);
  pinMode(8, OUTPUT);
  digitalWrite(DOORPIN, LOW);
  sensors.begin();
  Ethernet.begin(mac, ip);
  SD.begin(4);
  webserver.setDefaultCommand(&statusCommand);
  webserver.addCommand("index.html", &statusCommand);
  webserver.addCommand("control.html", &controlCommand);
  webserver.addCommand("config.html", &configCommand);
  webserver.addCommand("changeConfig.html", &changeConfigCommand);
  webserver.begin();
}

/**
 *  The main loop of the program which is constantly executing
 */
void loop() {
  char buff[64];
  int len = 64;
  readDoorSensor();
  webserver.processConnection(buff, &len);
  delay(100);
}
