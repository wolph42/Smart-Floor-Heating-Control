#include <W42_Thermostat_Header.h>
const int FW_VERSION = 63;

#define BCK 1                                     // COMPILE FOR: MAIN(0) or BACKUP(1)

#if BCK == 0
  char ESP_NAME[]           = "ESP_CENTRAL";
#endif
#if BCK == 1
  char ESP_NAME[]           = "ESP_BCK_CENTRAL";
#endif

String idColor              = CCENTRAL;
String idBCKColor           = CBCKCENTRAL;

char auth[]                 = auth_CENTRAL;       // assign correct token number, stored in W42 header
const float WIFIPOWER       = 10;                 // range from 0 to +20.5dBm, where 0 is NOT off, but the lowest setting

// constructors
BlynkTimer timer;                                 // initialize timer
//SimpleTimer timer;                                 // initialize timer
WidgetTerminal terminal(BCK?TERMINALBCK_PIN:TERMINAL_PIN);  // initialize Terminal widget use different pins for CENTRAL and BCK
WidgetRTC rtc;                                    // initialize Real-Time-Clock. Note that you MUST HAVE THE WIDGET in blynk to sync to!!!
CONNECTION_STATE connectionState;

/////////////////////////////// BLYNK//////////////////////////////////////////
// enum ROOM_INDEX { LIVING, STUDY, BATH, BED, LAUNDRY, ARTHUR, CASPER }; // so: 0,1,2,3,4,5 and 6
// enum FLOOR_INDEX { FLOOR_1, FLOOR_2, FLOOR_3, FLOOR_4}; // so: 0,1,2,3

String tokenRLY[4] = {auth_R_FLOOR_1, auth_R_FLOOR_2, auth_R_FLOOR_3, auth_R_FLOOR_4}; // used for alive check
String tokenTHM[7] = {auth_T_LIVING, auth_T_STUDY, auth_T_BATH, auth_T_BED, auth_T_LAUNDRY, auth_T_ARTHUR, auth_T_CASPER}; // used for alive check
bool heatOn[7] = {false,false,false,false,false,false,false};  // if a valve is opened (heatOn=true) then start CV
bool cvOn = false;
bool stopAllProcesses = false;
bool bck_active = false;                          // used for CENTRAL_BCK notification
float targetT_Vacation = 5;                       // preset target temperature during vacation

const int HEATER_RELAY_PIN_CV = D1;               // the pin that the CV Relay is attached to: LOW LVL TRIGGER (so GRND is ON)
const int CENTRAL_RELAY_PIN = D2;                 // the pin that the CENTRAL Relay is attached to: NORMALLY CLOSED, so HIGH=OFF/OPEN. ONLY BCK is connected to this one. And is used to reset CENTRAL in case its 'stuck'.
const unsigned long BCK_TIME_SLEEP = 10 * 60e6;   // sleep cycles are in us, so 60e6=1 minute
int HTTP_OTA = 0;                                 // when switch on app is hit, the ESP searches for and firmware.bin update
int initializing = 1;                             // during setup the CV check is called 14 times, pushing its value to the terminal. This prevents that// used to ignore previously saved state before booot (which can prevent the CV from turning on!!
int reconnectionAttempts=0;                       // used to track the amount of reconnection trials
int wifiID;
uint8_t connectionCounter;
bool backOnlineNotification = false;              // prevent 'back online' message too much
void toTerminal(String input, bool showDate = true) {
    terminal.println(String(showDate?(getDateTime() + "-"):"") + input);
    terminal.flush();
    Serial.println( String(showDate?(getDateTime() + "-TO TERM.:"):"TO TERM.:") + input);
}

///////////////////////////////// REAL TIME CLOCK//////////////////////////////
String getTime(){     return String((hour()<10)?"0":"") + hour() + ":"  + String((minute()<10)?"0":"") + minute() + ":" + String((second()<10)?"0":"") + second();  }
String getDate(){     return String(day()) + "/" + month();      }
String getDateTime(){ return getDate() + "-" + getTime();                       }

