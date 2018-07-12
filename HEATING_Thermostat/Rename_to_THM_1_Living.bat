@ECHO OFF
REM This is an example batch file, you will need one of these for each unit and it needs to be adjusted to 'your' situation. 
REM See word doc for more info.

del x:\fota\ESP_THM_LIVING.bin
rename Heating_Thermostat.ino.d1_mini.bin ESP_THM_LIVING.bin
move ESP_THM_LIVING.bin x:\fota
