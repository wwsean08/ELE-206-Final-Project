//these imports allow us to use the ethernet shield
//the imports below are used for the ethernet shield
#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
//to deal with the sd card for file i/o
#include <SD.h>  
//the imports below are used for the temperature and time sensor
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>

#define DS1307_I2C_ADDRESS 0x68  // This is the I2C address
#define PREFIX "" //tells the web server to start at the root
/*
*Below are the global variables and constants
 */

const byte ONE_WIRE_BUS = 8;  //the pin for reading the temperature
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);  //the teperature sensor
static uint8_t mac[] = { 
  0x90, 0xA2, 0xDA, 0x0D, 0x08, 0x0C }; //the mac address of the arduino
static uint8_t ip[] = { 
  192, 168, 1, 20 }; //the ip address of the arduino
WebServer webserver(PREFIX, 80);
const byte ULTRASOUNDSIGNAL = 7; // Ultrasound signal pin
int val = 0;
int temp = 0;
int timecount = 0; // Echo counter
int timeOpen = 0;
const int THRESHOLD = 250;  //the threshold to determine if the door is open or closed
boolean isOpen = false;
boolean wasOpen = false;
const byte HOMEPIN = 9;  //the pin which controls the LED in the house
const byte DOORPIN = 5;  //the pin which will swap the transistor to open/close the doorunsigned long timeOpen = 0;
const byte LASERPIN = 3;  //the pin we will read the laser on
unsigned long autoclose = 7200000; //2 hours in miliseconds  
boolean AutoCloseEnabled = true;

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
    digitalWrite(HOMEPIN, LOW); //turn light off the LED if it's open
  }
  else{  //we are open
    if(wasOpen == false){
      timeOpen = millis();
      wasOpen = true;
    }
    else{
      if(AutoCloseEnabled){
        if(timeOpen+autoclose < millis()){
          changeDoorState();
        }
      }
    }
    isOpen = true;
    digitalWrite(HOMEPIN, HIGH);  //light up the LED if it's open
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
  temp = (int)sensors.getTempFByIndex(0);
  return temp;
}

/**
* checks if the safety laser is blocked and returns true if it is, it currently is stubbed out.
* this needs to be tested as i'm not sure if it's normally open or normally closed
*/
boolean laserIsBlocked(){
  if(digitalRead(LASERPIN) == HIGH){
    return false;
  }else{
    return true;
  }
}

/**
* Updates the log file
* Modes:
*    0. Door closing
*    1. Door opening
*    2. Door closing automatically
*/
void updateLog(byte mode){
  File logger = SD.open("log.txt", FILE_WRITE);
  switch(mode){
    case 0:
      logger.println("<insert time here> Closing garage door from website");
      break;
    case 1:
      logger.println("<insert time here> Opening garage door from website");
      break;
    case 2:
      logger.println("<insert time here> Closing garage door automatically");
      break;
  }
  logger.close();
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
  P(buttonCloseDisabled) = "<input type=\"submit\" disabled=\"disabled\" value=\"Close Door\" name=\"button\" /> \n";
  P(buttonOpen) = "<input type=\"submit\" value=\"Open Door\" name=\"button\" /> \n";
  P(buttonOpenDisabled) = "<input type=\"submit\" disabled=\"disabled\" value=\"Open Door\" name=\"button\" /> \n";
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
  server.printP(form);
  if(isOpen){
    if(!laserIsBlocked())
      server.printP(buttonClose);
    else
      server.printP(buttonCloseDisabled);
  }
  else{
    if(!laserIsBlocked())
      server.printP(buttonOpen);
    else
      server.printP(buttonOpenDisabled);
  }
  server.print("</form>\n");
  server.printP(endBody);
}

/**
 *  This will take care of opening or closing the garage door if the button
 *  on the main page is clicked.  In a good scinario this would require a password
 *  however in ours it won't for simplicity reasons and because it will be local only
 */
void controlCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  if(!laserIsBlocked()){
    if(isOpen)
      updateLog(0);
    else
      updateLog(1);
    changeDoorState();
    File control = SD.open("control.htm");
    if(control){
      server.httpSuccess();
      while(control.available()){
        server.print((char)control.read());
      }
      control.close();
    }else{
      server.httpFail();
    }
  }
}

/**
 * will load up the configuration website from the sd card and display it to the user
 */
void configCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  File config = SD.open("config.htm");
  if(config){
    server.httpSuccess();
    while(config.available()){
      server.write((char)config.read());
    }
    config.close();
  }else{
    server.httpFail();
  }
}

/**
 * this will take care of any changes to the configuration file, updating the memory, and writing them to a file
 */
void changeConfigCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  //1. check if the password submitted matches the current one, if it doesn't don't change the settings and return
  SD.remove("config.txt");
  File configFile = SD.open("config.txt", FILE_WRITE);
  //2. go thru the arguments we should be receiving and write them to the file in a defined way, if there is a "missing"
  //   argument, keep the current one from memory.
  //3. update the memory for the variables like temp to close at, and when to close, and the password
}

/**
* Takse care of someone wanting to look at the log
*/ 
void logCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  File logger = SD.open("log.txt");
  if(logger){
    server.httpSuccess();
    while(logger.available()){
      server.write((char)logger.read());
    }
    logger.close();
  }else{
    server.httpFail();
  }
}

void failCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  File failPage = SD.open("404.htm");
  if(failPage){
    while(failPage.available()){
      server.write((char)failPage.read());
    }
    failPage.close();
  }
}

/**
 *  The initial setup of the program, it is only called once to initialize everything
 */
void setup() {
  pinMode(HOMEPIN, OUTPUT);  //the pin with the LED for notification
  pinMode(DOORPIN, OUTPUT);  
  pinMode(8, OUTPUT);        //the pin to read the temperature sensor
  pinMode(LASERPIN, INPUT);  //the pin to read the laser safety sensor
  attachInterrupt(0, changeDoorState, RISING);  //located on digital pin 2
  digitalWrite(DOORPIN, LOW);
  sensors.begin();
  Ethernet.begin(mac, ip);  
  SD.begin(4);
  Wire.begin();
  webserver.setDefaultCommand(&statusCommand);
  webserver.addCommand("index.htm", &statusCommand);  //the home page which gives you the status, same as above
  webserver.addCommand("control.htm", &controlCommand);  //this raises or lowers the garage door
  webserver.addCommand("config.htm", &configCommand);  //this is the one that will display the form to change the config
  webserver.addCommand("changeConfig.htm", &changeConfigCommand);  //this is used to actually change the settings
  webserver.addCommand("log.txt", &logCommand);  //this command will read the log file to the person requesting it
  webserver.setFailureCommand(&failCommand);  //if it's none of the above http requests
  webserver.begin();  //start up the webserver, time for some fun
}

/**
 *  The main loop of the program which is constantly executing
 */
void loop() {
  char buff[64];
  int len = 64;
  readDoorSensor();  //read the sensor and possibly does some of the automatic stuff
  webserver.processConnection(buff, &len);  //check to see if someone wants to connect to the site
  delay(100);  //delay and go again
}