void updateTerminalLAbel(){                       // Digital clock display of the time
 int wifisignal = map(WiFi.RSSI(), -105, -40, 0, 100);

 int gmthour = hour();
  if (gmthour == 24){
     gmthour = 0;
  }
  String displayhour =   String(gmthour, DEC);
  int hourdigits = displayhour.length();
  if(hourdigits == 1){
    displayhour = "0" + displayhour;
  }
  String displayminute = String(minute(), DEC);
  int minutedigits = displayminute.length();  
  if(minutedigits == 1){
    displayminute = "0" + displayminute;
  }  
  // label for terminal
  displaycurrenttimepluswifi = String(ESP_NAME) + " (v.:" + FW_VERSION + ")                       Clock:  " + displayhour + ":" + displayminute + "               Signal:  " + wifisignal +" %";
  Blynk.setProperty(BCK?TERMINALBCK_PIN:TERMINAL_PIN, "label", displaycurrenttimepluswifi);
}

void showTime(){
  Blynk.virtualWrite(SHOW_TIME_PIN, getDateTime());
}
///////////////////////////////// REAL TIME CLOCK//////////////////////////////

/////////////////////// CONNECTION CONTROL ///////////////////////////
//Created by: ohjohnsen
//https://community.blynk.cc/t/esp8266-how-to-properly-connect-and-maintain-connection-to-blynk-local-server/26846/4
void ConnectionHandler(void) {
  switch (connectionState) {
  case CONNECT_TO_WIFI:
  Serial.println(String("Connecting to ") + ssid);
    WiFi.begin(ssid, pass);
    connectionState = AWAIT_WIFI_CONNECTION;
    connectionCounter = 0;
    break;

  case AWAIT_WIFI_CONNECTION:
    if (WiFi.status() == WL_CONNECTED) {
    Serial.println(String("Connecting to ") + ssid);
      connectionState = CONNECT_TO_BLYNK;
    }
    else if (++connectionCounter == 50) {
      Serial.println(String("Unable to connect to ") + ssid + ". Retry connection.");
      WiFi.disconnect();
      connectionState = AWAIT_DISCONNECT;
      connectionCounter = 0;
    }
    break;

  case CONNECT_TO_BLYNK:
    Serial.println("Attempt to connect to Blynk server.");
    Blynk.config(auth, server, port);
    Blynk.connect();
    connectionState = AWAIT_BLYNK_CONNECTION;
    connectionCounter = 0;
    break;

  case AWAIT_BLYNK_CONNECTION:
    if (Blynk.connected()) {
      Serial.println("Connected to Blynk server.");
      connectionState = MAINTAIN_CONNECTIONS;
    }
    else if (++connectionCounter == 50) {
      Serial.println("Unable to connect to Blynk server. Retry connection.");
      Blynk.disconnect();
      WiFi.disconnect();
      connectionState = AWAIT_DISCONNECT;
      connectionCounter = 0;
    }
    break;

  case MAINTAIN_CONNECTIONS:
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wifi connection lost. Reconnect.");
      Blynk.disconnect();
      WiFi.disconnect();
      connectionState = AWAIT_DISCONNECT;
      connectionCounter = 0;
    }
    else  if (!Blynk.connected()) {
      Serial.println("Blynk server connection lost. Reconnect.");
      Blynk.disconnect();
      connectionState = CONNECT_TO_BLYNK;
    }
    else {
      Blynk.run();
    }
    break;

  case AWAIT_DISCONNECT:
    if (++connectionCounter == 10) {
      connectionState = CONNECT_TO_WIFI;
    }
    break;
  }
}
/////////////////////// CONNECTION CONTROL ///////////////////////////


///////////////// API BRIDGE FUNCTIONS/////////////////////////////////
// created by WANEK: https://community.blynk.cc/t/a-substitute-for-bridge-for-lazy-people/24128 
bool deviceAlive(String token){
  HTTPClient http;                                // Create: http://blynk-cloud.com/383d08989c24456dbdf28bf268807c7c/get/V16
  String payload = "request failed";
  String url = "http://192.168.1.93:8080/" + token + "/isHardwareConnected";
  http.begin(url);
  int httpCode = http.GET();
  delay(50);

  if (httpCode > 0) {
    payload = http.getString();                   // get response payload as String, value = true or false
  }  else payload = payload + ", httpCode: " + httpCode;
  http.end();
  delay(10);
  return (payload=="true")?1:0;
}

void APIwriteDevicePin(String token, int pin, String value){
  String spin = String(pin);                      // convert pint number to string
  HTTPClient http;                                // Create:http://192.168.1.93:8080/383d08989c2zzdbdf28bf268807c7c/update/v14?value=42
  String url = "http://192.168.1.93:8080/";       // url -- http://IP:port
  url += token;                                   // blynk token
  url += "/update/V";
  url += spin;                                    // pin to update
  url += "?value=";
  url += value;                                   // value to write
//  Serial.print("Value send to server: ");
//  Serial.println(url);
  http.begin(url);
  http.GET();
  delay(50);
  http.end();
  delay(10);
}
//////////////// API BRIDGE FUNCTIONS/////////////////////////////////

