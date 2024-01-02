# NodeMCU 
An arduino sketch that connects a
- MQ-135 air quality sensor
- DHT-11 temperature/humidity sensor
- KY-037 microphone

  To an MQTT broker

# Installation
Load up your Arduino IDE

Install ESP8266 board via board manager

Select the correct board (NodeMCU 1.0 (ESP-12E Module))

Install the following libraries via Library manager:
  - MQ135.h
  - DHT.h
  - ESP8266 library
  - ESPAsyncTCP
  - [MQTT Async Library](https://github.com/marvinroger/async-mqtt-client) - add to your Arduino library folder

Useful tools for flashing NodeMCU

- [NodeMCU Firmware](https://nodemcu-build.com/trigger-build.php) - Use default settings and yes, give up that email address
- [NodeMCU Flasher](https://github.com/nodemcu/nodemcu-flasher)


Hit compile
