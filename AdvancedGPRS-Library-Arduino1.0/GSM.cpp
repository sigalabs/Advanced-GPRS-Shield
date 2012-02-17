/*
    GSM.cpp - library for the Advanced GPRS Shield - SiGAlabs
	www.sigalabs.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


 #if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
  #include "WProgram.h"
#endif

#include "GSM.h"

extern "C" {
  #include <string.h>
}


/**********************************************************
Method returns GSM library version

return val: 100 means library version 1.00
            101 means library version 1.01
**********************************************************/
int GSM::GSMLibVer(void)
{
  return (GSM_LIB_VERSION);
}

/**********************************************************
  Constructor definition
***********************************************************/

GSM::GSM(void)
{
  // set some GSM pins as inputs, some as outputs
  pinMode(GSM_ON, OUTPUT);               // sets pin 4 as output
   // not registered yet
  module_status = STATUS_NONE;
  
 }


/**********************************************************
Methods return the state of corresponding
bits in the status variable

- these methods do not communicate with the GSM module

return values: 
      0 - not true (not active)
      >0 - true (active)
**********************************************************/
byte GSM::IsRegistered(void)
{
  return (module_status & STATUS_REGISTERED);
}

byte GSM::IsInitialized(void)
{
  return (module_status & STATUS_INITIALIZED);
}

/**********************************************************
  Checks if the GSM module is responding 
  to the AT command
  - if YES  nothing is made 
  - if NO   switch on sequence is repeated until there is a response
				from GSM module
**********************************************************/
void GSM::TurnOn(void)
{
  SetCommLineStatus(CLS_ATCMD);

  while (AT_RESP_ERR_NO_RESP == SendATCmdWaitResp("AT", 500, 20, "OK", 5)) {
    // there is no response => turn on the module
  
#ifdef DEBUG_PRINT
    // parameter 0 - because module is off so it is not necessary 
    // to send finish AT<CR> here
    DebugPrint("DEBUG: GSM module is off\r\n", 0);
#endif
    
    // generate switch on pulse
    digitalWrite(GSM_ON, HIGH);
    delay(1200);
    digitalWrite(GSM_ON, LOW);
    delay(1200);

    delay(1500); // wait before next try
  }
  SetCommLineStatus(CLS_FREE);

  // send collection of first initialization parameters for the GSM module    
  InitParam(PARAM_SET_0);
}


/**********************************************************
  Sends parameters for initialization of GSM module

  group:  0 - parameters of group 0 - not necessary to be registered in the GSM
          1 - parameters of group 1 - it is necessary to be registered
**********************************************************/
void GSM::InitParam(byte group)
{

  switch (group) {
    case PARAM_SET_0:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;
      SetCommLineStatus(CLS_ATCMD);

      // Reset to the factory settings
      SendATCmdWaitResp("AT&F0", 1000, 20, "OK", 5);      
      // switch off echo
      SendATCmdWaitResp("ATE0", 500, 20, "OK", 5);
      // setup auto baud rate
      SendATCmdWaitResp("AT+IPR=0", 500, 20, "OK", 5);
      SetCommLineStatus(CLS_FREE);
      break;

    case PARAM_SET_1:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;
      SetCommLineStatus(CLS_ATCMD);

      // set the SMS mode to text 
      SendATCmdWaitResp("AT+CMGF=1", 500, 20, "OK", 5);
      // init SMS storage
      InitSMSMemory();
      // select phonebook memory storage
      SendATCmdWaitResp("AT+CPBS=\"SM\"", 1000, 20, "OK", 5);
      break;
  }
  
}

