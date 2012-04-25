#define PREFIX "" //tells the web server to start at the root

//these imports allow us to use the ethernet shield
//the imports below are used for the ethernet shield
#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
//to deal with the sd card for file i/o
#include <SD.h>

/*
*Below are the global variables and constants
 */

static uint8_t mac[] = { 
  0x90, 0xA2, 0xDA, 0x0D, 0x08, 0x0C }; //the mac address of the arduino
static uint8_t ip[] = { 
  192, 168, 16, 18 }; //the ip address of the arduino
WebServer webserver(PREFIX, 80);
const byte ULTRASOUNDSIGNAL = 7; // Ultrasound signal pin
int val = 0;
int temp = 0;
int timecount = 0; // Echo counter
const int THRESHOLD = 250;  //the threshold to determine if the door is open or closed
boolean isOpen = false;
boolean wasOpen = false;
const byte HOMEPIN = 9;  //the pin which controls the LED in the house
const byte DOORPIN = 5;  //the pin which will power the transistor to open/close the door
unsigned long openTime = 0;  //the time that the door was opened according to the millis command
unsigned long autoCloseTime = 7200000; //2 hours in miliseconds  
int lowCloseTemp = 0;
int highCloseTemp = 100;
boolean AutoCloseEnabled = true;
int tempSensorPin = A0;
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
      openTime = millis();
      wasOpen = true;
    }
    else{
      if(AutoCloseEnabled){
        if(openTime+autoCloseTime < millis()){
          changeDoorState();
        }else if((getTemp() > highCloseTemp) || (getTemp() < lowCloseTemp)){
          changeDoorState();
        }
      }
    }
    isOpen = true;
    digitalWrite(HOMEPIN, HIGH);  //light up the LED if it's open
  }
}

/*This should get called if the button is pressed in order to close 
 *the garrage door using an interrupt or if the user hits the button
 *on the website to close the garage door
 */
void changeDoorState(){
  digitalWrite(DOORPIN, HIGH);  //switch on
  delay(250);                   //wait
  digitalWrite(DOORPIN, LOW);   //switch off
}

/**
 * get's the current temperature of the garage
 */
float getTemp(){
  int reading = analogRead(tempSensorPin);
  float volt = reading*5.0;
  volt /= 1024;
  float tempC = (volt - .5)*100;
  return ((tempC*9.0/5.0)+32); //convert to ferinheit
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
  P(form) = "<form action=\"control.htm\" method=\"post\"> \n";
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
    server.printP(buttonClose);
  }
  else{
    server.printP(buttonOpen);
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
  changeDoorState();
  File control = SD.open("control.htm");
  if(control){
    server.httpSuccess();
    while(control.available()){
      server.print((char)control.read());
    }
    control.close();
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
  }
  else{
    server.httpFail();
  }
}

#define NAMELEN 32
#define VALUELEN 32

/**
 * this will take care of any changes to the configuration file, updating the memory, and writing them to a file
 * The variables will be gotten using an http Get request from the form on the config.html page
 */
void changeConfigCommand(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete){
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  int  name_len;
  char value[VALUELEN];
  int value_len;
  boolean foundEnable = false;
  if(type == WebServer::GET){
      server.httpSuccess();
      if(strlen(url_tail)){
        while(strlen(url_tail)){
          rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
          if (rc == URLPARAM_EOS){
            break;
          }else{
            if(name == "enableAutoClose"){
              AutoCloseEnabled = true;
              foundEnable = true;
            }else if(name == "delay"){
              if(value == "0"){
                AutoCloseEnabled = false;
              }else if(value == "1"){
                autoCloseTime = 900000;
              }else if(value == "2"){
                autoCloseTime = 1800000;
              }else if(value == "3"){
                  autoCloseTime = 2700000;
              }else if(value == "4"){
                  autoCloseTime = 3600000;
              }else if(value == "5"){
                  autoCloseTime = 7200000;
              }
            }else if(name == "tempLow"){
              lowCloseTemp = (int)value;
            }else if(name == "tempHigh"){
              highCloseTemp = (int)value;
            }
          }
        }
      }if(!foundEnable){
        AutoCloseEnabled = false;
      }
      SD.remove("config.txt");
      File configFile = SD.open("config.txt", FILE_WRITE);
      configFile.println("autoCloseEnabled=" + AutoCloseEnabled);
      configFile.println("autoCloseTime=" + autoCloseTime);
      configFile.println("autoCloseHigh=" + highCloseTemp);
      configFile.println("autoCloseLow=" + lowCloseTemp);
      configFile.close();
  }
}

