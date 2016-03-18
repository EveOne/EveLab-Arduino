\_   _____/__  __ ____ |    |   _____ \_ |__   \_   ___ \  ____   __| _/____  
 |    __)_\  \/ // __ \|    |   \__  \ | __ \  /    \  \/ /  _ \ / __ |/ __ \
 |        \\   /\  ___/|    |___ / __ \| \_\ \ \     \___(  <_> ) /_/ \  ___/
/_______  / \_/  \___  >_______ (____  /___  /  \______  /\____/\____ |\___  >
        \/           \/        \/    \/    \/          \/            \/    \/ 

This is an Arduino library that allows you to control EveLab (http://eveone.org).

It can be used to receive commands remotely from the browser, process those commands, and send the response over websockets.
 When receiving commands remotely the socket can either be used raw or as a websocket from a browser.

It uses the SHA1 and Base64 libraries from https://github.com/ejeklint/ArduinoWebsocketServer in order to provide the Websocket functionality
and this code is based on the code of the Mirobot (http://www.mirobot.io)
