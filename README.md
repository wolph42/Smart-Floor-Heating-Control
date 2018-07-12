# Smart-Floor-Heating-Control
This project entails an interactive system with which you can control multi-zone floor heating. It covers all aspects required like: Thermostat (per zone), Operating Relays (per distribution unit) and Central Relay (operating the Central Heater unit). The entire system makes use of Blynk.

The end result is: 
- a thermostat per 'zone' (usually room)
- physical buttons on the thermostats to influence the target temperature
- an interactive app (powered by Blynk)
- relays to operate the pump and valves at the distribution unit
- relay to operate the Central Heating unit
- clever maintenance cycles for ALL valves and pumps to prevent them from breaking down
- a Raspberri Pi server (Java, Blynk, Samba VNC and Apache web)

To do:
- a failsave system for when one of the units fails to reconnect to the router
- an additional temperature sensor per zone for feedback purposes per zone to spot potential hardware failure (like Valves that are stuck or pumps that fail).

The project was based on the following situation: 4 floors with 1 zone on floor 1,2 and 4 and 4 zones on floor 3. 

The word document: **"Multi level floor heating system Vx.docx"** contains the entire build story of the system, including tutorials and full  designs of the system. 

It also contains a full description on how to change the code to make it applicable for 'your' situation. 


IF you want to do a quick build: each of the HEATING_xxx directories contain the .ino files for the specified unit (e.g. Thermostat) which can be compiled in IDE. To compile it you do need to add the W42_Header_Files to your IDE *libraries* directory.
