@ECHO OFF
REM This is an example batch file, you will need one of these for each floor and it needs to be adjusted to 'your' situation. 
REM See word doc for more info.

del x:\fota\ESP_RLY_FLOOR_1.bin
rename Heating_Relay.ino.d1_mini.bin ESP_RLY_FLOOR_1.bin
move ESP_RLY_FLOOR_1.bin x:\fota
