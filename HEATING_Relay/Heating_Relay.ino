#include <W42_Thermostat_Header.h>
const int FW_VERSION = 65;
// UPDATE PER FLOOR: updated 
#define FLOOR 2                                   // used during initialization and below
// UPDATE PER FLOOR 

#if FLOOR == 1                                    // ONLY floor 1 check for main online!!!
  char ESP_NAME[] = "ESP_RLY_FLOOR_1";
  char auth[]     = auth_R_FLOOR_1;
  char auth_THM[] = auth_T_LIVING;
  String idColor  = CLIVING;
  const float WIFIPOWER = 10;                     // range from 0 to +20.5dBm, where 0 is NOT off, but the lowest setting
  const int ROOM  = 1;                            // the room the relays is coupled to
#endif

#if FLOOR == 2
  char ESP_NAME[] = "ESP_RLY_FLOOR_2";
  char auth[]     = auth_R_FLOOR_2;
  char auth_THM[] = auth_T_STUDY;
  String idColor  = CSTUDY;
  const float WIFIPOWER = 10;                     
  const int ROOM  = 2;
#endif

#if FLOOR == 3
  char ESP_NAME[]   = "ESP_RLY_FLOOR_3";
  char auth[]       = auth_R_FLOOR_3;
  char auth_THM[]   = auth_T_BATH;                // D2
  char auth_THM1[]  = auth_T_BED;                 // D5 used on floor 3
  char auth_THM2[]  = auth_T_LAUNDRY;             // D6
  char auth_THM3[]  = auth_T_ARTHUR;              // D7
  String idColor    = CBATH;
  const float WIFIPOWER = 10;                     
  const int ROOM    = 3;
#endif

#if FLOOR == 4
  char ESP_NAME[] = "ESP_RLY_FLOOR_4";
  char auth[]     = auth_R_FLOOR_4;
  char auth_THM[] = auth_T_CASPER;
  String idColor  = CCASPER;
  const float WIFIPOWER = 10;                     
  const int ROOM  = 7;
#endif

// Note that there is no mem difference between const and define, however define is more bug prone!
// the pin that the Relay is attached to: LOW LVL TRIGGER (so GRND is ON)
// most rooms are connected to the 1st pin only D2. Only 3rd floor has 4 pins in use). 
// so the order is: LIVING=D2, STUDY=D2, BATH=D2, BED=D5, LAUNDRY=D6, ARTHUR=D6, CASPER=D2
const int HEATER_RLY_PIN[7]                 = {D2, D2, D2, D5, D6, D7, D2}; // safest pins to use are D1 and D2, then 5,6, and 7. The latter 3 go HIGH on boot then LOW. D=GPIO: D1=5,D2=4,D5=14,D6=12,D7=13
const int HEATER_RLY_PIN_PUMP               = D1;                           // the pin that the Relay is attached to: LOW LVL TRIGGER (so GRND is ON)
const long TIME_START_MAINTENANCE           = 1000 * 60 * 60 * 12;          // time to start maintenance cycle when valve state has not changed
const long TIME_STOP_MAINTENANCE            = 1000 * 60 * 5;                // duration of the maintenance cycle
const long TIME_CENTRAL_ONLINE_CHECK        = 1000 * 60 * 1;                // check if central is online every minute
const long TIME_CHECK_CENTRAL_NOTIFICATION  = 1000 * 60 * 30;               // check if central is offline for longer than 30m
uint8_t connectionCounter;
int tmpLambda = 0;                                                          // a tmp var to pass on non global variables to a lambda functions
// debug mode:                                                          
// const long TIME_START_MAINTENANCE = 1000 * 15;  // time to start maintenance cycle when valve state has not changed
// const long TIME_STOP_MAINTENANCE  = 1000 * 10;  // duration of the maintenance cycle

