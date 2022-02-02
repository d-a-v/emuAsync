
This fake library allows to compile and run natively on a computer *some*
esp8266/Arduino libraries or applications which are using the Async*
libraries for easier debugging.

This library is unfinished, in alpha stage, and should not be relied on.

- Requirements:

  - esp8266/Arduino repository

    `export ESP8266ARDUINO=path/to/esp8266/Arduino`

  - Arduino libraries directory

    `export ARDUINOLIB=path/to/libraryDirectory`

    `ARDUINOLIB` can point to the Arduino IDE library directory

  - [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets)

- Testing prerequisites:

  These directories must exist:
    - `${ARDUINOLIB}/emuAsync` (this library)
    - `${ARDUINOLIB}/arduinoWebSockets` (dependency)

- Testing With [ESPUI](https://github.com/s00500/ESPUI)

  These additional directories must exist:
  - `${ARDUINOLIB}/ESPUI`
  - `${ARDUINOLIB}/ArduinoJson` (dependency for ESPUI)
    
```
cd ${ARDUINOLIB}/emuAsync/tests
./ESPUI-gui
${ESP8266ARDUINO}/tests/host/bin/gui/gui
```
    