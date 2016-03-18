
EVELAB ARDUINO README
**********************************

This is an Arduino library that allows you to control the EveLab (http://eveone.org).

It can be used to receive commands remotely from the browser, process those commands, and send the response over websockets.
 When receiving commands remotely the socket can either be used raw or as a websocket from a browser.

It uses the SHA1 and Base64 libraries from https://github.com/ejeklint/ArduinoWebsocketServer in order to provide the Websocket functionality
and the https://github.com/adafruit/Adafruit_MPR121 to control the capacitive touch.

This code is based the Mirobot Arduino library (http://www.mirobot.io)