///////////////////////////////////VARIABLES//////////////////////////////////////////////// 
// constructors
// SimpleTimer timer;                              // initialize timer construct
BlynkTimer timer;
WidgetTerminal terminal(TERMINAL_PIN);            // initialize Terminal widget ; V124 defined in W42_header
WidgetRTC rtc;                                    // initialize Real-Time-Clock. Note that you MUST HAVE THE WIDGET in blynk to sync to!!!
CONNECTION_STATE connectionState;

WidgetBridge bridge_thm(channel1_PIN); 
#if FLOOR == 3
  WidgetBridge bridge_thm1(channel2_PIN); 
  WidgetBridge bridge_thm2(channel3_PIN); 
  WidgetBridge bridge_thm3(channel4_PIN); 
#endif

bool heatOn[7] = {false,false,false,false,false,false,false};          // turn on the heat or not (so: open valve or not)
bool maintenanceOn_V[7] = {false,false,false,false,false,false,false}; // open/close valve every so many hours
int maintenanceTimer_V[7];                                             // identifier of the valvemaintenance timer
bool maintenanceOn_P = false;                                          // Make sure the pump runs 5m every 12 hours
int maintenanceTimer_P;                                                // identifier of the pump maintenance timer
bool initialize = true;
bool stopPump = false;                                                 // override ALL commands
bool stopValve[7] = {false,false,false,false,false,false,false};       // override ALL commands
bool centralOffline = false;                                           // is central offline?
int HTTP_OTA = 0;                                                     // when switch on app is hit, the ESP searches for and firmware.bin update
int wifiID;

///////////////////////////////////VARIABLES//////////////////////////////////////////////// 

////////////////////////////////////////////////////////////////////////////// 
////////////////////////////// FUNCTIONS//////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////////// 
void checkPump();
void resetMaintenanceTimerValve(int i);
void terminalCommands(String param);

void toTerminal(String input, bool showDate = true) {
  terminal.println(String(showDate?(getDateTime() + "-"):"") + input);
  terminal.flush();
  Serial.println(String("TO TERM.:") + input);
}

/////////////////////////////////REAL TIME CLOCK////////////////////////////// 
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
  displaycurrenttimepluswifi = String(ESP_NAME) + " (v.:" + FW_VERSION + ")                    Clock:  " + displayhour + ":" + displayminute + "               Signal:  " + wifisignal +" %";
  Blynk.setProperty(TERMINAL_PIN, "label", displaycurrenttimepluswifi);
}
/////////////////////////////////REAL TIME CLOCK////////////////////////////// 
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
///////////////////////Connection Control///////////////////////////
///////////////////////////////NOTIFICATIONS///////////////////////////
/////////////////API BRIDGE FUNCTIONS/////////////////////////////////
bool deviceAlive(String token){
  HTTPClient http;                                // Create:http://192.168.1.93:8080/383d08989c2zzdbdf28bf268807c7c/isHardwareConnected
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
// created by WANEK: https://community.blynk.cc/t/a-substitute-for-bridge-for-lazy-people/24128 
  String spin = String(pin);                      // convert pint number to string
  HTTPClient http;                                // Create:http://192.168.1.93:8080/383d08989c2zzdbdf28bf268807c7c/update/v14?value=42
  String url = "http://192.168.1.93:8080/";       // url -- http://IP:port
  url += token;                                   // blynk token
  url += "/update/V";
  url += spin;                                    // pin to update
  url += "?value=";
  url += value;                                   // value to write
  Serial.print("Value send to server: ");
  Serial.println(url);
  http.begin(url);
  http.GET();
  delay(50);
  http.end();
  delay(10);
}

