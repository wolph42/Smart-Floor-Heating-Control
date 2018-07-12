#include <W42_Thermostat_Header.h>
const int FW_VERSION = 55;
// UPDATE PER ROOM: 1=LIVING, 2=STUDY, 3=BATH, 4=BED, 5=LAUNDRY, 6=ARTHUR, 7=CASPER, updated: 
// IP ADDRESSES:    100=LIVI, 101=STU, 102=BA, 103=B, 104=LAUND, 105=ARTH, 106=CASPE, updated: 
#define ROOM 5
// UPDATE PER ROOM 

#if ROOM == 1
  char ESP_NAME[]             = "ESP_THM_LIVING";
  char auth[]                 = auth_T_LIVING;    // assign correct token number, stored in W42 header
  char auth_RLY[]             = auth_R_FLOOR_1;   // auth_R_FLOOR_1, auth_R_FLOOR_2, auth_R_FLOOR_3, auth_R_FLOOR_4
  const float WIFIPOWER       = 0;                // range from 0 to +20.5dBm, where 0 is NOT off, but the lowest setting
  const String idColor        = CLIVING;
  float TCalibration          = -1.2;             // correction on displayed T
#endif
#if ROOM == 2
  char ESP_NAME[]             = "ESP_THM_STUDY";
  char auth[]                 = auth_T_STUDY; 
  char auth_RLY[]             = auth_R_FLOOR_2;
  const float WIFIPOWER       = 0;             
  const String idColor        = CSTUDY;
  float TCalibration          = -1.2;             // correction on displayed T
#endif
#if ROOM == 3
  char ESP_NAME[]             = "ESP_THM_BATH";
  char auth[]                 = auth_T_BATH; 
  char auth_RLY[]             = auth_R_FLOOR_3;
  const float WIFIPOWER       = 10;             
  const String idColor        = CBATH;
  float TCalibration          = -1.2;             // correction on displayed T
#endif
#if ROOM == 4
  char ESP_NAME[]             = "ESP_THM_BED";
  char auth[]                 = auth_T_BED; 
  char auth_RLY[]             = auth_R_FLOOR_3;
  const float WIFIPOWER       = 0;             
  const String idColor        = CBED;
  float TCalibration          = -1.7;             // correction on displayed T
#endif
#if ROOM == 5
  char ESP_NAME[]             = "ESP_THM_LAUNDRY";
  char auth[]                 = auth_T_LAUNDRY; 
  char auth_RLY[]             = auth_R_FLOOR_3;
  const float WIFIPOWER       = 0;             
  const String idColor        = CLAUNDRY;
  float TCalibration          = -1.2;             // correction on displayed T
#endif
#if ROOM == 6
  char ESP_NAME[]             = "ESP_THM_ARTHUR";
  char auth[]                 = auth_T_ARTHUR; 
  char auth_RLY[]             = auth_R_FLOOR_3;
  const float WIFIPOWER       = 0;             
  const String idColor        = CARTHUR;
  float TCalibration          = -1.2;             // correction on displayed T
#endif
#if ROOM == 7
  char ESP_NAME[]             = "ESP_THM_CASPER";
  char auth[]                 = auth_T_CASPER; 
  char auth_RLY[]             = auth_R_FLOOR_4;
  const float WIFIPOWER       = 0;             
  const String idColor        = CCASPER;
  float TCalibration          = -1.2;             // correction on displayed T
#endif

const byte TIME_CHECK_BUTTONS = 50; // check button state nearly constantly
const long TIME_HEATER_CHECK  = ((15 * 60) - 30) * 1000; // force heat check without heatOn every 14:30m
const byte MAX_VALUE_CHANGE   = 2;                // limit the BME sensor read out to a maximum deviation of 2C/%/Bar from the averaged T/H/Pcurrent
bool stopAllProcesses = false;                    // use to override ALL processes, command issues through terminal
///////////////////////////////////VARIABLES//////////////////////////////////////////////// 
// constructors
U8G2_SSD1306_64X48_ER_F_HW_I2C u8g2(U8G2_R0);     // OLED driver
BlynkTimer timer;                                 // timer
WidgetTerminal terminal(TERMINAL_PIN);            // initialize Terminal widget ; V123 defined in W42_header
WidgetRTC rtc;                                    // initialize Real-Time-Clock. Note that you MUST HAVE THE WIDGET in blynk to sync to!!!
CONNECTION_STATE connectionState;                 // this construction can be found in the W42 header file
BME280I2C bme;                                    // BME Sensor ; Default : forced mode, standby time = 1000 ms
                                                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
