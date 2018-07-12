#define BLYNK_HEARTBEAT 60
#define BLYNK_NO_BUILTIN  			// Disable built-in analog & digital pin operations

#include <BlynkSimpleEsp8266.h>     // Blynk
#include <ArduinoOTA.h>             // OTA
#include <U8g2lib.h>                // OLED
#include <BME280I2C.h>              // Sensor
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>		// Http OTA update
#include <Wire.h>                   // I2C communication (Sensor)
#include <WidgetRTC.h>              // RTC Widget

//#include <ESP8266WiFi.h>            // 
//#include <ESP8266mDNS.h>            // 
//#include <ESP8266HTTPClient.h>      // 
//#include <WiFiUdp.h>                // 
//#include <TimeLib.h>			      // 

#define BLYNK_PRINT Serial			// redirect blynk messages like "Connecting to..." to Serial.print()
//#define BLYNK_HEARTBEAT 20			// defaults to 10s. 
//#define BLYNK_TIMEOUT_MS 5000UL		// defaults to 2000UL

//when going to production comment out blynk_print and uncomment the following lines:
//#define BLYNK_NO_BUILTIN   			// Disable built-in analog & digital pin operations
//#define BLYNK_NO_FLOAT     			// Disable float operations

// #define BLYNK_USE_128_VPINS		// no longer required

#define BLYNK_GREEN     "#23C48E"	// RR GG BB
#define BLYNK_BLUE      "#04C0F8"
#define BLYNK_YELLOW    "#ED9D00"
#define BLYNK_RED       "#D3435C"
#define BLYNK_DARK_BLUE "#5F7CD8"
#define BLYNK_ORANGE	"#FFA500"

#define RED 			"#FF0000"
#define WHITE 			"#FFFFFF"  
#define BLACK 			"#000000"  
#define YELLOW 			"#FFFF00" 
#define PURPLE 			"#9400D3" 
#define INDIGO 			"#4B0082" 
#define BLUE 			"#0000FF"  
#define GREEN 			"#00FF00"  
#define GRAY 			"#919191"  
#define GREY 			"#C0C0C0"  
#define ORANGE	 		"#FFA500"

#define CCENTRAL 		"#000000" // BLACK
#define CBCKCENTRAL 	"#919191" // GRAY
#define CLIVING 		"#23C48E" // BLYNK_GREEN
#define CSTUDY 			"#9400D3" // PURPLE
#define CBATH 			"#d68a00" // DARK_ORANGE
#define CBED 			"#d68a00" // DARK_ORANGE
#define CLAUNDRY 		"#d68a00" // DARK_ORANGE
#define CARTHUR 		"#d68a00" // DARK_ORANGE
#define CCASPER 		"#04C0F8" // BLYNK_BLUE

const char* ssid		= "";
const char* pass		= "";
const char* fwUrlBase	= "http://192.168.1.42/fota/";   // used for http OTAbool HTTP_OTA = false;
IPAddress server(192,168,1,42); 
const int port			= 8080;

#define auth_CENTRAL   "xxxxxxxxxxxxxxxx"

#define auth_T_LIVING  "xxxxxxxxxxxxxxxx"
#define auth_T_STUDY   "xxxxxxxxxxxxxxxx"
#define auth_T_BATH    "xxxxxxxxxxxxxxxx"
#define auth_T_BED     "xxxxxxxxxxxxxxxx"
#define auth_T_LAUNDRY "xxxxxxxxxxxxxxxx"
#define auth_T_ARTHUR  "xxxxxxxxxxxxxxxx"
#define auth_T_CASPER  "xxxxxxxxxxxxxxxx"

#define auth_R_FLOOR_1 "xxxxxxxxxxxxxxxx"
#define auth_R_FLOOR_2 "xxxxxxxxxxxxxxxx"
#define auth_R_FLOOR_3 "xxxxxxxxxxxxxxxx"
#define auth_R_FLOOR_4 "xxxxxxxxxxxxxxxx"

// pins used by therm and relay
#define channel0_PIN		V0
#define channel1_PIN		V1
#define channel2_PIN		V2
#define channel3_PIN		V3
#define channel4_PIN		V4

// pins used by thermometers
#define currentT_PIN		V10
#define currentH_PIN		V11
#define currentP_PIN		V12

#define heatOn_PIN			V13
#define maintOn_PIN			V14

#define error_PIN			V19 // sensor error
#define OLED_PIN			V20 // slider for oled contrast
#define timer1_PIN			V21	// timer input widget
#define timer2_PIN			V22	// timer input widget
#define timer3_PIN			V23	// timer input widget
#define timer4_PIN			V24	// timer input widget

// pins used by CENTRAL and THM
#define targetTType_PIN   	V31 // pin where the current targetTtype is stored (home, away, vacation)

const byte setTargetT_PIN[3]	= {V32, V33, V34}; // 0=@home; 1=@night; 2=@vacation
const String setTargetTCOLOR[3]	= {BLYNK_GREEN, BLYNK_DARK_BLUE, BLYNK_ORANGE}; // 0=@home; 1=@night; 2=@vacation
#define homeTargetT_PIN		V32 // only available at THM can't use the array in a case statement, hence the seperate definition as well. 
#define nightTargetT_PIN	V33 // only available at THM
#define vacationTargetT_PIN	V34 // THM and CENRTAL

