@ECHO OFF
REM This is an example batch file, you will need to create another one for the BACKUP unit and it needs to be adjusted to 'your' situation. 
REM See word doc for more info.

del x:\fota\ESP_CENTRAL.bin
rename HEATING_Central.ino.d1_mini.bin ESP_CENTRAL.bin
move ESP_CENTRAL.bin x:\fota