/////////////////////////////// FUNCTIONS//////////////////////////////////////////
String whichCV(){
  String CVs = String("");
  bool first = true;
  
  for(int i=0;i<7;i++){
    if(heatOn[i]){
      CVs = CVs + (first?"":",") + i;
      first = false;
    }
  }
  Serial.println(CVs);
  return CVs;
}

void checkCV(){                                   // turns central heating on or off. 
  bool allHeat = heatOn[0]||heatOn[1]||heatOn[2]||heatOn[3]||heatOn[4]||heatOn[5]||heatOn[6];
  Serial.println(String("CV update, allheat/cvon/initializing/stopAllprocesses: ") + String(allHeat) + "/" + String(cvOn) +  "/" + String(initializing) +  "/" + String(stopAllProcesses));
  if(allHeat && (!cvOn || initializing) && !stopAllProcesses){
    Serial.print("CV TURNED ON, initializing=");    Serial.println(initializing);
    if(!initializing){ 
      toTerminal(String("CV TURNED ON BY: ") + whichCV());
    }
    pinMode(HEATER_RELAY_PIN_CV, OUTPUT);
    digitalWrite(HEATER_RELAY_PIN_CV, LOW);       // activate = LOW, deactivate = HIGH
    Blynk.virtualWrite(CV_PIN, "AAN");
    Blynk.setProperty(CV_PIN, "color", RED);
    cvOn = true;
    max(0,initializing);
  }else if(  (!allHeat || stopAllProcesses) && (cvOn || initializing)){
    pinMode(HEATER_RELAY_PIN_CV, OUTPUT);
    digitalWrite(HEATER_RELAY_PIN_CV, HIGH);      // activate = LOW, deactivate = HIGH
    if(!initializing){ 
      toTerminal("CV TURNED OFF");
    }
    Blynk.virtualWrite(CV_PIN, "UIT");
    Blynk.setProperty(CV_PIN, "color", GRAY);
    cvOn = false;
  }
}

void resetWidgetColors(){
  Serial.println("Resetting Colors");
  for(int vpin=0; vpin<64; vpin+=1) Blynk.setProperty(vpin, "color", BLYNK_GREEN);  // change colors of the widgets used in 'nerd' section
  Blynk.setProperty(offline_PIN, "color", RED);                                     // change LED color
  Blynk.setProperty(CV_PIN, "color", RED);                                          // change LED color or ERROR to red.

  Blynk.setProperty(TERMINAL_PIN, "color", idColor);                                // change Terminal color.
  Blynk.setProperty(TERMINALBCK_PIN, "color", idBCKColor);                          
  
  Blynk.setProperty(OTA_update_PIN, "offBackColor", idColor);                       // change Update color.
  Blynk.setProperty(OTA_BCK_update_PIN, "offBackColor", idBCKColor);                

  Blynk.setProperty(version_PIN, "color", idColor);                                 // change Version color. 
  Blynk.setProperty(version_BCK_PIN, "color", idBCKColor);                          
}

void wifisignal(){
  int wifisignal = map(WiFi.RSSI(), -105, -40, 0, 100);
  toTerminal(String(wifisignal));
}

void resetESP(){
    toTerminal(String("ESP RESET"));
    delay(3000);
    ESP.restart();
    delay(5000);
}
/*
void reconnectBlynk() {
  if (!Blynk.connected()) {
    //@@@
	WiFi.disconnect(); //gonna give this one a shot!!
    WiFi.mode(WIFI_STA); // added in V 3.1a to disable AP_SSID publication in Client mode - default was WIFI_AP_STA
    //WiFi.begin(WIFI_SSID, WIFI_PASS);  // connect to local WIFI Access Point
    Blynk.begin(auth, ssid, password, server, port);
	//@@@
	Serial.println("Lost connection for " + String(reconnectionAttempts++) + "minutes");
    if(Blynk.connect()) {
      Serial.println("Reconnected");
      reconnectionAttempts = -1;
    }
    else {
      reconnectionAttempts++;
      Serial.println("Not reconnected");
      if(reconnectionAttempts == 5){
        Serial.println("Lost connection for over 5 minutes, releasing relay command to Central Backup");
		pinMode(HEATER_RELAY_PIN_CV, OUTPUT);
        digitalWrite(HEATER_RELAY_PIN_CV, HIGH);        // activate = LOW, deactivate = HIGH		
      }// recon. att.
      if(!reconnectionAttempts%10) resetESP();			// reset ESP every 10 min.
    }// blynk.connect
  }// blynk.connected
}// reconnectblynk
*/
void blinkLED(){
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);                           // turn the LED on/off (HIGH is the voltage level)
  delay(100);
  digitalWrite(2, HIGH);                          // turn the LED on/off (HIGH is the voltage level)
}
/////////////////////////////// FUNCTIONS//////////////////////////////////////////