String APIreadDevicePin(String token, int pin){
  String spin = String(pin);                      // convert pint number to string
  HTTPClient http;                                // create: // http://192.168.1.93:8080/383d08989c2zzdbdf28bf268807c7c/get/pin
  String payload = "request failed";
  String url = "http://192.168.1.93:8080/";       // url -- http://IP:port
  url += token;                                   // blynk token
  url += "/get/V";
  url += spin;                                    // pin to read
  http.begin(url);
  int httpCode = http.GET();
  delay(50);
  if (httpCode > 0) {
    payload = http.getString();                   // get response payload as String
    payload.remove(0, 2);
    payload.remove(payload.length() - 2);         // strip [""]
  }
  else payload = payload + ", httpCode: " + httpCode;

  http.end();
  delay(10);
  return payload;
}
/////////////////API BRIDGE FUNCTIONS/////////////////////////////////
///////////////////////////////NOTIFICATIONS - CHECK CENTRAL ALIVE////////////////////////////////////////////////// 
void checkNotifications(){                                                                          // sends messages to app in case of emergency, runs after checkHeater
  String apiResult      =  APIreadDevicePin(auth_CENTRAL, offline_PIN);                             // check whether another (or this) unit already detected the issue and initiated a timer. Note that this bool is reset to -1 when Central comes back online!! So the 3 options are -1/0/1+
  int oldCentralOffline = apiResult.toInt();

  centralOffline        = !deviceAlive(auth_CENTRAL);                                               // check if CENTRAL is actually offline
  //toTerminal(String("Api/oldCO/CO: ") + apiResult +"/"+ String(oldCentralOffline) +"/"+ String(centralOffline),0);
  if (!centralOffline && !oldCentralOffline) return;                                                // was and is online; everything is peaches
  if (centralOffline) {                                                                             // if offline
    oldCentralOffline++;
    toTerminal(String("oldCO: ") + String(oldCentralOffline) );
    APIwriteDevicePin(auth_CENTRAL, offline_PIN, String(oldCentralOffline));                        // store updated value as a 'Central' pin, so other THMs can find it. Note that CENTRAL will reset it to 0 when it gets back online!
    //@@@ if(oldCentralOffline > 20) Blynk.notify("Central is offline for 20m now!");                     // time the amount of minutes offline. (checkNotifications is called every minute), CENTRAL_BCK *should* take over in <16 min. So this line should never happen!!
  }
}
///////////////////////////////NOTIFICATIONS////////////////////////////////////////////////// 

void checkValve(int i){                           // i runs from 0 to 6
  if(stopValve[i]){                               // CLOSE VALVE IGNORE ALL PROCESSES
    Serial.println("SYSTEM OVERRIDE: CLOSE VALVE");
    pinMode(HEATER_RLY_PIN[i], OUTPUT);
    digitalWrite(HEATER_RLY_PIN[i], HIGH);
    Blynk.virtualWrite(rlyValveOn_PIN[i], HIGH);     // additional bool for THM to check whether the pin is actually set
  }
  else {
    pinMode(HEATER_RLY_PIN[i], OUTPUT);
    digitalWrite(HEATER_RLY_PIN[i], heatOn[i]?LOW:HIGH);  // activate = LOW, deactivate = HIGH
    Blynk.virtualWrite(rlyValveOn_PIN[i], heatOn[i]?LOW:HIGH); // additional bool for THM to check whether the pin is actually set
  }
}

void updateThermostat(int i){                     // used to send Maintenance Cycles Updates to Thermostats
  Serial.println(String("Updating VPIN_H / VPIN_M / STATE_H / STATE_M: ") + heatOn_PIN + " / " + maintOn_PIN + " / " +heatOn[i] + " / " + maintenanceOn_V[i]);

#if FLOOR == 3
  switch(i){
    case 3:   bridge_thm.virtualWrite(maintOn_PIN,  maintenanceOn_V[i]); break;
    case 4:   bridge_thm1.virtualWrite(maintOn_PIN, maintenanceOn_V[i]); break;
    case 5:   bridge_thm2.virtualWrite(maintOn_PIN, maintenanceOn_V[i]); break;
    case 6:   bridge_thm3.virtualWrite(maintOn_PIN, maintenanceOn_V[i]); break;
  }
#else
  bridge_thm.virtualWrite(maintOn_PIN, maintenanceOn_V[i]); // V14 send to thermostat, thermostat must NOT send updates during cycle!!
#endif
  checkValve(i);
}