BME280::TempUnit tempUnit(BME280::TempUnit_Celsius); // set readout unit
BME280::PresUnit presUnit(BME280::PresUnit_hPa);

float targetT[3]        = {20,15,5};              // preset target temperature where 0=normal ; 1=night/away ; 2=vacation
byte targetTType        = 0;                      // used as indicator for targetT. where 0=normal ; 1=night/away ; 2=vacation
float currentT          = 20;                     // the average current temperature, measured over the passed half hour
float currentH          = 30;                     // byproduct of the sensor, so why not use it!
float currentP          = 1000;                   // byproduct of the sensor, so why not use it!
const byte T_ARRAY_SIZE = 5;                      // number of measurements over which the currentT is averaged. Each measurement is done every minute
float Tmeasurements[T_ARRAY_SIZE];                // measure T every minute stored in an array, currenT will be averaged over these values
float Hmeasurements[T_ARRAY_SIZE];                // measure Humidity  every minute stored in an array, currenT will be averaged over these values
float Pmeasurements[T_ARRAY_SIZE];                // measure Air Pressure in hPa every minute stored in an array, currenT will be averaged over these values
float offsetT           = 0.2;                    // to prevent the heater from constant flipping build in an offset for the switch when targetT is reached
byte maxVChange         = MAX_VALUE_CHANGE;       // maximum change in a sensor readout vs the current averaged T is 2C. This value is different during init, hence the constant
bool buttonUpState      = 0;                      // current state of the button
bool lastButtonUpState  = 0;                      // previous state of the button
bool buttonDwnState     = 0;                      // current state of the button
bool lastButtonDwnState = 0;                      // previous state of the button
bool heatOn             = false;                  // indicator received from central indicating whether the heat is on (valve is open, pump on)
bool maintenanceOn      = false;                  // indicator received from central indicating whether the maintenance cylce is on running
bool oledOn             = true;                   // is oled on or off, can be set through OLED_PIN
bool initializing       = 1;                      // check heaton with server during start up
byte notificationTimeOut                = 120;    // notification runs each minute. After it sets this number will decrease each minute, hence it acts as a notification time-out
const byte MAXSCHEDULE                  = 4;
unsigned long startseconds[MAXSCHEDULE] = {0,0,0,0}; // used for schedule
unsigned long stopseconds[MAXSCHEDULE]  = {0,0,0,0}; 
bool scheduleWeekDay[MAXSCHEDULE][7]    = { {false,false,false,false,false,false,false},
                                            {false,false,false,false,false,false,false} };
int HTTP_OTA = 0;                                 // when switch on app is hit, the ESP searches for and firmware.bin update. Also used to store version number in!!
uint8_t connectionCounter;

///////////////////////////////////VARIABLES//////////////////////////////////////////////// 

////////////////////////////////////////////////////////////////////////////// 
////////////////////////////// FUNCTIONS//////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////////// 

WidgetBridge bridge_central(channel0_PIN); 
WidgetBridge bridge_rly(channel1_PIN); 

void toTerminal(String input, bool showDate = true) {
  terminal.println(String(showDate?(getDateTime() + "-"):"") + input);
  terminal.flush();
  Serial.println(String("TO TERM.:") + input);
}

/////////////////////////////////REAL TIME CLOCK////////////////////////////// 
String getTime(){     return String((hour()<10)?"0":"") + hour() + ":"  + String((minute()<10)?"0":"") + minute() + ":" + String((second()<10)?"0":"") + second();  }
String getDate(){     return String(day()) + "/" + month();      } // + "/" + year()
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
  displaycurrenttimepluswifi = String(ESP_NAME) + " (v.:" + FW_VERSION + ")      Clock:  " + displayhour + ":" + displayminute + "      Signal:  " + wifisignal +" %";
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
/////////////////////// CONNECTION CONTROL ///////////////////////////