/**********************************************************
Method checks if the GSM module is registered in the GSM net
- this method communicates directly with the GSM module
  in contrast to the method IsRegistered() which reads the
  flag from the module_status (this flag is set inside this method)

- must be called regularly - from 1sec. to cca. 10 sec.

return values: 
      REG_NOT_REGISTERED  - not registered
      REG_REGISTERED      - GSM module is registered
      REG_NO_RESPONSE     - GSM doesn't response
      REG_COMM_LINE_BUSY  - comm line between GSM module and Arduino is not free
                            for communication
**********************************************************/
byte GSM::CheckRegistration(void)
{
  byte status;
  byte ret_val = REG_NOT_REGISTERED;

  if (CLS_FREE != GetCommLineStatus()) return (REG_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  Serial.println(F("AT+CREG?"));
  // 5 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  status = WaitResp(5000, 20); 

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
    if(IsStringReceived("+CREG: 0,1") 
      || IsStringReceived("+CREG: 0,5")) {
      // it means module is registered
      // ----------------------------
      module_status |= STATUS_REGISTERED;
    
    
      // in case GSM module is registered first time after reset
      // sets flag STATUS_INITIALIZED
      // it is used for sending some init commands which 
      // must be sent only after registration
      // --------------------------------------------
      if (!IsInitialized()) {
        module_status |= STATUS_INITIALIZED;
        SetCommLineStatus(CLS_FREE);
        InitParam(PARAM_SET_1);
      }
      ret_val = REG_REGISTERED;      
    }
    else {
      // NOT registered
      // --------------
      module_status &= ~STATUS_REGISTERED;
      ret_val = REG_NOT_REGISTERED;
    }
  }
  else {
    // nothing was received
    // --------------------
    ret_val = REG_NO_RESPONSE;
  }
  SetCommLineStatus(CLS_FREE);
 

  return (ret_val);
}

/**********************************************************
Method checks status of call

return: 
      CALL_NONE         - no call activity
      CALL_INCOM_VOICE  - incoming voice
      CALL_ACTIVE_VOICE - active voice
      CALL_NO_RESPONSE  - no response to the AT command 
      CALL_COMM_LINE_BUSY - comm line is not free
**********************************************************/
byte GSM::CallStatus(void)
{
  byte ret_val = CALL_NONE;

  if (CLS_FREE != GetCommLineStatus()) return (CALL_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  Serial.println(F("AT+CPAS"));

  // 5 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(5000, 20)) {
    // nothing was received (RX_TMOUT_ERR)
    // -----------------------------------
    ret_val = CALL_NO_RESPONSE;
  }
  else {
    // something was received but what was received?
    // ---------------------------------------------
    // ready (device allows commands from TA/TE)
    // <CR><LF>+CPAS: 0<CR><LF> <CR><LF>OK<CR><LF>
    // unavailable (device does not allow commands from TA/TE)
    // <CR><LF>+CPAS: 1<CR><LF> <CR><LF>OK<CR><LF> 
    // unknown (device is not guaranteed to respond to instructions)
    // <CR><LF>+CPAS: 2<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
    // ringing
    // <CR><LF>+CPAS: 3<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
    // call in progress
    // <CR><LF>+CPAS: 4<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
    if(IsStringReceived("0")) { 
      // ready - there is no call
      // ------------------------
      ret_val = CALL_NONE;
    }
    else if(IsStringReceived("3")) { 
      // incoming call
      // --------------
      ret_val = CALL_INCOM_VOICE;
    }
    else if(IsStringReceived("4")) { 
      // active call
      // -----------
      ret_val = CALL_ACTIVE_VOICE;
    }
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);

}