void resetMaintenanceTimerValve(int i){
  // A Maintanance cycle is initiated every 12 hours if NOTHING has happened
  // Thus if SOMETHING happened; the 12 hour timer resets.
  timer.restartTimer(maintenanceTimer_V[i]);
  Serial.println(String("MaintenanceTimer for valve is reset to: ") + millis()/1000);
} 

void turnOffmaintenanceValve(int i){
  // This is called on a 1 time timer of TIME_STOP_MAINTENANCE seconds so the cycle is automatically stopped
  Serial.println("Turning off valve maintenance cycle, resetting maintenance timer");
  toTerminal( String("STOP VALVE ") + i + " MNT.");
  maintenanceOn_V[i] = false;
  heatOn[i] = !heatOn[i]; // revert to original heat settings
  resetMaintenanceTimerValve(i);
  updateThermostat(i);
}
/*
void turnOffmaintenanceValve_0(){ turnOffmaintenanceValve(0); }
void turnOffmaintenanceValve_1(){ turnOffmaintenanceValve(1); }
void turnOffmaintenanceValve_2(){ turnOffmaintenanceValve(2); }
void turnOffmaintenanceValve_3(){ turnOffmaintenanceValve(3); }
void turnOffmaintenanceValve_4(){ turnOffmaintenanceValve(4); }
void turnOffmaintenanceValve_5(){ turnOffmaintenanceValve(5); }
void turnOffmaintenanceValve_6(){ turnOffmaintenanceValve(6); }
*/
void maintenanceValve(int i){
  /*the purpose of this cycle is to change the valve position at least
  every 12 hours to prevent it from getting stuck.
  This cycle ONLY starts when the valve position has not changed for
  12 hours, its timer gets reset when the valve position changes*/
  maintenanceOn_V[i] = true;
  heatOn[i] = !heatOn[i]; // This swaps the Valve state so it moves from one position to the other
  toTerminal( String("START VALVE ") + i + " MNT.");
  Serial.println(String("Valve maintenance Cycle started. Valve is temporarily: ") + heatOn[i]?"OPENED":"CLOSED");
  // turn off the maintenance cycle after 5 minutes (routine is run 1 time)
  // after that the system resumes its old cycle
  Serial.println("Starting timer: 'turn off' valve maintenance cycle");
  tmpLambda = i; //move i  to global scope else you can't pass it onto the lambda function
  timer.setTimeout(TIME_STOP_MAINTENANCE, []() { turnOffmaintenanceValve(tmpLambda); }); //Generic Timer function method (Lambda function)
  /*switch(i){
    case 0: timer.setTimer(TIME_STOP_MAINTENANCE, turnOffmaintenanceValve_0, 1); break;
    case 1: timer.setTimer(TIME_STOP_MAINTENANCE, turnOffmaintenanceValve_1, 1); break;
    case 2: timer.setTimer(TIME_STOP_MAINTENANCE, turnOffmaintenanceValve_2, 1); break;
    case 3: timer.setTimer(TIME_STOP_MAINTENANCE, turnOffmaintenanceValve_3, 1); break;
    case 4: timer.setTimer(TIME_STOP_MAINTENANCE, turnOffmaintenanceValve_4, 1); break;
    case 5: timer.setTimer(TIME_STOP_MAINTENANCE, turnOffmaintenanceValve_5, 1); break;
    case 6: timer.setTimer(TIME_STOP_MAINTENANCE, turnOffmaintenanceValve_6, 1); break;
  }*/
  updateThermostat(i);
}