/////////////////////////////// EVENT HANDLERS//////////////////////////////////////////
BLYNK_CONNECTED() {                               // sync pins as stored last on server.
                                                  // order of syncing is important due to BCK unit
  rtc.begin();                                    // sync clock with server AFTER blynk has connected with said server. 
  ESP.wdtFeed();                                  // prevent wdt time-out
  Blynk.syncVirtual(OTA_BCK_update_PIN);          // make sure BCK updates before going to sleep
  Blynk.syncVirtual(offline_PIN);                 // puts BCK back to sleep!! (if main is online)
  Blynk.syncVirtual(OTA_update_PIN);
  Blynk.syncVirtual(1,2,3,4,5,6,7);               // heat states of thermometers
  Blynk.syncVirtual(targetTType_PIN);             // home/night/vacation
  ESP.wdtFeed();                                  // prevent wdt time-out

  updateTerminalLAbel();                          // immediately update terminal else it takes a minute
  initializing = 1;                               // overrides the 'cvOn' state which might be stored during boot!!
  pinMode(HEATER_RELAY_PIN_CV, OUTPUT);           // Make sure that relay pin is setup correctly
  checkCV();                                      // check if state has changed during offlne state
  initializing = 0;                               
  
  if(BCK){
    toTerminal("ESP BCK online, attempt to reset CENTRAL: LOW");
    pinMode(CENTRAL_RELAY_PIN, OUTPUT);           // Make sure that relay pin is setup correctly
    digitalWrite(CENTRAL_RELAY_PIN, LOW);         // TURN OFF ESP_CENTRAL.
    delay(1000);
    toTerminal("and back to... HIGH");
    digitalWrite(CENTRAL_RELAY_PIN, HIGH);        // TURN ON ESP_CENTRAL. This basically RESETS CENTRAL ESP
  }    
  
  toTerminal("======RECONNECTED=====");
  ESP.wdtFeed();                                  // prevent wdt time-out
} 

BLYNK_WRITE(offline_PIN){                         // put in a seperate routine due to stack overflow issue
  Serial.println(String("Write to offline_pin: ") + param.asStr());
  if(!BCK){               // MAIN UNIT
    if(param.asInt() != -1){                        // prevent eternal sync loop
      Blynk.virtualWrite(offline_PIN, -1);          // reset the 'offline' pin when back online. Use -1 as this allows RLY to reset everything and set it automaticallly to 0 as it usess ++
      APIwriteDevicePin(auth_CENTRAL, offline_PIN, String(-1)); //extra measure to attempt to kick BCK back to sleep as virtualWrite does not always initiate blynk_write for the other unit (with the same token)
    }
    if(!backOnlineNotification){ 
      Blynk.notify("Central MAIN is back online");
      backOnlineNotification = true;
    }
  } else {                                        // BCK UNIT ; MAIN unit is offline 
    if(param.asInt() < 0){
      toTerminal(String("Offline_Pin value: ") + param.asStr() + " BACK TO SLEEP");
      ESP.deepSleep(BCK_TIME_SLEEP);              // main is (back) online, go back to sleep (runs setup() first on wakeup (i hope)
    }
    if(param.asInt() > 5 && !bck_active){
      Blynk.notify(String("Central MAIN is offline for ") + param.asStr() + "m now, BACKUP is taking over!");
      toTerminal("CENTRAL BCK TAKING OVER THE WORLD");
      bck_active = true;
      pinMode(HEATER_RELAY_PIN_CV, OUTPUT);
      digitalWrite(HEATER_RELAY_PIN_CV, HIGH);        // activate = LOW, deactivate = HIGH

    } 
  }
}                             