struct sensor{
  public:
  float temp, hum, pres;
  
  void readout(){
    temp = NAN;
    hum = NAN;
    pres = NAN;
    int everythingOk = 10;                         // prevent getting stuck in the while loop
    while(everythingOk && isnan(temp)){
      bme.read(pres, temp, hum, tempUnit, presUnit);      
      everythingOk--;
    }
    if (!everythingOk){
      Serial.print("Temp readout failure, setting T to last average");
      temp = currentT;
      hum = currentH;
      Blynk.virtualWrite(error_PIN, 200);         // send error update // LED brightness goes from 1 to 255
    }else{
      Blynk.virtualWrite(error_PIN, 0);           // send error update // LED brightness goes from 1 to 255
    }
    temp = min(max(temp, currentT-maxVChange), currentT+maxVChange); // make sure that the readout does not deviate more than 2 points
    hum = min(max(hum, hum-maxVChange), hum+maxVChange); 
    pres = min(max(pres, pres-maxVChange), pres+maxVChange); 
  }
} sensor;

void updateTArray(){
  // shift/copy the entire array-1 from address 0 (current position) to address 1
  // this way you keep averaging over the last 'T_ARRAY_SIZE' number of measurement, which prevents strong fluctuations
  // memmove(Tmeasurements, &Tmeasurements[1], T_ARRAY_SIZE-1); 
  // Tmeasurements[0]=singleTReadout(); // now place the new reading on address 0
  for(int i=T_ARRAY_SIZE-2; i>= 0; --i){
    Tmeasurements[i+1] = Tmeasurements[i];
    Hmeasurements[i+1] = Hmeasurements[i];
    Pmeasurements[i+1] = Pmeasurements[i];
  } 
  sensor.readout();
  Tmeasurements[0]=sensor.temp;
  Hmeasurements[0]=sensor.hum;
  Pmeasurements[0]=sensor.pres;
}

void updateCurrentT(){
  // average currentT over all the done measurements
  updateTArray(); // add a new readout to the T Array
  // then average currenT over the entire array (which is the last half hour)
  float tmpT = 0, tmpH = 0, tmpP = 0;
  for(int i = 0; i < T_ARRAY_SIZE; i++){
    tmpT += Tmeasurements[i];
    tmpH += Hmeasurements[i];
    tmpP += Pmeasurements[i];
  }
  currentT = tmpT / T_ARRAY_SIZE + TCalibration;
  currentH = tmpH / T_ARRAY_SIZE;
  currentP = tmpP / T_ARRAY_SIZE;

  Blynk.virtualWrite(currentT_PIN, floatToString(currentT,1));     // send updates to APP
  Blynk.virtualWrite(currentH_PIN, floatToString(currentH,0));
  Blynk.virtualWrite(currentP_PIN, floatToString(currentP,0));
  Serial.println(String("Temp: ") + floatToString(currentT,1) + " pin: " + currentT_PIN);
  updateOLED();
}

void updateTargetT(int tmpTType){                 // targetT[x] can be updated WITHOUT changing the targetTType
  // overall targetT can be changed as follows:
  // 1. buttons. Whenever these are used targetTtype is set to @home (for this THM unit only), THEN targetT is changed
  // 2. app. Whenever the app is used to change a targetT value, targetTtype is NOT changed.
  // The three types are 0=@home; 1=@night; 2=@vacation

  Blynk.virtualWrite(  setTargetT_PIN[tmpTType], floatToString(targetT[tmpTType],1)  ); // temp indicator in the device tile either @home or @night
  toTerminal(String("targetT ") + String(tmpTType) + " updated to: " + String(targetT[tmpTType],1) + ". currentTType: " + String(targetTType), 0 );
  
  if(tmpTType == targetTType) {
    Blynk.virtualWrite(  showTargetT_PIN, floatToString(targetT[targetTType],1)  ); // show the targettemp that is active
    checkHeater(); // Only checkHeater if the relevant targetT is changed. this will also update the OLED. 
  } else updateOLED();
}

void forceHeaterCheck(){
  initializing = 1;
  checkHeater();
  initializing = 0;
}

