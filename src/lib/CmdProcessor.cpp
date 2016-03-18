#include "CmdProcessor.h"
#include "sha1.h"
#include "Base64.h"

CmdProcessor::CmdProcessor(){
  socketMode = RAW;
  in_process = false;
  at_cmd_state = CLOSED;
  strcpy(webSocketKey, "------------------------258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
  resetTimeout = 0;
}

void CmdProcessor::setup(Stream &s, Evelab &m){
  httpState = WAITING;
  _s = &s;
  _m = &m;
  resetCheck();
}

// This function lets the user press the reset button on the Arduino 3 times to turn the WiFi module access point back on
void CmdProcessor::resetCheck(){
  if(MCUSR & _BV(EXTRF)){
    // Reset was caused by the reset button, so let's increment the eeprom counter
    byte counter = EEPROM.read(4) + 1;
    if(counter >= 3){
      // The reset button has been pressed three times in quick succession so let's reset the WiFi module
      startAtCmdReset();
      counter = 0;
    }
    EEPROM.write(4, counter);
    // Set the timeout for 5 seconds
    resetTimeout = millis() + 5000;
  }else{
    // Reset was caused by the power switch, so let's reset the eeprom counter
    if(EEPROM.read(4) != 0){
      EEPROM.write(4, 0);
    }
  }
  MCUSR = 0;
}

void CmdProcessor::process(){
  // check if the previous command is ready
  if(in_process && _m->ready() && at_cmd_state == CLOSED){
    in_process = false;
    sendResponse("complete", "", *current_id);
  }
  // check the reset timeout to make sure the reset button is pressed within 5 seconds
  if(resetTimeout > 0 && resetTimeout < millis()){
    resetTimeout = 0;
    EEPROM.write(4, 0);
  }
  // process incoming data
  if (_s->available() > 0){
    last_char = millis();
    char incomingByte = _s->read();
    if(at_cmd_state != CLOSED){
      handleAtCmds(incomingByte);
    }else if((incomingByte == '\r' || incomingByte == '\n') && processLine()){
      // It's been successfully processed as a line
      input_buffer_pos = 0;
    }else{
      // Not a line to process so store for processing as a websocket frame
      input_buffer[input_buffer_pos++] = incomingByte;
      if(processWSFrame()){
        input_buffer_pos = 0;
      }
      if(input_buffer_pos == INPUT_BUFFER_LENGTH){
        // We've filled the buffer, send an error message
        sendResponse("error", "Message too long", (char&)"");
        input_buffer_pos = 0;
      }
    }
  }else{
    //reset the input buffer if nothing is received for 1/2 second to avoid things getting messed up
    if(millis() - last_char >= 500){
      input_buffer_pos = 0;
    }
  }
}

void CmdProcessor::startAtCmdReset(){
  _s->print("+++");
  at_cmd_state = OPENING;
}

void CmdProcessor::handleAtCmds(char incomingByte){
  // process any incoming data as an AT command
  if(at_cmd_state == OPENING){
    // we're doing the starting handshake
    if(incomingByte == 'a'){
      // respond to the incoming 'a' with our own 'a' after sending the +++
      _s->print("a");
    }else{
      // look for the '+ok' response to say we're in AT mode
      input_buffer[input_buffer_pos++] = incomingByte;
      if(input_buffer_pos >= 3){
        if(!strncmp(input_buffer, "+ok", 3)){
          at_cmd_state = READY;
          input_buffer_pos = 0;
        }else if(!strncmp(input_buffer, "+++", 3)){
          // It's already open and has just echoed our +++
          // send a carriage return to flush
          _s->print("\r\n");
          at_cmd_state = FLUSHING;
        }
      }
    }
  }else if(at_cmd_state == READY){
    // send the AT message we buffered earlier
    _s->println("AT+WMODE=APSTA");
    at_cmd_state = RECEIVING;
  }else if(at_cmd_state == RECEIVING){
    if((incomingByte == '\r' || incomingByte == '\n') && input_buffer_pos > 0){
      // we've received a line
      if(!strncmp(input_buffer, "+ok", 3)){
        _s->println("AT+Z");
        at_cmd_state = CLOSED;
      }
      input_buffer_pos = 0;
    }else{
      input_buffer[input_buffer_pos++] = incomingByte;
    }
  }else if(at_cmd_state == FLUSHING){
    if((incomingByte == '\r' || incomingByte == '\n')){
      at_cmd_state = READY;
    }
  }
}

boolean CmdProcessor::processLine(){
  //do a simple check to see if it looks like json
  if(input_buffer_pos > 0 && input_buffer[0] == '{' && input_buffer[input_buffer_pos - 1] == '}'){
    socketMode = RAW;
    processJSON();
    return true;
  }else if(processHeaders()){
    return true;
  }

  if(httpState == WEBSOCKET_READY){
    return false;
  }else{
    return true;
  }
}

boolean CmdProcessor::processHeaders(){
  if(input_buffer_pos >= 23 && !strncmp(input_buffer, "GET /websocket HTTP/1.1", 23)){
    // The client is attempting to open a websocket
    httpState = WEBSOCKET_INIT;
    return true;
  }else if(input_buffer_pos >= 19 && !strncmp(input_buffer, "GET /http?", 10)){
    // The client is attempting to use HTTP
    httpState = HTTP_INIT;
    return true;
  }else if(input_buffer_pos == 43 && !strncmp(input_buffer, "Sec-WebSocket-Key: ", 19)){
    // Grab the security key for later
    strncpy(webSocketKey, &input_buffer[19], 24);
    httpState = WEBSOCKET_RESPOND;
    newLineCount = 0;
    return true;
  }else if(input_buffer_pos == 0 && httpState == WEBSOCKET_RESPOND && ++newLineCount == 4){
    // When we receive the header delimiter then respond with the correct headers
    uint8_t *hash;
    char result[21];
    char b64Result[30];

    Sha1.init();
    Sha1.print(webSocketKey);
    hash = Sha1.result();

    for (int i=0; i<20; ++i) {
      result[i] = (char)hash[i];
    }
    result[20] = '\0';

    base64_encode(b64Result, result, 20);

    _s->print("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ");
    _s->print(b64Result);
    _s->print("\r\n\r\n");
    newLineCount = 0;
    httpState = WEBSOCKET_READY;
    socketMode = WEBSOCKET;
    return true;
  }else if(input_buffer_pos == 0 && httpState == HTTP_RESPOND && ++newLineCount == 4){
    httpState = HTTP_READY;
    socketMode = HTTP;
    _s->print("HTTP/1.1 200 OK\r\nContent-Type: application/json;\r\nTransfer-Encoding: chunked\r\n\r\n");
  }
  return false;
}

boolean CmdProcessor::processWSFrame(){
  boolean fin = false;
  uint8_t opcode = 0;
  boolean mask_set = false;
  uint8_t length = 0;
  uint8_t mask[4] = {0,0,0,0};


  if(httpState == WEBSOCKET_READY && input_buffer_pos > 6){

    // byte 1
    fin = input_buffer[0] >> 7;
    opcode = input_buffer[0] & 0x0F;

    if(fin != 1 && opcode != 0x01 && opcode != 0x08){
      //It's not a websocket frame or it's not final
      return false;
    }

    //byte 2
    mask_set = input_buffer[1] >> 7;
    length = input_buffer[1] & 0x7F;

    if(input_buffer_pos >= (length + 6)){
      if(length < 125){
          //extract the mask
          if(mask_set){
            for(char i = 0; i<4; i++){
              mask[i] = input_buffer[i + 2];
            }
          }
          //process the message
          for(uint8_t i=0; i<length; i++){
            input_buffer[i] = (char)(input_buffer[i+6] ^ mask[i % 4]);
          }
          input_buffer_pos = length;
          input_buffer[input_buffer_pos] = '\0';
          if(opcode == 0x08){
            // The socket is closing, we need to send a close frame in response
            _s->write(0x88);
            _s->write(0x02);
            _s->write(0x03);
            _s->write(0xe9);
          }else{
            processJSON();
          }
          return true;
      }else{
        sendResponse("error", "Message too long", (char&)"");
        return true;
      }
    }
  }
  return false;
}


boolean CmdProcessor::processJSON(){
  char cmd[14], arg[11], id[11];
  if(input_buffer_pos > 0 && input_buffer[0] == '{' && input_buffer[input_buffer_pos - 1] == '}'){
    extractAttr("cmd", input_buffer, cmd, 13);
    extractAttr("arg", input_buffer, arg, 10);
    extractAttr("id", input_buffer, id, 10);
    processCmd(*cmd, *arg, *id);
    return true;
  }
  return false;
}

void CmdProcessor::processCmd(char &cmd, char &arg, char &id){
  char v[8];
  char sensorState[6];
  if(!strcmp(&cmd, "ping")){
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "reset")){
    sendResponse("complete", "", id);
    _m->reset();
  }else if(!strcmp(&cmd, "version")){
    sendResponse("complete", Evelab_VERSION, id);
  }else if(!strcmp(&cmd, "hwversion")){
    if(!_m->hwVersion.major && !_m->hwVersion.minor){
      sendResponse("complete", "unknown", id);
    }else{
      sprintf(v, "%d.%d", _m->hwVersion.major, _m->hwVersion.minor);
      sendResponse("complete", v, id);
    }
  }else if(!strcmp(&cmd, "sethwversion")){
    _m->setHwVersion(arg);
    sprintf(v, "%d.%d", _m->hwVersion.major, _m->hwVersion.minor);
    sendResponse("complete", v, id);
  }else if(!strcmp(&cmd, "pause")){
    _m->pause();
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "resume")){
    _m->resume();
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "stop")){
    _m->stop();
    sendResponse("complete", "", id);
  }
  else if(!strcmp(&cmd, "gpio_on")){
    _m->gpioOn(atoi(&arg));
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "gpio_off")){
    _m->gpioOff(atoi(&arg));
    sendResponse("complete", "", id);
  }

  else if(!strcmp(&cmd, "analogInput")){
    sprintf(sensorState, "%d", _m->getAnalogPinValue(atoi(&arg)));
    sendResponse("complete", sensorState, id);
  }
  else if(!strcmp(&cmd, "mpr121")){
    sprintf(sensorState, "%d", _m->mpr121read());
    sendResponse("complete", sensorState, id);
  }

  else if(!strcmp(&cmd, "gpio_pwm_3")){
    _m->gpioPWM3(atoi(&arg));
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "gpio_pwm_5")){
    _m->gpioPWM5(atoi(&arg));
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "gpio_pwm_6")){
    _m->gpioPWM6(atoi(&arg));
    sendResponse("complete", "", id);
  }

  else if(!strcmp(&cmd, "temp_c")){
    sprintf(sensorState, "%d", _m->getTempC());
    sendResponse("complete", sensorState, id);
  }else if(!strcmp(&cmd, "temp_f")){
    sprintf(sensorState, "%d", _m->getTempF());
    sendResponse("complete", sensorState, id);
  }else if(!strcmp(&cmd, "set_temp_pin")){
    _m->setTempPin(atoi(&arg));
    sendResponse("complete", "", id);
  }

  else if(!strcmp(&cmd, "cds_read")){
    sprintf(sensorState, "%d", _m->getCDSLumens());
    sendResponse("complete", sensorState, id);
  }else if(!strcmp(&cmd, "set_cds_pin")){
    _m->setCDSPin(atoi(&arg));
    sendResponse("complete", "", id);
  }

  else if(!strcmp(&cmd, "set_speaker")){
    _m->setSpeakerPin(atoi(&arg));
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteC")){
    _m->note(atoi(&arg),NOTE_C);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteCSHARP")){
    _m->note(atoi(&arg),NOTE_CSHARP);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteD")){
    _m->note(atoi(&arg),NOTE_D);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteDSHARP")){
    _m->note(atoi(&arg),NOTE_DSHARP);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteE")){
    _m->note(atoi(&arg),NOTE_E);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteF")){
    _m->note(atoi(&arg),NOTE_F);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteFSHARP")){
    _m->note(atoi(&arg),NOTE_FSHARP);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteG")){
    _m->note(atoi(&arg),NOTE_G);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteGSHARP")){
    _m->note(atoi(&arg),NOTE_GSHARP);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteA")){
    _m->note(atoi(&arg),NOTE_A);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteASHARP")){
    _m->note(atoi(&arg),NOTE_ASHARP);
    sendResponse("complete", "", id);
  }else if(!strcmp(&cmd, "noteB")){
    _m->note(atoi(&arg),NOTE_B);
    sendResponse("complete", "", id);
  }else{
    // It's a command that runs for some time
    if(in_process){
      // the previous command hasn't finished, send an error
      sendResponse("error", "Previous command not finished", id);
    }else{
      strcpy(current_id, &id);
      in_process = true;
      //time consuming processes here
      if (0) {
      } else{
        // the command isn't recognised, send an error
        sendResponse("error", "Command not recognised", id);
        return;
      }
      sendResponse("accepted", "", *current_id);
    }
  }
}