#define showTargetT_PIN		V35 // only available at THM //value varies depending on targetTType (so either home/night/vacation
#define targetTMIN_PIN		V36 // only available at THM
#define targetTPLUS_PIN		V37 // only available at THM

// pins used by CENTRAL
#define CV_PIN				V10 // turn CV on/off
#define SHOW_TIME_PIN		V11 // show a clock

// pins used by ALL
#define TERMINAL_PIN		V30 // used to show the terminal outputs
#define version_BCK_PIN		V38 // shows current version of firmware. this pin is exclusively used by ESP_BCK_CENTRAL
#define OTA_BCK_update_PIN	V39 // on=check for update on server this pin is exclusively used by ESP_BCK_CENTRAL
#define OTA_update_PIN		V40 // on=check for update on server
#define offline_PIN			V41 // Every unit uses the pin to check offline status of that unit
#define version_PIN			V42 // shows current version of firmware
#define wifiSignal_PIN		V43 // button to turn on wifi signal checker (in terminal)
#define TERMINALBCK_PIN		V44 // used to show the BCK terminal outputs

#define rlyPumpOn_PIN		V50 // additional check to see if the pump rly is set. 
byte rlyValveOn_PIN[7] = {V51,V51,V51,V52,V53,V54,V51}; // additional check whether the Valve pin is actually set

// Note that there is no mem difference between const and define, however define is more bug prone!
const byte SCL_PIN = D5;
const byte SDA_PIN = D6;
const byte RES_PIN = D1;
const byte DC_PIN = D2; 
const byte BUTTON_UP_PIN = D7;   // the pin that the pushbutton is attached to YELLOW +
const byte BUTTON_DO_PIN = D8;   // the pin that the pushbutton is attached to WHITE -
// const int T_SENSOR_PIN = A0; // ADC !! (only this pin = (A0)
const int TIME_RUN_FUNCTIONS			= 1000 * 60; // Run all 1 minute functions
const int TIME_CHK_CONNECTION			= 100;       // run connection check every 0.1s Note that BLYNK.RUN is part of this!!!
String displaycurrenttimepluswifi;

typedef enum {
	CONNECT_TO_WIFI,
	AWAIT_WIFI_CONNECTION,
	CONNECT_TO_BLYNK,
	AWAIT_BLYNK_CONNECTION,
	MAINTAIN_CONNECTIONS,
	AWAIT_DISCONNECT
} CONNECTION_STATE;

///////////////////////////////////VARIABLES//////////////////////////////////////////////// 

///FUNCTIONS // Usually I would move this to a cpp file, but I get numerous errors so I gave up and placed them here. 
/////////////////STRING///////////////////////////////////////////////
String floatToString(float val, int decimalplaces){
  String value(val, decimalplaces);
  return value;
}

/*String getMAC(){//return mac address of unit
  uint8_t mac[6];
  char result[14];
  snprintf( result, sizeof( result ), "%02x%02x%02x%02x%02x%02x", mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ] );
  return String( result );
}*/
/////////////////STRING///////////////////////////////////////////////
/////////////////OTA////////////////////////////////////////////////// 
void initOTA(char ESP_NAME[]){
  Serial.print("EPS Name: ");
  Serial.println(ESP_NAME);
  ArduinoOTA.setHostname(ESP_NAME);
  // ArduinoOTA.setpass("admin");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
/////////////////OTA////////////////////////////////////////////////// 
//////////////////////// FOTA//////////////////////////////////////// 
String checkForUpdates(char ESP_NAME[]) {
  String ESP = String(ESP_NAME);
  String fwURL = String( fwUrlBase ) + ESP + ".bin";          // e.g. ESP_CENTRAL.bin
  String oneLine = "";                                         // used for returntxt and serial line
  String returnTxt = "HTTP FIRMWARE UPDATE FOR: " + ESP + "\n"; // in case of error return string which then can be ported to terminal
  
  Serial.println( returnTxt );
  delay(500);                                                 // make sure everything is set. 
  t_httpUpdate_return ret = ESPhttpUpdate.update( fwURL );    // IF successful, ESP resets after this line
  //t_httpUpdate_return ret = ESPhttpUpdate.update("https://username.github.io/firmware/blink.bin", "", "fingerprint_goes_here");
  switch(ret) {                                               // In case of failure....
	case HTTP_UPDATE_FAILED:
	  oneLine = "HTTP_UPDATE_FAILED Error "+ String(ESPhttpUpdate.getLastError()) + ": " + ESPhttpUpdate.getLastErrorString().c_str() + "\n";
	  Serial.print(oneLine);
	  returnTxt += oneLine;
	  break;
	case HTTP_UPDATE_NO_UPDATES:
	  oneLine = "HTTP_UPDATE_NO_UPDATES\n";
	  Serial.print(oneLine);
	  returnTxt += oneLine;
	  break;
  }
  return returnTxt;
}
//////////////////////// FOTA//////////////////////////////////////// 