void checkHeater(){
  // toTerminal(String("Check heat cT/tT/Init/hO: ") + currentT + "/" + targetT[targetTType] + "/" + initializing + "/" + heatOn);
  if(maintenanceOn) return;                       // do not change state while running maintenance
  if ( (currentT < (targetT[targetTType]-offsetT)) && (!heatOn || initializing>0) && !stopAllProcesses ){ 
      heatOn = true;                              // turn heat ON when it was previous off. use 0.2 offset to prevent valve from constant flipping if its close to targetT
      toTerminal(String("HEAT ON cT/tT:") + String(currentT) + "/" + String(targetT[targetTType]-offsetT));                      
      Blynk.virtualWrite(heatOn_PIN, heatOn*200); // 200 = TO BLYNK APP: displayed by a led with a luminosity ranging from 0 to 256
      bridge_rly.virtualWrite(ROOM, heatOn);      // update relay
      bridge_central.virtualWrite(ROOM, heatOn);  // update CENTRAL CV
  }
  if (currentT > (targetT[targetTType]+offsetT) && (heatOn || initializing>0) || (stopAllProcesses && heatOn) ){
      heatOn = false;                             // turn heat ON when it was previous off.
      toTerminal(String("HEAT OFF cT/tT:") + String(currentT) + "/" + String(targetT[targetTType]+offsetT));
      Blynk.virtualWrite(heatOn_PIN, heatOn*200); // 200 = TO BLYNK APP: displayed by a led with a luminosity ranging from 0 to 256
      bridge_rly.virtualWrite(ROOM, heatOn);      // update relay
      bridge_central.virtualWrite(ROOM, heatOn);  // update CENTRAL CV
   }
    updateOLED();                                 // need to run EVERY check as its also used for the button update
}

void updateOLED(){
  if(!oledOn){
    u8g2.clearDisplay();
  } else {
    char output[100] = "";
    // full screen buffer method
    u8g2.clearBuffer();
    
    // draw the current temp LARGE
    // arduino does not support floats, so convert to two integers pre and post dot. 
    int preDot = (int)currentT;
    int postDot = int((currentT - int(currentT))*10+0.5);
    snprintf(output,  sizeof(output), "%d.%d", preDot,postDot);
    u8g2.setFont(u8g2_font_helvB24_te);
    u8g2.drawStr(1,25,output);
  
    // draw the target temp small
    preDot = (int)targetT[targetTType];
    postDot = int((targetT[targetTType] - int(targetT[targetTType]))*10+0.5);
    snprintf(output,  sizeof(output), "%d.%d", preDot,postDot);
    u8g2.setFont(u8g2_font_7x14_tr);
    u8g2.drawStr(1,47,output);  
  
    // U8G2 VERSION 2.22
    // u8g2.setFont(open_iconic_weather_2x);
    // u8g2.drawGlyph(16, 47, 0x0042);  // moon, for later use!
  
    // draw the heat and maintenance indicators
    // if M is not shown, show Night or Vacation mode else show the humidity
    
    if(maintenanceOn){
      // u8g2.setFont(open_iconic_embedded_2x);
      // u8g2.drawGlyph(0, 47, 0x0048);  // wrench */
      u8g2.setFont(u8g2_font_7x14_tr);
      u8g2.drawStr(38,47, "M");  
     
    }else if(targetTType){ // either Night or Vacation
      u8g2.setFont(u8g2_font_7x14_tr);
      u8g2.drawStr(38,47, (targetTType==1)?"N":"V");  
    }
    else{
      u8g2.setFont(u8g2_font_7x14_tr);
      snprintf(output,  sizeof(output), "%d", (int)currentH);
      u8g2.drawStr(38,47, output);  
    }
    
    u8g2.drawStr(56,47,(heatOn)? "H" : "");  
    // u8g2.setFont(open_iconic_thing_2x);
    // u8g2.drawGlyph(33, 47, 0x004E);  // flame
  
    // snprintf(output,  sizeof(output), "updating OLED MO/HO/CT/TT/CH: %d/%d/%d/%d/%d", maintenanceOn, heatOn, (int)currentT, (int)targetT[targetTType], (int)currentH);
    //Serial.println(output);
    // draw screen
    u8g2.sendBuffer();  
  }
}