/**********************************************************
Method checks status of call(incoming or active) 
and makes authorization with specified SIM positions range

phone_number: a pointer where the tel. number string of current call will be placed
              so the space for the phone number string must be reserved - see example
first_authorized_pos: initial SIM phonebook position where the authorization process
                      starts
last_authorized_pos:  last SIM phonebook position where the authorization process
                      finishes

                      Note(important):
                      ================
                      In case first_authorized_pos=0 and also last_authorized_pos=0
                      the received incoming phone number is NOT authorized at all, so every
                      incoming is considered as authorized (CALL_INCOM_VOICE_NOT_AUTH is returned)

return: 
      CALL_NONE                   - no call activity
      CALL_INCOM_VOICE_AUTH       - incoming voice - authorized
      CALL_INCOM_VOICE_NOT_AUTH   - incoming voice - not authorized
      CALL_ACTIVE_VOICE           - active voice
      CALL_INCOM_DATA_AUTH        - incoming data call - authorized
      CALL_INCOM_DATA_NOT_AUTH    - incoming data call - not authorized  
      CALL_ACTIVE_DATA            - active data call
      CALL_NO_RESPONSE            - no response to the AT command 
      CALL_COMM_LINE_BUSY         - comm line is not free
**********************************************************/
byte GSM::CallStatusWithAuth(char *phone_number,
                             byte first_authorized_pos, byte last_authorized_pos)
{
  byte ret_val = CALL_NONE;
  byte search_phone_num = 0;
  byte i;
  byte status;
  char *p_char; 
  char *p_char1;

  phone_number[0] = 0x00;  // no phonr number so far
  if (CLS_FREE != GetCommLineStatus()) return (CALL_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  Serial.println(F("AT+CLCC"));

  // 5 sec. for initial comm tmout
  // and max. 1500 msec. for inter character timeout
  RxInit(5000, 1500, 1, 1);
  // wait response is finished
  do {
    if (IsStringReceived("OK\r\n")) { 
      // perfect - we have some response, but what:

      // there is either NO call:
      // <CR><LF>OK<CR><LF>

      // or there is at least 1 call
      // +CLCC: 1,1,4,0,0,"+420XXXXXXXXX",145<CR><LF>
      // <CR><LF>OK<CR><LF>
      status = RX_FINISHED;
      break; // so finish receiving immediately and let's go to 
             // to check response 
    }
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  // generate tmout 30msec. before next AT command
  delay(30);

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // example: //+CLCC: 1,1,4,0,0,"+420XXXXXXXXX",145
    // ---------------------------------------------
    if(IsStringReceived("+CLCC: 1,1,4,0,0")) { 
      // incoming VOICE call - not authorized so far
      // -------------------------------------------
      search_phone_num = 1;
      ret_val = CALL_INCOM_VOICE_NOT_AUTH;
    }
    else if(IsStringReceived("+CLCC: 1,1,4,1,0")) { 
      // incoming DATA call - not authorized so far
      // ------------------------------------------
      search_phone_num = 1;
      ret_val = CALL_INCOM_DATA_NOT_AUTH;
    }
    else if(IsStringReceived("+CLCC: 1,0,0,0,0")) { 
      // active VOICE call - GSM is caller
      // ----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_VOICE;
    }
    else if(IsStringReceived("+CLCC: 1,1,0,0,0")) { 
      // active VOICE call - GSM is listener
      // -----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_VOICE;
    }
    else if(IsStringReceived("+CLCC: 1,1,0,1,0")) { 
      // active DATA call - GSM is listener
      // ----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_DATA;
    }
    else if(IsStringReceived("+CLCC:")){ 
      // other string is not important for us - e.g. GSM module activate call
      // etc.
      // IMPORTANT - each +CLCC:xx response has also at the end
      // string <CR><LF>OK<CR><LF>
      ret_val = CALL_OTHERS;
    }
    else if(IsStringReceived("OK")){ 
      // only "OK" => there is NO call activity
      // --------------------------------------
      ret_val = CALL_NONE;
    }

    
    // now we will search phone num string
    if (search_phone_num) {
      // extract phone number string
      // ---------------------------
      p_char = strchr((char *)(comm_buf),'"');
      p_char1 = p_char+1; // we are on the first phone number character
      p_char = strchr((char *)(p_char1),'"');
      if (p_char != NULL) {
        *p_char = 0; // end of string
        strcpy(phone_number, (char *)(p_char1));
      }
      
      if ( (ret_val == CALL_INCOM_VOICE_NOT_AUTH) 
           || (ret_val == CALL_INCOM_DATA_NOT_AUTH)) {

        if ((first_authorized_pos == 0) && (last_authorized_pos == 0)) {
          // authorization is not required => it means authorization is OK
          // -------------------------------------------------------------
          if (ret_val == CALL_INCOM_VOICE_NOT_AUTH) ret_val = CALL_INCOM_VOICE_AUTH;
          else ret_val = CALL_INCOM_DATA_AUTH;
        }
        else {
          // make authorization
          // ------------------
          SetCommLineStatus(CLS_FREE);
          for (i = first_authorized_pos; i <= last_authorized_pos; i++) {
            if (ComparePhoneNumber(i, phone_number)) {
              // phone numbers are identical
              // authorization is OK
              // ---------------------------
              if (ret_val == CALL_INCOM_VOICE_NOT_AUTH) ret_val = CALL_INCOM_VOICE_AUTH;
              else ret_val = CALL_INCOM_DATA_AUTH;
              break;  // and finish authorization
            }
          }
        }
      }
    }
    
  }
  else {
    // nothing was received (RX_TMOUT_ERR)
    // -----------------------------------
    ret_val = CALL_NO_RESPONSE;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method picks up an incoming call

return: 
**********************************************************/
void GSM::PickUp(void)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  Serial.println(F("ATA"));
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method hangs up incoming or active call

return: 
**********************************************************/
void GSM::HangUp(void)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  Serial.println(F("ATH0\r"));
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method calls the specific number

number_string: pointer to the phone number string 
               e.g. gsm.Call("+420123456789"); 

**********************************************************/
void GSM::Call(char *number_string)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  // ATDxxxxxx;<CR>
  Serial.print(F("ATD"));
  Serial.print(number_string);    
  Serial.print(F(";\r"));
  // 10 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  WaitResp(10000, 20);
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method calls the number stored at the specified SIM position

sim_position: position in the SIM <1...>
              e.g. gsm.Call(1);
**********************************************************/
void GSM::Call(int sim_position)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  // ATD>"SM" 1;<CR>
  Serial.print(F("ATD>\"SM\" "));
  Serial.print(sim_position);    
  Serial.print(F(";\r"));

  // 10 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  WaitResp(10000, 20);

  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method sets speaker volume

speaker_volume: volume in range 0..14

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..100 current speaker volume 
**********************************************************/
char GSM::SetSpeakerVolume(byte speaker_volume)
{
  
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // remember set value as last value
  if (speaker_volume > 100) speaker_volume = 100;
  // select speaker volume (0 to 100)
  // AT+CLVL=X<CR>   X<0..100>
  Serial.print(F("AT+CLVL="));
  Serial.print((int)speaker_volume);    
  Serial.print(F("\r")); // send <CR>
  // 10 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(10000, 20)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      last_speaker_volume = speaker_volume;
      ret_val = last_speaker_volume; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method increases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..100 current speaker volume 
**********************************************************/
char GSM::IncSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value < 100) {
    current_speaker_value++;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 100;

  return (ret_val);
}

/**********************************************************
Method decreases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..100 current speaker volume 
**********************************************************/
char GSM::DecSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value > 0) {
    current_speaker_value--;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 0;

  return (ret_val);
}

/**********************************************************
Method sends DTMF signal
This function only works when call is in progress

dtmf_tone: tone to send 0..15

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0.. tone
**********************************************************/
char GSM::SendDTMFSignal(byte dtmf_tone)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // e.g. AT+VTS=5<CR>
  Serial.print(F("AT+VTS="));
  Serial.print((int)dtmf_tone);    
  Serial.print(F("\r"));
  // 1 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(1000, 20)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      ret_val = dtmf_tone; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Functions for manipulation with strings
**********************************************************/
/*
 * Skips the string until the given char is found.
 */
char *GSM::Skip(char *str, char match) {
  byte c = 0;
  while (true) {
    c = *str++;
    if ((c == match) || (c == '\0')) {
      break;
    }
  }
  return str;
}


/*
 * Reads a token from the given string. Token is seperated by the 
 * given delimiter.
 */
char *GSM::ReadToken(char *str, char *buf, char delimiter) {
  byte c = 0;
  while (true) {
    c = *str++;
    if ((c == delimiter) || (c == '\0')) {
      break;
    }
    else if (c != ' ') {
      *buf++ = c;
    }
  }
  *buf = '\0';
  return str;
}

/**********************************************************
Method update status of battery charging

return: 
	  BATT_NOT_CHARGING - no charger connected
	  BATT_CHARGING     - battery is charging
	  BATT_FULL         - battery is full
	  
OK ret val:
        -----------
        1 - Battery status updated
		0 - Battery status not updated
		
ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
**********************************************************/
char GSM::CheckBattery()
{
 char ret_val = -1;

 char *p_char; 
 char *p_char1;
 char *p_char2;
 
 if (CLS_FREE != GetCommLineStatus()) return (ret_val);
 SetCommLineStatus(CLS_ATCMD);
 ret_val = 0; // not found yet
 
 Serial.print(F("AT+CBC\r"));
 
 switch (WaitResp(1000, 20, "+CBC")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      if(IsStringReceived("+CBC: 0")){
		SetBattChargeStatus(BATT_NOT_CHARGING);
	  }
	  else if(IsStringReceived("+CBC: 1")){
		SetBattChargeStatus(BATT_CHARGING);
	  }
	  else if(IsStringReceived("+CBC: 2")){
		SetBattChargeStatus(BATT_FULL);
	  }
	  
      p_char = strchr((char *)(comm_buf),',');
      if (p_char != NULL) {
        p_char++;       // we are on the first battery level character
        // find out ',' as finish character of battery level string
        p_char1 = strchr((char *)(p_char),',');
        if (p_char1 != NULL) {
          *p_char1 = 0; // end of string
          batteryLevel= atoi(p_char); //Convert string to integer 0-100%
		  p_char1++;//Move on to first voltage number
		  p_char2 = strchr((char *)(p_char1),'\r');//find the end of the line
		  if (p_char != NULL) {
			p_char2 = 0;
			batteryVoltage= atoi(p_char1);//Convert bat voltage to integer
			ret_val = 1;
		  }
		}
      }
	 
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // only OK or ERROR => no phone number
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);

}

/**********************************************************
Methods return the state of battery charging

- these methods do not communicate with the GSM module

return values: 
      0 - not true (not active)
      >0 - true (active)
**********************************************************/
byte GSM::IsCharging()
{
  if(GetBattChargeStatus()==BATT_CHARGING)
	return 1;
  else
	return 0;
}

/**********************************************************
Methods return the state of the charger

- these methods do not communicate with the GSM module

return values: 
      0 - not true (not connected)
      >0 - true (connected)
**********************************************************/
byte GSM::IsChargerConnected()
{
  if(GetBattChargeStatus()==BATT_NOT_CHARGING)
	return 0;
  else
	return 1;
}

/**********************************************************
Methods returns if battery charge completed

- these methods do not communicate with the GSM module

return values: 
      0 - not true (not completed)
      >0 - true (completed)
**********************************************************/
byte GSM::IsBatteryFull()
{
  if(GetBattChargeStatus()==BATT_FULL)
	return 1;
  else
	return 0;
}


/**********************************************************
Method update signal level
return: 
	  
OK ret val:
        -----------
        1 - Signal Level updated
		0 - Signal level not updated
		
ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
**********************************************************/
char GSM::UpdateSignalLevel()
{
 char ret_val = -1;
 char sig=0;
 char *p_char; 
 char *p_char1;
 
 if (CLS_FREE != GetCommLineStatus()) return (ret_val);
 SetCommLineStatus(CLS_ATCMD);
 ret_val = 0; // not found yet
 
 Serial.print(F("AT+CSQ\r"));
 
 switch (WaitResp(1000, 20, "+CSQ")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      
      p_char = strchr((char *)(comm_buf),':');
      if (p_char != NULL) {
        p_char++;       // we are on the first battery level character
        // find out ',' as finish character of battery level string
        p_char1 = strchr((char *)(p_char),',');
        if (p_char1 != NULL) {
          *p_char1 = 0; // end of string
          sig= atoi(p_char); //Convert string to integer 0-100%
		}
      }
	  
	  if((sig==0) || (sig==99))
		signalLevel=0;
	  else if((sig>0) && (sig<=5))
		signalLevel=1;
	  else if((sig>5) && (sig<=12))
		signalLevel=2;
	  else if((sig>12) && (sig<=17))
		signalLevel=3;
	  else if((sig>17) && (sig<=22))
		signalLevel=4;
	  else if((sig>22) && (sig<=32))
		signalLevel=5;
	  else
	    signalLevel=0;
	  
	 ret_val =1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      
	  ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);

}
