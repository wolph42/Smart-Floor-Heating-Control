///////////////////////////////NOTIFICATIONS//////////////////////////////////////////////////
bool recentMessage[9] = {false,false,false,false,false,false,false,false,false}; //warning sent on T>30, 40 and 50 ; H>70, 80 and 90 (0,1,2, 3,4,5)

void sentMessage(String text, int type, int setTo){
  if( setTo==notificationTimeOut && recentMessage[type]<1  ){         //bad stuff message is sent for the first time
    Blynk.notify(text);   
    recentMessage[type]+=setTo;
  }
  if( setTo==-1){                                                     //bad stuff declining message
    if(recentMessage[type]==notificationTimeOut) Blynk.notify(text);  //first time
    recentMessage[type]+=setTo;                                       //starting timeout, nothing will happen until timeout value has reached 0
  }
}

void checkNotifications(){                         //sends messages to app in case of emergency, runs after checkHeater
  //==BAD STUFF RISING==
  if (!recentMessage[0] && currentT < 4){  sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "is now lower than 4C",  0, notificationTimeOut); }
  if (!recentMessage[1] && currentT > 30){ sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "is now higher than 30C",  1, notificationTimeOut); }
  if (!recentMessage[2] && currentT > 40){ sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "is now higher than 40C",  2, notificationTimeOut); }
  if (!recentMessage[3] && currentT > 50){ sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "is now higher than 50C",  3, notificationTimeOut); }
  if (!recentMessage[4] && currentH > 70){ sentMessage(String("The humidity in room: ")     + String(ESP_NAME) + "is now higher than 70%",  4, notificationTimeOut); }
  if (!recentMessage[5] && currentH > 80){ sentMessage(String("The humidity in room: ")     + String(ESP_NAME) + "is now higher than 80%",  5, notificationTimeOut); }
  if (!recentMessage[6] && currentH > 90){ sentMessage(String("The humidity in room: ")     + String(ESP_NAME) + "is now higher than 90%",  6, notificationTimeOut); }

  //==UNCOMFORTABLE
  if(ROOM==1 ||ROOM==4 ||ROOM==6 ||ROOM==7){ //living, bed, arthur, casper
    if (!recentMessage[7] && currentT < 14 && targetTType!=2){ sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "is now lower than 15C",  7, notificationTimeOut); }
    if (!recentMessage[8] && currentH < 25 && targetTType==0){ sentMessage(String("The humidity in room: ")  + String(ESP_NAME) + "is now lower than 30%",  8, notificationTimeOut); }
  }
  
  //==BAD STUFF DECLINING==
  if (recentMessage[0]>0 && currentT > 4){  sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "is now higher than 4C",  0, -1); }
  if (recentMessage[1]>0 && currentT < 25){ sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "has lowered below 25C",  1, -1); }
  if (recentMessage[2]>0 && currentT < 35){ sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "has lowered below 35C",  2, -1); }
  if (recentMessage[3]>0 && currentT < 45){ sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "has lowered below 45C",  3, -1); }
  if (recentMessage[4]>0 && currentH < 60){ sentMessage(String("The humidity in room: ")     + String(ESP_NAME) + "has lowered below 60%",  4, -1); }
  if (recentMessage[5]>0 && currentH < 70){ sentMessage(String("The humidity in room: ")     + String(ESP_NAME) + "has lowered below 70%",  5, -1); }
  if (recentMessage[6]>0 && currentH < 80){ sentMessage(String("The humidity in room: ")     + String(ESP_NAME) + "has lowered below 80%",  6, -1); }
  
  //==COMFORTABLE
  if (recentMessage[7]>0 && currentT > 15){ sentMessage(String("The temperature in room: ")  + String(ESP_NAME) + "is now higher than 15C",  7, -1); }
  if (recentMessage[8]>0 && currentH > 30){ sentMessage(String("The humidity in room: ")  + String(ESP_NAME) + "is now higher than 30%",  8, -1); }
}
///////////////////////////////NOTIFICATIONS//////////////////////////////////////////////////

