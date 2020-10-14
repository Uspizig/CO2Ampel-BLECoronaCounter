# CO2Ampel-BLECoronaCounter
Measures CO2 with a Sensirion SGP30 Sensor and counts BLE Device with a UUID for German Corona Warn Apps
on a ESP32 Mikrocontroller programmed with Arduino
This code was programmed for a M5 Atom and ESP32. It might be adapted to your needs.

Germany suffers from Covid-19. To prevent spreading of the Virus further it is advised to follow 4 rules
AHAL - 
Abstand - Keep your Disctance
Hygiene - Wash your hands
Alltagsmaske - Wear a Mask
Lüften - Open you windows periodically and get some fresh air in.

Additionally there is an App availible in Germany that warns you if you got in contact with a positiv tested person.

This Code checks the Air Quality of a Room with a Sensiroin SGP30
It scans every 90 sec for BLE devices One BLE Scan takes 30 seconds.
Afterwards it lists the found devices and the amount of German Corona-Warn app users.

