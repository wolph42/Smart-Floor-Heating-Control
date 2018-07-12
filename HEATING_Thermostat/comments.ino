/* Learnings:
 *  the pin numbers on the board do NOT CORRESPOND with the Arduino pins...sigh...
 *  do not use pin 1 (TX) as this garbles the serial!!
 *  Pin 2 (D4) is the LED onboard of the Wemos
 *  if you can't find the OTA under Tools>Port then check whether windows bonjour service is running
 *  
 *  SETUP FOR NOW:
 *  THERMOSTAT does the reading
 *  RELAY does the maintenance checks
 *  CENTRAL does the communication
 *  
 *  REFERENCES
 *  
 *  A0   = 0;  //analogue in/out
 *  D0   = 16;
 *  D1   = 5;  //I2C SCL
 *  D2   = 4;  //I2C SDA 
 *  D3   = 0;  //not analogue
 *  D4   = 2;  //LED
 *  D5   = 14; //SPI CLK
 *  D6   = 12; //SPI MISO
 *  D7   = 13; //SPI MOSI
 *  D8   = 15; //SPI CS
 *  RX   = 3;
 *  TX   = 1;
 *  
 *  
 */
//define means an effective search and replace with on compilation!