void CmdProcessor::sendResponse(const char status[], const char msg[], char &id){
  //Calculate the length of the message for websockets and chunked encoding
  unsigned char len = 13 + strlen(status);
  if(strcmp(msg, "")){ len += 10 + strlen(msg); }
  if(strcmp(&id, "")){ len += 9  + strlen(&id); }

  if(socketMode == WEBSOCKET){
    _s->write(0x81);
    _s->write(len & B01111111);
  }else if(socketMode == HTTP){
    _s->println(len);
  }

  _s->print("{\"status\":\"");
  _s->print(status);
  _s->print("\"");

  if(strcmp(msg, "")){
    _s->print(", \"msg\":\"");
    _s->print(msg);
    _s->print("\"");
  }
  if(strcmp(&id, "")){
    _s->print(", \"id\":\"");
    _s->print(&id);
    _s->print("\"");
  }
  _s->print("}");

  if(socketMode == HTTP && !strcmp(status, "complete")){
    // Close the socket if it's a complete message
    _s->print("0\r\n\r\n");
  }
  if(socketMode == RAW){
    _s->print("\r\n");
  }
}

/*void CmdProcessor::collideNotify(const char state[]){
  sendResponse("notify", state, (char &)"collide");
}

void CmdProcessor::followNotify(int state){
  char sensorState[6];
  sprintf(sensorState, "%d", state);
  sendResponse("notify", sensorState, (char &)"follow");
}*/