/**
 * the fail command in case of a 404 or other erros
 */
void viewConfigCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  File failPage = SD.open("404.htm");
  if(failPage){
    while(failPage.available()){
      server.write((char)failPage.read());
    }
    failPage.close();
  }
}

/**
 * displays the config.txt which contains all the configuration data
 */
void failCommand(WebServer &server, WebServer::ConnectionType type, char *, bool){
  File config = SD.open("config.txt");
  if(config){
    while(config.available()){
      server.write((char)config.read());
    }
    config.close();
  }
}

/**
* this reads the contents of the config file and sets the variables accordingly
* Line 1: autoclose enabled
* Line 2: autoclose delay
* Line 3: autoclose on low temp
* Line 4: autoclose on high temp
*/
void readConfig(){
  File config = SD.open("config.txt");
  String value;
  byte line = 1;
  boolean passedEquals = false;
  if(config){  //the file exists, read it
    byte nextChar = config.read();
    while(nextChar != -1){
      if(passedEquals){
        if(nextChar != 10){
          value = value + (char)nextChar;
        }else{
          if(line == 1){
            if(value.charAt(0) == 't'){
              AutoCloseEnabled = true;
            }else{
              AutoCloseEnabled = false;
            }
            line++;
          }else if(line == 2){
            char buf[value.length()];
            value.toCharArray(buf, value.length());
            autoCloseTime = (long)buf;
            line++;
          }else if(line == 3){
            char buf[value.length()];
            value.toCharArray(buf, value.length());
            highCloseTemp = (int)buf;
            line++;
          }else if(line == 4){
            char buf[value.length()];
            value.toCharArray(buf, value.length());
            highCloseTemp = (int)buf;
            config.close();
            return;
          }
          passedEquals = false;
        }
      }
      else if(nextChar == 61){  //ascii = sign
        passedEquals = true;
        value = "";
      }
      nextChar = config.read();
    }
  }else{  //write the default values if the file does not exist already on the sd card
      config.println("autoCloseEnabled=false");
      config.println("autoCloseTime=7200000");
      config.println("autoCloseHigh=100");
      config.println("autoCloseLow=0");
  }
  config.close();
}

/**
 *  The initial setup of the program, it is only called once to initialize everything
 *  Initialize the pins, setup the ethernet and sd card, as well as add all the commands for the webserver
 */
void setup() {
  pinMode(HOMEPIN, OUTPUT);  //the pin with the LED for notification
  pinMode(DOORPIN, OUTPUT);  
  digitalWrite(DOORPIN, LOW);
  pinMode(tempSensorPin, INPUT);
  Ethernet.begin(mac, ip);
  SD.begin(4);
  readConfig();
  webserver.setDefaultCommand(&statusCommand);
  webserver.addCommand("index.htm", &statusCommand);  //the home page which gives you the status, same as above
  webserver.addCommand("control.htm", &controlCommand);  //this raises or lowers the garage door
  webserver.addCommand("config.htm", &configCommand);  //this is the one that will display the form to change the config
  webserver.addCommand("changeConfig.htm", &changeConfigCommand);  //this is used to actually change the settings
  webserver.addCommand("config.txt", &viewConfigCommand);  //view the config text file
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