/*void maintenanceValve_0(){  maintenanceValve(0); }
void maintenanceValve_1(){  maintenanceValve(1); }
void maintenanceValve_2(){  maintenanceValve(2); }
void maintenanceValve_3(){  maintenanceValve(3); }
void maintenanceValve_4(){  maintenanceValve(4); }
void maintenanceValve_5(){  maintenanceValve(5); }
void maintenanceValve_6(){  maintenanceValve(6); }
*/

////////////////////// PUMP//////////////////// 
void checkPump(){
  //toTerminal(String("stp/mntP/mntV/hO: ") + stopPump + "/" + maintenanceOn_P + "/" + maintenanceOn_V[ROOM-1] + "/" + heatOn[ROOM-1]);
  if(stopPump){                                   // system override
      toTerminal("SYSTEM OVERRIDE: STOP PUMP");
      pinMode(HEATER_RLY_PIN_PUMP, OUTPUT);
      digitalWrite(HEATER_RLY_PIN_PUMP, HIGH);
      Blynk.virtualWrite(rlyPumpOn_PIN,HIGH); 
  }
  else
  {
    if(maintenanceOn_P)
    {                                             // run pump regardless of valve states
      Serial.println("START PUMP MAINTENANCE");
      pinMode(HEATER_RLY_PIN_PUMP, OUTPUT);
      digitalWrite(HEATER_RLY_PIN_PUMP, LOW);     // activate = LOW, deactivate = HIGH
    Blynk.virtualWrite(rlyPumpOn_PIN,LOW); 
    }
    else
    {
      if( heatOn[0] && !maintenanceOn_V[0] || 
          heatOn[1] && !maintenanceOn_V[1] ||
          heatOn[2] && !maintenanceOn_V[2] ||
          heatOn[3] && !maintenanceOn_V[3] ||
          heatOn[4] && !maintenanceOn_V[4] ||
          heatOn[5] && !maintenanceOn_V[5] ||
          heatOn[6] && !maintenanceOn_V[6] )      // run pump IF a valve is open aka heatOn = true
      {                 
        toTerminal("Heating request: START PUMP");
        pinMode(HEATER_RLY_PIN_PUMP, OUTPUT);
        digitalWrite(HEATER_RLY_PIN_PUMP, LOW);   // activate = LOW, deactivate = HIGH
        Blynk.virtualWrite(rlyPumpOn_PIN,LOW); 
    resetMaintenanceTimerPump();              // reset maint. timer for pump as its now running.
      }
      else
      {
        toTerminal("Heating request: STOP PUMP");
        pinMode(HEATER_RLY_PIN_PUMP, OUTPUT);
        digitalWrite(HEATER_RLY_PIN_PUMP, HIGH); 
        Blynk.virtualWrite(rlyPumpOn_PIN,HIGH); 
    }
    }
  }
}

void resetMaintenanceTimerPump(){
  // A Maintanance cycle is initiated every 12 hours if PUMP HAS NOT RUN
  // Thus if the PUMP did run: the 12 hour timer resets.
  Serial.println("MaintenanceTimer for pump is reset");
  timer.restartTimer(maintenanceTimer_P);
} 

void turnOffmaintenancePump(){
  // This is called on a 1 time timer of TIME_STOP_MAINTENANCE seconds so the cycle is automatically stopped
  Serial.println("STOP PUMP MAINTENANCE, resetting pump maintenance timer");
  toTerminal("STOP PUMP MNT.");
  maintenanceOn_P = false;
  checkPump();
  resetMaintenanceTimerPump();
}

void maintenancePump(){
  Serial.println("Pump maintenance Cycle started.");
  maintenanceOn_P = true;
  checkPump();
  // turn off the maintenance cycle after 5 minutes (routine is run 1 time)
  Serial.println("Starting 'turn off' pump cycle maintenance timer");
  toTerminal("START PUMP MNT.");
  timer.setTimer(TIME_STOP_MAINTENANCE, turnOffmaintenancePump, 1); 
}
////////////////////// PUMP//////////////////// 