void checkButtons(){
  // read the pushbutton input pin:
  buttonUpState = digitalRead(BUTTON_UP_PIN);
  buttonDwnState = digitalRead(BUTTON_DO_PIN);
  
  // IF OLED OFF: turn off for this unit if a button is pressed!
  if (buttonUpState != lastButtonUpState && !oledOn) {    // button is pressed AND OLED is OFF
    oledOn = true;
    Blynk.virtualWrite(OLED_PIN, 1);              // update screen contrast slider in app
    return;                                       // exit this routine --> don't change the temp THIS time
  }

  // IF NIGHT OR HOLIDAY: turn off for this unit if a button is pressed!
  if (buttonUpState != lastButtonUpState && targetTType) { // button is pressed AND targetTType is either night or holiday
    targetTType = 0;                              // set to @home (this unit only)
    checkHeater();                                // also updates OLED
    return;                                       // exit this routine --> don't change the temp THIS time
  }
  
  // UP - compare the buttonState to its previous state to prevent it from continuous change
  // IF the state has changed AND if the current state is HIGH
  // THEN the button went from off to on: increment the targetT[targetTType] with 0.5C
  if (buttonUpState != lastButtonUpState && buttonUpState == HIGH) {
    targetT[targetTType]+=0.5;
    updateTargetT(targetTType);
  }
  lastButtonUpState = buttonUpState;              // save the current state as the last state, for next time through the loop

  // DOWN - same method as UP
  if (buttonDwnState != lastButtonDwnState && buttonDwnState == HIGH) {
    targetT[targetTType]-=0.5;
    updateTargetT(targetTType);
  }
  lastButtonDwnState = buttonDwnState;
}

void resetESP(int param){
  if((bool) param){
    toTerminal(String("ESP RESET"));
  delay(3000);
  ESP.restart();
  delay(5000);                                  // not sure why, but it fails if you don't 
  }
}

void resetWidgetColors(){
  Serial.println("Resetting Colors");
  for(int vpin=0; vpin<64; vpin+=1) Blynk.setProperty(vpin, "color", BLYNK_GREEN);  // change colors of the widgets used in 'nerd' section
  Blynk.setProperty(offline_PIN, "color", RED);   // change LED color
  Blynk.setProperty(heatOn_PIN, "color", RED);    // change LED color or ERROR to red.
  Blynk.setProperty(maintOn_PIN, "color", RED);   // change LED color or ERROR to red.
  Blynk.setProperty(error_PIN, "color", RED);     // change LED color or ERROR to red.

  Blynk.setProperty(TERMINAL_PIN, "color", idColor);          // change Terminal color. idColor is set in header file
  Blynk.setProperty(OTA_update_PIN, "offBackColor", idColor); // change Update color. idColor is set in header file
  Blynk.setProperty(version_PIN, "color", idColor);           // change Update color. idColor is set in header file
}

/*void reconnectBlynk() {
  if (!Blynk.connected()) {
    Serial.println("Lost connection");

    if(Blynk.connect()) Serial.println("Reconnected");
    else                Serial.println("Not reconnected");
  }
}
*/
///////////////////////////////////////EVENT HANDLERS//////////////////////////////////////// 

void setSchedule(const BlynkParam& param, int nSchedule) {         
  TimeInputParam t(param);
  startseconds[nSchedule] =  t.getStartHour()*3600 + t.getStartMinute()*60;
  stopseconds[nSchedule]  =  t.getStopHour()*3600  + t.getStopMinute()*60;
  for(int day = 1; day <=7; day++) { scheduleWeekDay[nSchedule][day%7] = t.isWeekdaySelected(day); }   // Time library starts week on Sunday=1, Blynk on Monday=1, need to translate Blynk value to time value!! AND need to run from 0-6 instead of 1-7
}