BLYNK_WRITE_DEFAULT(){                            // this routine is activated when ANY of the CENTRAL Vpins are changed. 
  int pin = request.pin;
  Serial.println(String("Incomming pin/value: ") + String(pin) + " / " + param.asStr());
  
  if (pin == OTA_update_PIN && !BCK){             // switch in app to check for updates
    HTTP_OTA = param.asInt();                     // on the pin is the old version stored OR its '1' in case an update is due
    if(HTTP_OTA > 1){                             // an OTA update just took place, check if version number updated correctly
      toTerminal(String(ESP_NAME) + " UPDATED: old version: " + String(HTTP_OTA) + " new version: " + FW_VERSION,0);
      HTTP_OTA = 0;                               // reset bool
      Blynk.virtualWrite(OTA_update_PIN, HTTP_OTA);
      Blynk.virtualWrite(version_PIN, FW_VERSION);
    }
  }

 if (pin == OTA_BCK_update_PIN && BCK){           // switch in app to check for updates
    HTTP_OTA = param.asInt();                     // on the pin is the old version stored OR its '1' in case an update is due
    if(HTTP_OTA > 1){                             // an OTA update just took place, check if version number updated correctly
      toTerminal("ESP CENTRAL UPDATED: old version: " + String(HTTP_OTA) + " new version: " + FW_VERSION,0);
      HTTP_OTA = 0;                               // reset bool
      Blynk.virtualWrite(OTA_BCK_update_PIN, HTTP_OTA);
      Blynk.virtualWrite(version_BCK_PIN, FW_VERSION);
    }
    if(HTTP_OTA == 1){                             // This is required for BCK or else it will never update!! server update (upload file to: http://192.168.1.93/fota/
      HTTP_OTA = FW_VERSION;                       // use H_OTA to store old version number in to compare with new one on reboot. This will also prevent eternal update loop
      Blynk.virtualWrite(OTA_BCK_update_PIN, HTTP_OTA); // use separate pin for BCK and CENTRAL else they influence eachother (same token)
      String returnTxt = checkForUpdates(ESP_NAME);       // if successful the ESP reboots after this line, if not it will return a string with the error
    toTerminal(returnTxt);
    }  
  }

  if( pin >= 1 && pin <= 7 ){                     // Heat update from room thermostats
    int i = pin - 1;                              // i runs from 0 to 6, rooms from 1 to 7
    heatOn[i] = (bool) param.asInt();             // store value that came via Bridge from THERMOSTAT
    checkCV();                                    // check whether the central heating unit needs to be turned on or off.
  }
  if (pin == TERMINAL_PIN && !BCK || pin == TERMINALBCK_PIN && BCK){  // input from the terminal or bckterminal
    Serial.println(String("Incomming message from Terminal: ")+param.asStr());
    if (String("reset") == param.asStr()) { 
      resetESP();  
    }
    if (String("stop") == param.asStr()) {        // BEGIN: turn off CV no matter what.
      stopAllProcesses = true;  
      toTerminal("ALL PROCESSES HALTED, CV OFF",false); 
      checkCV();
    } 
    if (String("start") == param.asStr()) {       // END: turn off CV no matter what.
      stopAllProcesses = false;  
      toTerminal("ALL PROCESSES CONTINUING...",false); 
      checkCV(); 
    }
    if (String("cv on") == param.asStr()) {       // END: turn off CV no matter what.
      pinMode(HEATER_RELAY_PIN_CV, OUTPUT);
      digitalWrite(HEATER_RELAY_PIN_CV, LOW);     // activate = LOW, deactivate = HIGH
      toTerminal("CV is turned on",false); 
    }
    if (String("cv off") == param.asStr()) {       // END: turn off CV no matter what.
      pinMode(HEATER_RELAY_PIN_CV, OUTPUT);
      digitalWrite(HEATER_RELAY_PIN_CV, HIGH);     // activate = LOW, deactivate = HIGH
      toTerminal("CV is turned off",false); 
    }
    if (String("check cv") == param.asStr()) {       // END: turn off CV no matter what.
      toTerminal("CV is back to normal",false); 
      checkCV(); 
    }
    // place for other commands
  }
  if(pin == targetTType_PIN){                     // sets the targetT for ALL THM units (so home=1/night=2/vacation=3)
    int targetTType = param.asInt()-1;            // button returns 1,2,3 need 0,1,2
    toTerminal(  String("target T Type: ") + String(targetTType==0?"Home":targetTType==1?"Night":"Vacation")  );
    String color = setTargetTCOLOR[targetTType];
    Blynk.setProperty(targetTType_PIN, "color", color);                           // 3 BUTTON WIDGET
    for(int i=0; i<7; i++) APIwriteDevicePin(tokenTHM[i], targetTType_PIN, param.asStr());
  }
  
  if(pin == targetTMIN_PIN){                      // targetT during vacation. Note that home and night targetT is set by each individual THM.
    targetT_Vacation -= 0.5;
    Blynk.virtualWrite(vacationTargetT_PIN, floatToString(targetT_Vacation,1));
    for(int i=0; i<7; i++) APIwriteDevicePin(tokenTHM[i], vacationTargetT_PIN, String(targetT_Vacation));
  }
  if(pin == targetTPLUS_PIN){                     // targetT during vacation
    targetT_Vacation += 0.5;
    Blynk.virtualWrite(vacationTargetT_PIN, floatToString(targetT_Vacation,1));
    for(int i=0; i<7; i++) APIwriteDevicePin(tokenTHM[i], vacationTargetT_PIN, String(targetT_Vacation)); 
  }
  if (pin == wifiSignal_PIN){                     // button to turn on signal wifi checker
    if(param.asInt()){
      wifiID = timer.setInterval(300, wifisignal);// check wifi signal every 300ms and send to serial port and terminal
    }else{
      timer.deleteTimer(wifiID);
    }
  }

}
/////////////////////////////// EVENT HANDLERS//////////////////////////////////////////
/////////////////////////////// SETUP////////////////////////////////////////////////
void setup() {
  ///////////////////////////////////// BLYNK//////////////////////////////
  Serial.begin(9600);
  Serial.println("Starting Blynk");
/*  
  Blynk.begin(auth, ssid, password, server, port);
  while (Blynk.connect() == false) { }            // Wait until connected  
  //resetWidgetColors();
  
 
  toTerminal("-------------");
  toTerminal(String("Blynk v") + BLYNK_VERSION + " started");
  toTerminal("-------------");
*/
  connectionState = CONNECT_TO_WIFI;
  ///////////////////////////////////// OTA//////////////////////////////
  toTerminal("Start OTA");
  initOTA(ESP_NAME);

  ///////////////////////////////////// INIT PINS CV and RELAY ARE done in BLYNK_CONNECT
  pinMode(2, OUTPUT);                               // ESP LED for alive routine

  ///////////////////////////////////// INIT TIMERS//////////////////////////////
  timer.setInterval(TIME_RUN_FUNCTIONS, runFunctions);               // every 1 minute: run time functions.
  runFunctions();                                                    // make sure they also run on startup
  timer.setInterval(1000, showTime);                                 // update clock in app every second
  timer.setInterval(TIME_CHK_CONNECTION, ConnectionHandler);         // connect to wifi and blynk

  ///////////////////////////////////// ALIVE//////////////////////////////
  Serial.println("Starting 'alive' routine...");
  timer.setInterval(4000, blinkLED);               // led on/off every second

  ///////////////////////////////////// ONE TIME STUFF//////////////////////////////
  Blynk.setProperty(offline_PIN, "color", RED);    // change LED color
}
/////////////////////////////// SETUP////////////////////////////////////////////////
void runFunctions(){
  updateTerminalLAbel();                           // every 1 minute: update the label on the terminal (time/wifi strength/version)
  //reconnectBlynk();                                // check every minute if still connected to server
}

void loop() {
  timer.run(); 
  if(Blynk.connected()) { Blynk.run(); }
  ArduinoOTA.handle();                              // the 'normal' OTA update.
  if(HTTP_OTA == 1){                                // server update (upload file to: http://192.168.1.93/fota/
    HTTP_OTA = FW_VERSION;                          // use H_OTA to store old version number in to compare with new one on reboot. This will also prevent eternal update loop
    Blynk.virtualWrite(BCK?OTA_BCK_update_PIN:OTA_update_PIN, HTTP_OTA); // use separate pin for BCK and CENTRAL else they influence eachother (same token)
    toTerminal("OTA before check: " + String(HTTP_OTA) + " FW Version: " + String(FW_VERSION));
    String returnTxt = checkForUpdates(ESP_NAME);   // if successful the ESP reboots after this line, if not it will return a string with the error
    toTerminal(returnTxt);
    Blynk.syncVirtual(OTA_update_PIN);              // force blynk_write with updated (>1) http_ota value; on successful update this line should never execute!
 }  
}

// ==================================================================DEVELOPMENT==