void resetWidgetColors(){
  Serial.println("Resetting Colors");
  for(int vpin=0; vpin<64; vpin+=1) Blynk.setProperty(vpin, "color", BLYNK_GREEN);  // change colors of the widgets used in 'nerd' section
  Blynk.setProperty(TERMINAL_PIN, "color", idColor);                                // change Terminal color. idColor is set in header file
  Blynk.setProperty(OTA_update_PIN, "offBackColor", idColor);                       // change Update color. idColor is set in header file
  Blynk.setProperty(version_PIN, "color", idColor);                                 // change Update color. idColor is set in header file
}

void resetESP(){
  toTerminal(String("ESP RESET"));
  delay(3000);
  ESP.restart();
  delay(5000);                                  // not sure why, but it fails if you don't 
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

  
    Serial.println("Lost connection");
    if(Blynk.connect()) {
      Serial.println("Reconnected");
    }
    else {
      Serial.println("Not reconnected");
    }
  }
}
*/
void wifisignal(){                                // this function start with a button widget, when turned on it shows the current signal every 0.3s in the terminal
  int wifisignal = map(WiFi.RSSI(), -105, -40, 0, 100);
  toTerminal(String(wifisignal));
}

void blinkLED(){
  digitalWrite(2, LOW);                           // turn the LED on/off (HIGH is the voltage level)
  delay(100);
  digitalWrite(2, HIGH);                          // turn the LED on/off (HIGH is the voltage level)
  //Serial.print(" . ");
}

///////////////////////////////////////EVENT HANDLERS//////////////////////////////////////// 
BLYNK_WRITE_DEFAULT(){                            // Read updates coming in from THERMOSTAT (heatOn)
  int pin = request.pin;
  //toTerminal(String("incomming pin/val: ") + pin + " / " + param.asStr() );
  if( pin >= 1 && pin <= 7 ){                     // Heat update from room thermostats
    int i = pin - 1;                              // i runs from 0 to 6, rooms from 1 to 7
    bool oldheatOn = heatOn[i];
    heatOn[i] = (bool) param.asInt();             // store value that came via Bridge from THERMOSTAT
    if(oldheatOn != heatOn[i]){
      toTerminal(String("heatOn updated from Thermostat: ") + heatOn[i] + ". Resetting maintenance timer" );
      checkValve(i);
      checkPump(); 
      resetMaintenanceTimerValve(i);              // as the valve changes, reset the maintenance
    }    
  }
  if (pin == TERMINAL_PIN){                       // input from the terminal
    terminalCommands(param.asStr());              // this function is on the terminal_command tab
  }
  if (pin == OTA_update_PIN){
    HTTP_OTA = param.asInt(); // switch in app to check for updates
    toTerminal(String("OTA IN BLYNK_WRITE: ") + String(HTTP_OTA));
  
      if(HTTP_OTA > 1){                           // an OTA update just took place, check if version number updated correctly
        toTerminal(String(ESP_NAME) + " UPDATED: old version: " + String(HTTP_OTA) + " new version: " + FW_VERSION,0);
        HTTP_OTA = 0;                             // reset bool
        Blynk.virtualWrite(OTA_update_PIN, HTTP_OTA);
        Blynk.virtualWrite(version_PIN, FW_VERSION);
      }
  }
  if (pin == wifiSignal_PIN){                       // button to turn on signal wifi checker
    toTerminal(String("wifi signal request: ")+ param.asStr());
    if(param.asInt()){
      wifiID = timer.setInterval(300, wifisignal);  // check wifi signal every 300ms and send to serial port and terminal
    }else{
      timer.deleteTimer(wifiID);
    }
  }

}