void checkSchedule(){
  if(targetTType == 2) return;                    // ignore schedule during vacation.
  
  for (int nSchedule=0; nSchedule<MAXSCHEDULE; nSchedule++){
    if( scheduleWeekDay[nSchedule][weekday()-1] ){// Schedule is ACTIVE today
      unsigned long nowseconds =  hour()*3600 + minute()*60 + second();
  
      if(nowseconds >= startseconds[nSchedule] - 31 && nowseconds <= startseconds[nSchedule] + 31 ){  // 62s on 60s timer ensures 1 trigger command is sent
        // START ACTIVITY
        toTerminal("Away/Night started");
        Blynk.virtualWrite(targetTType_PIN, 2);   // (targetTType_PIN)(NOTE: 1,2,3 instead off 0,1,2)
        Blynk.syncVirtual(targetTType_PIN);         // sync will force a BLYNK_WRITE for that pin!
      }
  
      if(nowseconds >= stopseconds[nSchedule] - 31 && nowseconds <= stopseconds[nSchedule] + 31 ){    // 62s on 60s timer ensures 1 trigger command is sent
        // STOP ACTIVITY 
        toTerminal("Away/Night ended");
        Blynk.virtualWrite(targetTType_PIN, 1);
        Blynk.syncVirtual(targetTType_PIN);
      }// time
    }// week
  }// for
}

BLYNK_WRITE_DEFAULT(){                            // Read updates comming in from CENTRAL OR APP
  int pin = request.pin;
  bool oldOledOn;
  String color,label;
  int cVal;
  Serial.println(String("Incomming pin: ") + pin);
  switch (pin) {
    case heatOn_PIN:                              // should not be in use!!
      heatOn = (bool) param.asInt(); 
      checkHeater();                              // check if the heat needs be changed this will also update the OLED
    break;
    case maintOn_PIN:
      maintenanceOn = (bool) param.asInt();
      Serial.print(String("Maintenance updated by relay: ") + maintenanceOn);
      checkHeater();                              // check if the heat needs be changed this will also update the OLED
    break;
  
    case targetTType_PIN:                         // incomming targetT type for this THM unit (where home=1/night=2/vacation=3)
      targetTType = param.asInt()-1;              // types zijn 0,1,2 button results zijn 1,2,3
      color = setTargetTCOLOR[targetTType];
      label = String("Gewenste Temp. ") + String(  (targetTType==0)?"Dag":(targetTType==1)?"Nacht":"Vakantie"  );
      Blynk.setProperty(targetTType_PIN, "color", color);                           // 3 BUTTON WIDGET
      Blynk.setProperty(showTargetT_PIN, "color", color);                           // TARGET T as shown in app
      Blynk.setProperty(showTargetT_PIN, "label", label);                           // TARGET T LABEL as shown in app

      Blynk.setProperty(targetTMIN_PIN, "offBackColor", color);                     // TARGET T - button as shown in app
      Blynk.setProperty(targetTMIN_PIN, "onBackColor", color);                      // TARGET T - button as shown in app
      Blynk.setProperty(targetTPLUS_PIN, "offBackColor", color);                    // TARGET T + button as shown in app
      Blynk.setProperty(targetTPLUS_PIN, "onBackColor", color);                     // TARGET T + button as shown in app

      Blynk.virtualWrite(showTargetT_PIN, floatToString(targetT[targetTType],1)  ); // show the targettemp that is active

      toTerminal(  String("target T Type: ") + String(targetTType==0?"Home":targetTType==1?"Night":"Vacation")  );
      checkHeater(); 
    break;

    case homeTargetT_PIN:                         // virtual pin used to set the @home temp
      targetT[0] = param.asFloat();
      updateTargetT(0);
    break;
    case nightTargetT_PIN:                        // virtual pin used to set the @night temp
      targetT[1] = param.asFloat();
      updateTargetT(1);
    break;
    case vacationTargetT_PIN:                     // virtual pin used to set the @vacation temp
      targetT[2] = param.asFloat();
      updateTargetT(2);
    break;
   
    case targetTMIN_PIN:
      targetT[targetTType] -= 0.5;                // update the targetT of the type that is currently active!!
      updateTargetT(targetTType);
    break;
    case targetTPLUS_PIN:
      targetT[targetTType] += 0.5;
      updateTargetT(targetTType);
    break;

    case OLED_PIN:
      u8g2.setContrast(param.asInt());
      oldOledOn = oledOn;
      oledOn = (bool) param.asInt();              // if value reaches 0 turn screen off.
      cVal = min(200, 255 - param.asInt());       // makes the slider fade out when lowering the contrast to a minimum setting of 1 to prevent it from going white entirely
      color = String("#")+String(cVal, HEX)+String(cVal, HEX)+String(cVal, HEX);
      Blynk.setProperty(OLED_PIN, "color", color);// change colors of the OLED CONTRAST SLIDER widget
      if(oledOn != oldOledOn) updateOLED();
    break;
    case timer1_PIN:
      Serial.println("Schedule 1 update");
      setSchedule(param,0);
      break;
    case timer2_PIN:
      Serial.println("Schedule 2 update");
      setSchedule(param,1);
      break;
    case timer3_PIN:
      Serial.println("Schedule 3 update");
      setSchedule(param,3);
      break;
    case timer4_PIN:
      Serial.println("Schedule 4 update");
      setSchedule(param,4);
      break;
    case TERMINAL_PIN:  // input from the terminal
      if (String("reset") == param.asStr()) { resetESP(true);  }
      if (String("stop") == param.asStr()) {       // BEGIN: turn off CV no matter what.
        stopAllProcesses = true;  
        toTerminal("ALL PROCESSES HALTED, CV OFF",false); 
        checkHeater();
      } 
      if (String("start") == param.asStr()) {      // END: turn off CV no matter what.
        stopAllProcesses = false;  
        toTerminal("ALL PROCESSES CONTINUING...",false); 
        checkHeater(); 
      }
    break;
    case OTA_update_PIN:                          // switch in app to check for updates
      HTTP_OTA = param.asInt();                   // either '1' by app or old fw version number is stored by loop
      if(HTTP_OTA > 1){                           // an OTA update just took place, check if version number updated correctly
        toTerminal(String(ESP_NAME) + " UPDATED: old version: " + String(HTTP_OTA) + " new version: " + FW_VERSION,0);
        HTTP_OTA = 0;                             // reset bool
        Blynk.virtualWrite(OTA_update_PIN, HTTP_OTA);
        Blynk.virtualWrite(version_PIN, FW_VERSION);
      }
    break;
  }
}

