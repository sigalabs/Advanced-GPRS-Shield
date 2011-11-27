/*
    GSM.h - library for the Advanced GPRS Shield - SiGAlabs
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



#ifndef __GSM
#define __GSM

#include "WProgram.h"

#define GSM_LIB_VERSION 011 // library version X.YY (e.g. 1.00)



// in case it is required to use DEBUG_PRINT sections in this module
// please enable this constant in AT.h file


// if defined - debug LED is enabled, otherwise debug LED is disabled
#define DEBUG_LED_ENABLED

// if defined - SMSs are not send(are finished by the character 0x1b
// which causes that SMS are not sent)
// by this way it is possible to develop program without paying for SMSs 
#define DEBUG_SMS_ENABLED



// length for the internal communication buffer
#define COMM_BUF_LEN        200

#include "AT.h"
#include "GSM_GPRS.h"



// pins definition
#define GSM_ON              4 // connect GSM Module turn ON to pin 4 
#define GSM_STATUS          2 // connect GSM Module STATUS to pin 2
#define GSM_RING		    3 // connect DTMF Output Enable to pin 3


// some constants for the InitParam() method
#define PARAM_SET_0   0
#define PARAM_SET_1   1

// status bits definition
#define STATUS_NONE                 0
#define STATUS_INITIALIZED          1
#define STATUS_REGISTERED           2
#define STATUS_USER_BUTTON_ENABLE   4


// Time-Delays
#define START_TINY_COMM_TMOUT       20
#define START_SHORT_COMM_TMOUT      500
#define START_LONG_COMM_TMOUT       1000
#define START_XLONG_COMM_TMOUT      5000
#define START_XXLONG_COMM_TMOUT     7000
#define MAX_INTERCHAR_TMOUT         20
#define MAX_MID_INTERCHAR_TMOUT     100
#define MAX__LONG_INTERCHAR_TMOUT   1500
#define AT_DELAY                    500



class GSM : public AT
{
  public:

    //=================================================================
    // general GSM section: implementaion of methods are placed
    //                      in the GSM.cpp  
    //=================================================================
    // constructor
    GSM(void);

    // library version
    int GSMLibVer(void);
    // turns on GSM module
    void TurnOn(void);
    // sends some initialization parameters
    void InitParam (byte group);
    // enables DTMF decoder
    void EnableDTMF(void);
    // gets DTMF value
    byte GetDTMFSignal(void);
    // turns off/on the speaker
    void SetSpeaker(byte off_on);
    // checks if module is registered in the GSM network
    // must be called regularly
    byte CheckRegistration(void);
	// returns registration state
    byte IsRegistered(void);
    // must be called regularly
    char CheckBattery(void);
	byte IsCharging(void);
	//Batery related
	byte IsChargerConnected(void);
	byte IsBatteryFull(void);
	// returns whether complete initialization was made
    byte IsInitialized(void);
    // finds out the status of call
    byte CallStatus(void);
    byte CallStatusWithAuth(char *phone_number,
                            byte first_authorized_pos, byte last_authorized_pos);
    // picks up an incoming call
    void PickUp(void);
    // hangs up an incomming call
    void HangUp(void);
    // calls the specific number
    void Call(char *number_string);
    // makes a call to the number stored at the specified SIM position
    void Call(int sim_position);

    // Speaker volume methods - set, increase, decrease
    char SetSpeakerVolume(byte speaker_volume);
    char IncSpeakerVolume(void);
    char DecSpeakerVolume(void);

    // sends DTMF signal
    char SendDTMFSignal(byte dtmf_tone);

   
    // Support functions
    char *ReadToken(char *str, char *buf, char delimiter);
    char *Skip(char *str, char match);
	
	//Battery
	byte batteryLevel;
	int batteryVoltage;
	

	
  //=================================================================
    // GPRS section: implementaion of methods are placed
    //                      in the GSM_GPRS.cpp  
    //=================================================================
    int GPRSLibVer(void);
    char InitGPRS(char* apn, char* login, char* password);
    char EnableGPRS(byte open_mode);
    char DisableGPRS(void);
    char OpenSocket(byte socket_type, uint16_t remote_port, char* remote_addr);
    char CloseSocket(void);
    void SendData(char* str_data);
    void SendData(byte* data_buffer, unsigned short size);
    uint16_t RcvData(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, byte** ptr_to_rcv_data);
    signed short StrInBin(byte* p_bin_data, char* p_string_to_search, unsigned short size);


  private:
    //=================================================================
    // Private section for general GSM
    //=================================================================
    // global status - bits are used for representation of states
    byte module_status;
    // last value of speaker volume
    byte last_speaker_volume; 

};
#endif
