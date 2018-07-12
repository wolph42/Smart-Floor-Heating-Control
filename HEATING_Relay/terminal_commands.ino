void terminalCommands(String param){
    if (String("?") == param) {          //END: turn off pump and close valves no matter what.
    String output = String("\nreset: reset unit\n")                   +
    "stop: stop ALL processes, turn valves & pump off\n"              +
    "start: start ALL processes (check valves & pump\n"               +
    "\n"                                                              +
    "stop pump: stop pump, remain off until 'start pump'\n"           +
    "start pump: start pump process again (check pump)\n"             +
    "\n"                                                              +
    "stop valve: close ALL valves, remain closed until 'start'\n"     +
    "start valve: start ALL valve processes again\n"                  +
    "\n"                                                              +
    "stp valve n: close valve n, remain closed until 'start'\n"       +
    "stt valve n: start normal processing for valve n\n"              +
    "\n"                                                              +
    "valve on n: turn valve n on\n"                                   +
    "valve of n: turn valve n off\n"                                  +
    "\n"                                                              +
    "n = 1 (LIVING); 2 (STUDY) ; 3 (BATH) ; 4 (BED) ; 5 (LAUNDRY) ;"  +
    "6 (ARTHUR) ; 7 (CASPER)";
  toTerminal(output, false); 
  }

  if (String("reset") == param) {
    resetESP();  
  }

  if (String("stop") == param) {
    toTerminal("ALL PROCESSES HALTED, CV OFF",false); 
    stopPump = true;
    for(int i=0;i<7;i++){
      stopValve[i] = true;
      checkValve(i);
    }
    checkPump(); 
  }
  
  if (String("start") == param) {
    toTerminal("ALL PROCESSES CONTINUING...",false); 
    stopPump = false;
    for(int i=0;i<7;i++){
      stopValve[i] = false;
      checkValve(i);
    }
    checkPump(); 
  }
  
  if (String("stop pump") == param) {
    toTerminal("PUMP TURNED OFF, PROCES HALTED",false); 
    stopPump = true;
    checkPump(); 
  }
  
  if (String("start pump") == param) {
    toTerminal("START PUMP PROCES",false); 
    stopPump = false;
    checkPump(); 
  }
  
  if (String("stop valve") == param) {
    toTerminal("STOP ALL VALVES PROCESSES, VALVE = OFF",false); 
    for(int i=0;i<7;i++){
      stopValve[i] = true;
      checkValve(i);
    }
  }
  
  if (String("start valve") == param) {
    toTerminal("START VALVES PROCES",false); 
    for(int i=0;i<7;i++){
      stopValve[i] = false;
      checkValve(i);
    }
  }
  
  if(param.length() == 10){
    String id = param.substring(0,8);
    String iStr = param.substring(9);
    int i = iStr.toInt();
    Serial.println(String("paramOnOff: ") + param + " / id: " + id + " / i: " + i);
    if(id == "valve of"){
      digitalWrite(HEATER_RLY_PIN[i], HIGH);
      toTerminal(String("VALVE ") + i + " OFF",false); 
    }
    if(id == "valve on"){
      digitalWrite(HEATER_RLY_PIN[i], LOW);
      toTerminal(String("VALVE ") + i + " ON",false); 
    }
  }
  
  if(param.length() == 11){
    String id = param.substring(0,9);
    String iStr = param.substring(10);
    int i = iStr.toInt();
    Serial.println(String("paramSttStp: ") + param + " / id: " + id + " / i: " + i);
    if(id == "stp valve"){
      toTerminal(String("STOP VALVE ") + i + " PROCES",false); 
      stopValve[i] = true;
      checkValve(i);
    }
    if(id == "stt valve"){
      toTerminal(String("START VALVE ") + i + " PROCES",false); 
      stopValve[i] = false;
      checkValve(i);
    }
  }
}