BLYNK_CONNECTED() { 
  Blynk.syncVirtual(
    targetTType_PIN,
    setTargetT_PIN[0],
    setTargetT_PIN[1],
    setTargetT_PIN[2],
    heatOn_PIN, 
    OLED_PIN,
    OTA_update_PIN
  );                                               // to update the display. Do not use Blynk.syncAll(); this also to prevent being stuck on a maintenance cycle
  bridge_central.setAuthToken(auth_CENTRAL);
  bridge_rly.setAuthToken(auth_RLY);
  
  rtc.begin();                                     // sync clock with server AFTER blynk has connected with said server. 
  Blynk.virtualWrite(offline_PIN, 0);              // reset the 'offline' pin when back online.
  updateTerminalLAbel();                           // immediately update terminal else it takes a minute
  forceHeaterCheck();                              // does heater check regardless of heatOn;
  toTerminal("======RECONNECTED=====");
}
///////////////////////////////////////EVENT HANDLERS//////////////////////////////////////// 

////////////////////////////////////////////////////////////////////////////// 
////////////////////////////// END FUNCTIONS//////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////////// 


void setup() {
  /////////////////////////////////////OLED////////////////////////////// 
  u8g2.begin();  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvR08_te);
  u8g2.drawStr(1,25,"Connecting");
  u8g2.sendBuffer();   
  /////////////////////////////////////OLED////////////////////////////// 
  /////////////////////////////////////BLYNK////////////////////////////// 
  Serial.begin(9600);
  Serial.println("Booting");
  WiFi.setOutputPower(WIFIPOWER);
