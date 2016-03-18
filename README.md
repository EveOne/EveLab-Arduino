 ____                    __                __          __  __      __        ____                __            
/\  _`\                 /\ \              /\ \        /\ \/\ \  __/\ \__    /\  _`\             /\ \           
\ \ \L\_\  __  __     __\ \ \         __  \ \ \____   \ \ \/'/'/\_\ \ ,_\   \ \ \/\_\    ___    \_\ \     __   
\ \  _\L /\ \/\ \  /'__`\ \ \  __  /'__`\ \ \ '__`\   \ \ , < \/\ \ \ \/    \ \ \/_/_  / __`\  /'_` \  /'__`\
 \ \ \L\ \ \ \_/ |/\  __/\ \ \L\ \/\ \L\.\_\ \ \L\ \   \ \ \\`\\ \ \ \ \_    \ \ \L\ \/\ \L\ \/\ \L\ \/\  __/
  \ \____/\ \___/ \ \____\\ \____/\ \__/.\_\\ \_,__/    \ \_\ \_\ \_\ \__\    \ \____/\ \____/\ \___,_\ \____\
   \/___/  \/__/   \/____/ \/___/  \/__/\/_/ \/___/      \/_/\/_/\/_/\/__/     \/___/  \/___/  \/__,_ /\/____/

This is an Arduino library that allows you to control EveLab (http://eveone.org).

It can be used to receive commands remotely from the browser, process those commands, and send the response over websockets.
 When receiving commands remotely the socket can either be used raw or as a websocket from a browser.

It uses the SHA1 and Base64 libraries from https://github.com/ejeklint/ArduinoWebsocketServer in order to provide the Websocket functionality
and this code is based on the code of the Mirobot (http://www.mirobot.io)