BLYNK_CONNECTED() {
  initialize = true;
  Blynk.syncVirtual(OTA_update_PIN, HTTP_OTA, ROOM);
  bridge_thm.setAuthToken(auth_THM);
#if FLOOR == 3
 bridge_thm1.setAuthToken(auth_THM1);
 bridge_thm2.setAuthToken(auth_THM2);
 bridge_thm3.setAuthToken(auth_THM3);
 Blynk.syncVirtual(4,5,6);                        // relay 3 has 4 rooms connected: 3,4,5 and 6.
#endif
  rtc.begin();                                    // sync clock with server AFTER blynk has connected with said server. 
  updateTerminalLAbel();                          // immediately update terminal else it takes a minute
  initialize = false;
  toTerminal("======RECONNECTED=====");
}
///////////////////////////////////////EVENT HANDLERS//////////////////////////////////////// 

////////////////////////////////////////////////////////////////////////////// 
////////////////////////////// END FUNCTIONS//////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////////// 


void setup() {
  /////////////////////////////////////BLYNK////////////////////////////// 
  Serial.begin(9600);
  Serial.println("Booting");
  WiFi.setOutputPower(WIFIPOWER);
  Serial.println("Blynk Begin");
/*  Blynk.begin(auth, ssid, pass, server, port);
  Serial.println("Starting Blynk connection");
  Serial.println("Blynk connect");
  while (Blynk.connect() == false) {  }           // Wait until connected
  //resetWidgetColors();
  toTerminal("-------------");
  toTerminal(String("Blynk v") + BLYNK_VERSION + " started");
  toTerminal("-------------");
*/
  connectionState = CONNECT_TO_WIFI;
  /////////////////////////////////////BLYNK////////////////////////////// 
  
  /////////////////////////////////////OTA////////////////////////////// 
  initOTA(ESP_NAME);
  /////////////////////////////////////OTA////////////////////////////// 
  
  /////////////////////////////////////INIT PINS & FUNCTION TIMERS////////////////////////////// 
  initialize = true;
  Serial.println("Initiating all timed functions and Pins");
  // set it up depending on which floor this relay is
  switch(FLOOR){
  case 1:
    pinMode(HEATER_RLY_PIN[0], OUTPUT);           // The actual relay for valves
    digitalWrite(HEATER_RLY_PIN[0], HIGH);        // activate = LOW, deactivate = HIGH
    Blynk.virtualWrite(rlyValveOn_PIN[0], HIGH);     // additional bool for THM to check whether the pin is acutally set
    //maintenanceTimer_V[0]  = timer.setInterval(TIME_START_MAINTENANCE, maintenanceValve_0);  
  maintenanceTimer_V[0] = timer.setInterval(TIME_START_MAINTENANCE, []() { maintenanceValve(0); }); //Generic Timer function method (Lambda function) // id required to reset it when required
    updateThermostat(0);                          // also make sure that the thermostat has he latest settings and that if a maintenance cycle was running it is reset. 
  break;
  case 2:
    pinMode(HEATER_RLY_PIN[1], OUTPUT);
    digitalWrite(HEATER_RLY_PIN[1], HIGH);
    Blynk.virtualWrite(rlyValveOn_PIN[1], HIGH);     // additional bool for THM to check whether the pin is acutally set
    //maintenanceTimer_V[1]  = timer.setInterval(TIME_START_MAINTENANCE, maintenanceValve_1);
  maintenanceTimer_V[1]  = timer.setInterval(TIME_START_MAINTENANCE, []() { maintenanceValve(1); }); //Generic Timer function method (Lambda function) // id required to reset it when required
    updateThermostat(1);
  break;
  case 3:
    for(int i=2; i<=5; i++){
      pinMode(HEATER_RLY_PIN[i], OUTPUT);    
      digitalWrite(HEATER_RLY_PIN[i], HIGH); 
      Blynk.virtualWrite(rlyValveOn_PIN[i], HIGH);     // additional bool for THM to check whether the pin is acutally set
      updateThermostat(i);
      tmpLambda = i; //move i  to global scope else you can't pass it onto the lambda function
      maintenanceTimer_V[i] = timer.setInterval(TIME_START_MAINTENANCE, []() { maintenanceValve(tmpLambda); }); //Generic Timer function method (Lambda function) // id required to reset it when required
    }

//    maintenanceTimer_V[2]  = timer.setInterval(TIME_START_MAINTENANCE, maintenanceValve_2);
//    maintenanceTimer_V[3]  = timer.setInterval(TIME_START_MAINTENANCE, maintenanceValve_3);
//    maintenanceTimer_V[4]  = timer.setInterval(TIME_START_MAINTENANCE, maintenanceValve_4);
//    maintenanceTimer_V[5]  = timer.setInterval(TIME_START_MAINTENANCE, maintenanceValve_5);
  break;
  case 4:
    pinMode(HEATER_RLY_PIN[6], OUTPUT);    
    digitalWrite(HEATER_RLY_PIN[6], HIGH); 
    Blynk.virtualWrite(rlyValveOn_PIN[6], HIGH);     // additional bool for THM to check whether the pin is acutally set
//    maintenanceTimer_V[6]  = timer.setInterval(TIME_START_MAINTENANCE, maintenanceValve_6);
  maintenanceTimer_V[6] = timer.setInterval(TIME_START_MAINTENANCE, []() { maintenanceValve(6); }); //Generic Timer function method (Lambda function) // id required to reset it when required
    updateThermostat(6);
  break;
  }
  pinMode(HEATER_RLY_PIN_PUMP, OUTPUT);           // The actual relay for pump
  pinMode(2, OUTPUT);                             // LED
  digitalWrite(HEATER_RLY_PIN_PUMP, HIGH);        // activate = LOW, deactivate = HIGH
  Blynk.virtualWrite(rlyPumpOn_PIN,HIGH); 
  maintenanceTimer_P  = timer.setInterval(TIME_START_MAINTENANCE, maintenancePump);  // id required to reset it when required

  timer.setInterval(TIME_CHK_CONNECTION, ConnectionHandler);                         // connect to wifi and blynk
  timer.setInterval(TIME_RUN_FUNCTIONS, runFunctions);                               // every 1 minute: run time functions.
  runFunctions();                                                                    // make sure they also run on startup
  initialize = false;
  /////////////////////////////////////INIT PINS////////////////////////////// 
  //moved to blynk_connect
  /////////////////////////////////////ALIVE////////////////////////////// 
  Serial.println("Starting 'alive' routine...");
  timer.setInterval(4000, blinkLED);              // led on/off every second
  /////////////////////////////////////ALIVE////////////////////////////// 
}