/*  Blynk.begin(auth, ssid, password, server, port);
  while (Blynk.connect() == false) {  }           // Wait until connected
  //resetWidgetColors();                            // reset the colors of the widgets
  toTerminal("-------------");
  toTerminal(String("Blynk v") + BLYNK_VERSION + " started");
  toTerminal("-------------");
*/
  connectionState = CONNECT_TO_WIFI;
  /////////////////////////////////////BLYNK////////////////////////////// 
  /////////////////////////////////////OTA////////////////////////////// 
  u8g2.clearBuffer();
  u8g2.drawStr(1,25,"Start OTA");
  u8g2.sendBuffer();  
  delay(500);
  initOTA(ESP_NAME);
  /////////////////////////////////////OTA////////////////////////////// 
  /////////////////////////////////////SENSOR////////////////////////////// 
  u8g2.clearBuffer();
  u8g2.drawStr(1,20,"Init");
  u8g2.drawStr(1,48,"Sensor");
  u8g2.sendBuffer();  
  Wire.begin();
  while(!bme.begin())  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }
  
  switch(bme.chipModel())  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
  delay(500);
  /////////////////////////////////////SENSOR//////////////////////////////  
  /////////////////////////////////////INIT PINS////////////////////////////// 
  pinMode(BUTTON_UP_PIN, INPUT);
  pinMode(BUTTON_DO_PIN, INPUT);
  /////////////////////////////////////INIT PINS////////////////////////////// 
   /////////////////////////////////////INIT (FILL) TEMPERATURE ARRAY////////////////////////////// 
  u8g2.clearBuffer();
  u8g2.drawStr(1,20,"Fill T");
  u8g2.drawStr(1,48,"Buffer");
  u8g2.sendBuffer(); 
  Serial.println("Filling Temperature Array");
  maxVChange = 100;                               // when starting up do not limit readouts to a range of +/- 2C
  char output[16] = "";
  for(int i = 0; i < T_ARRAY_SIZE; i++){
    snprintf(output,  sizeof(output), "%d ", T_ARRAY_SIZE - i);
    Serial.print("Cells left: ");
    Serial.println(output);
    updateTArray();
    delay(50);                                    // wait a short while for the next read
  }
  maxVChange  = MAX_VALUE_CHANGE;                 // set the default limit (2C) for readout deviations
  /////////////////////////////////////INIT TEMPERATURE ARRAY////////////////////////////// 
  /////////////////////////////////////INITIATE ALL FUNCTIONS TIMERS////////////////////////////// 
  u8g2.clearBuffer();
  u8g2.drawStr(1,20,"Start");
  u8g2.sendBuffer(); 
  Serial.println("Initiating all timed functions");
  timer.setInterval(TIME_CHECK_BUTTONS, checkButtons);                 // every 0.1 second: check if a button is pressed
  timer.setInterval(TIME_RUN_FUNCTIONS, runFunctions);                 // every 1 minute: run time functions.
  timer.setInterval(TIME_HEATER_CHECK, forceHeaterCheck);              // every 15 minutes: check heater status regardless of heatOn;
  timer.setInterval(TIME_CHK_CONNECTION, ConnectionHandler);           // connect to wifi and blynk
  runFunctions();                                                      // make sure they also run on startup
  /////////////////////////////////////INITIATE ALL FUNCTIONS TIMERS////////////////////////////// 
}

void runFunctions(){
  updateCurrentT();                               // every 1 minute: read T
  checkHeater();                                  // every 1 minute: check if the heater needs to be turned on or off
  Blynk.run();                                    // prevent blynk heartbeat failure
  //reconnectBlynk();                               // every 1 minute: device still connected to server
  updateTerminalLAbel();                          // every 1 minute: update the label on the terminal (time/wifi strength/version)
  checkSchedule();                                // every 1 minute: check timer schedule (night/day cycle)
  checkNotifications();                           // every 1 minute: check if stuff goes haywire;
}

void loop() {
  timer.run(); 
  if(Blynk.connected()) { Blynk.run(); }
  ArduinoOTA.handle();                             // direct over the air update
  if(HTTP_OTA == 1){                               // server update (upload file to: http:// 192.168.1.93/fota/
    HTTP_OTA = FW_VERSION;                         // use H_OTA to store old version number in to compare with new one on reboot. This will also prevent eternal update loop
    Blynk.virtualWrite(OTA_update_PIN, HTTP_OTA);
    toTerminal("OTA before check: " + String(HTTP_OTA) + " FW Version: " + String(FW_VERSION));
    String returnTxt = checkForUpdates(ESP_NAME);  // if successful the ESP reboots after this line, if not it will return a string with the error
    toTerminal(returnTxt);
    Blynk.syncVirtual(OTA_update_PIN);             // force blynk_write with updated (>1) http_ota value; on successful update this line should never execute!
  }
}

// ==================================================================DEVELOPMENT==