//This is a very naive JSON parser which will extract the value of a specific attribute
//It puts a string into the output buffer. Examples of extracting attr:
//  {"attr":"one"} => "one"
//  {"attr":1}     => "1"
//  {"attr":[1, 2]} => "1,2"
//  {"attr":["one",2] "one,2"
void CmdProcessor::extractAttr(const char attr[], char *json, char *output, char len){
  parseState_t parseState = EXPECT_ATTR;
  char attrPos = 0;
  boolean match = false;
  boolean in_brackets = false;
  boolean in_quotes = false;

  while(*json != '\0'){
    switch(parseState){
      case EXPECT_ATTR:
        if(*json == '"'){
          parseState = ATTR;
          attrPos = 0;
        }
        break;
      case ATTR:
        if(*json == '"'){
          *json++;
          while((*json == ':' || *json == ' ') && *json != '\0'){
            *json++;
          }
          parseState = VAL;
        }else{
          //check the attribute
          if(attr[attrPos] == *json){
            attrPos++;
            if(attr[attrPos] == '\0'){
              match = true;
            }
          }
          break;
        }
      case VAL:
        if(!in_quotes && !in_brackets && *json == '['){
          in_brackets = true;
        }else if(!in_quotes && *json == '"'){
          in_quotes = true;
        }else if(!in_quotes && in_brackets && *json == ']'){
          in_brackets = false;
        }else if(in_quotes && *json == '"'){
          in_quotes = false;
        }else if(!in_quotes && in_brackets && *json == ' '){
        }else if(!in_quotes && !in_brackets && (*json == ',' || *json == ' ' || *json == '}')){
          if(match){
            *output = '\0';
            return;
          }else{
            parseState = EXPECT_ATTR;
          }
        }else{
          //copy the attribute if it's a match
          if(match){
            *output = *json;
            *output++;
            if(!--len){
              *output = '\0';
              return;
            }
          }
        }
        break;
    }
    *json++;
  }
}