void runFunctions(){
  updateTerminalLAbel();                          // every 1 minute: update the label on the terminal (time/wifi strength/version)
  //reconnectBlynk();                               // check every minute if still connected to server
#if FLOOR == 1                                    // ONLY floor 1 check for main online!!!
  Blynk.run();                                    // prevent blynk heartbeat failure
  checkNotifications();                           // do this also immediately on startup.
#endif
}

void loop() {
  timer.run(); 
  if(Blynk.connected()) { Blynk.run(); }
  ArduinoOTA.handle();                            // direct over the air update
  if(HTTP_OTA == 1){                              // server update (upload file to: http://192.168.1.93/fota/
    HTTP_OTA = FW_VERSION;                        // use H_OTA to store old version number in to compare with new one on reboot. This will also prevent eternal update loop
    Blynk.virtualWrite(OTA_update_PIN, HTTP_OTA);
    toTerminal("OTA before check: " + String(HTTP_OTA) + " FW Version: " + String(FW_VERSION));
    String returnTxt = checkForUpdates(ESP_NAME); // if successful the ESP reboots after this line, if not it will return a string with the error
    toTerminal(returnTxt);
    Blynk.syncVirtual(OTA_update_PIN);            // force blynk_write with updated (>1) http_ota value; on successful update this line should never execute!
  }
}

// ==================================================================DEVELOPMENT==
