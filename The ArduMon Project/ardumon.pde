/*
##############################################
#  _____ _  _____          _       _         #
# / ____(_)/ ____|   /\   | |     | |        #
#| (___  _| |  __   /  \  | | __ _| |__  ___ #
# \___ \| | | |_ | / /\ \ | |/ _` | '_ \/ __|#
# ____) | | |__| |/ ____ \| | (_| | |_) \__ \#
#|_____/|_|\_____/_/    \_\_|\__,_|_.__/|___/#
##############################################
#          /\/\~Description~/\/\             #
###########################################################################
# The ArduMon project turns your Arduino into a remote monitoring device. #       
# With the use of the SiGAlabs Advanced GPRS Shield and a sim card        #
# you can read the voltages on Arduino's analog inputs, read the inputs,  #
# control lights, dc motors or other devices using relays.                #
# Learn more info and update at official project's website.               #
# (http://www.sigalabs.com/the-ardumon-project)                           #
###########################################################################
#      /\/\~What you need to compile this and make it work~/\/\           #
########################################################################################## 
# 1)An Arduino UNO, MEGA or compatible board                                             # 
# 2)An Advanced GPRS Shield from SiGAlabs (http://www.sigalabs.com)                      #
# 3)The Arduino library of the shield copied to your <ArduinoIDE>\libraries subdirectory #
#    Download it from here (http://github.com/sigalabs/Advanced-GPRS-Shield)             #
# 4)A SIM Card from your provider with PIN CODE DISABLED                                 #
########################################################################################## 
*/

#include <AT.h>
#include <GSM.h>

#define SMS_MAX_LEN 100
#define SMS_PASSWORD "ADMIN"
//Choose the four input pins we use for digital input status
#define IN1 12
#define IN2 11
#define IN3 10
#define IN4 9
//Choose the two outputs for relays 
#define OUT1 6
#define OUT2 5

// definition of instance of GSM class
GSM gsm;

// variables used for timing
unsigned long	previous_timer;
byte timer100msec;

char string[120];
char position;          
char phone_num[20];      // array for the phone number string
char sms_text[SMS_MAX_LEN]; // array for the SMS text
 
int ledPin = 13;  

void setup()
{
  pinMode(ledPin, OUTPUT);      // sets the digital pin as output
  //Set digital input pins
  pinMode(IN1, INPUT);           // set pin to input
  digitalWrite(IN1, HIGH);       // turn on pullup resistors
  pinMode(IN2, INPUT);           // set pin to input
  digitalWrite(IN2, HIGH);       // turn on pullup 
  pinMode(IN3, INPUT);           // set pin to input
  digitalWrite(IN3, HIGH);       // turn on pullup 
  pinMode(IN4, INPUT);           // set pin to input
  digitalWrite(IN4, HIGH);       // turn on pullup 
  //Set output pins
  pinMode(OUT1, OUTPUT);           // set pin to output
  digitalWrite(OUT1, LOW);       // set to off
  pinMode(OUT2, OUTPUT);           // set pin to output
  digitalWrite(OUT2, LOW);       // set to off 

  // initialization of serial line
  gsm.InitSerLine(9600);		
  // turn on GSM module
  gsm.TurnOn();
  
  // periodic timer initialization
  timer100msec = 0;
  previous_timer = millis();
}

void loop()
{
  // -------------------
  // timing of main loop
  // -------------------
  if ((unsigned long)(millis() - previous_timer) >= 100) { 
    previous_timer = millis();  

  }
    
    
    //*******************************
    //****** EVERY 3 sec. ***********
    // %30 means every 3000 msec. = 3sec.
    //*******************************
    if ((timer100msec+4) % 30 == 0) {

      // is there a new UNREAD SMS ?
      // if YES - SIM position > 0 is returned
      // -------------------------------------
      if (gsm.IsRegistered()) {  
        // GSM module is registered

        // Note: if there is new SMS before IsSMSPresent() is executed
        // this SMS has a status UNREAD
        // after calling IsSMSPresent() method status of SMS
        // is automatically changed to READ
   
        //Just to signal we are working
		digitalWrite(ledPin, HIGH);   // sets the LED on
        delay(200);                  
        digitalWrite(ledPin, LOW);    // sets the LED off
        delay(200);    

   
        position = gsm.IsSMSPresent(SMS_ALL);
        if (position > 0) {

          // we have new SMS 
          // now we will make authorization with SIM phonebook pos. 1,2,3
          // ------------------------------------------------------------
          gsm.GetSMS(position, phone_num, sms_text, 100);
          
            // so lets check SMS text
            // --------------------------------------
            if (strstr(subStr(sms_text," ",1), SMS_PASSWORD) != NULL)
            {
               //password is correct, so check the command type
                  
              if (strstr(subStr(sms_text," ",2), "ANALOG") != NULL) 
              {  
                sprintf(string, "A0:%i A1:%i A2:%i A3:%i A4:%i A5:%i",analogRead(0),analogRead(1),analogRead(2),analogRead(3),analogRead(4),analogRead(5));
                gsm.SendSMS(phone_num, string ); 
              }
              else if( strstr(subStr(sms_text," ",2), "INPUTS")!=NULL)
              {
                  sprintf(string, "IN1:%i IN2:%i IN3:%i IN4:%i", digitalRead(IN1),digitalRead(IN2),digitalRead(IN3),digitalRead(IN4));
                  gsm.SendSMS(phone_num, string );  
              }
              else if( strstr(subStr(sms_text," ",2), "OUTPUTS")!=NULL)
              {
                  sprintf(string, "OUT1:%i OUT2:%i", digitalRead(OUT1),digitalRead(OUT2));
                  gsm.SendSMS(phone_num, string ); 
              }
              else if( strstr(subStr(sms_text," ",2), "OUT1")!=NULL)
              {
                if ( strstr(subStr(sms_text," ",3), "ON")!=NULL)
                {
                   digitalWrite(OUT1,HIGH); 
                   gsm.SendSMS(phone_num, "OUT1 IS ON" ); 
                }
                else if  ( strstr(subStr(sms_text," ",3), "OFF")!=NULL)
                {
                   digitalWrite(OUT1,LOW); 
                   gsm.SendSMS(phone_num, "OUT1 IS OFF" );
                } 
              }
              else if( strstr(subStr(sms_text," ",2), "OUT2")!=NULL)
              {
                if ( strstr(subStr(sms_text," ",3), "ON")!=NULL)
                {
                   digitalWrite(OUT2,HIGH); 
                   gsm.SendSMS(phone_num, "OUT2 IS ON" ); 
                }
                else if  ( strstr(subStr(sms_text," ",3), "OFF")!=NULL)
                {
                   digitalWrite(OUT2,LOW); 
                   gsm.SendSMS(phone_num, "OUT2 IS OFF" );
                } 
              }
            }

          // and delete received SMS 
          // to leave place for next new SMS's
          // ---------------------------------
          gsm.DeleteSMS(position);
        }
      }
    }
    
    
    
    //********************************************
    //********WRAP AROUND COUNTER 10 sec. ********
    //********************************************
    timer100msec = (timer100msec + 1) % 100;
  }	  



// Function to return a substring defined by a delimiter at an index
char* subStr (char* str, char *delim, int index) {
   char *act, *sub, *ptr;
   static char copy[SMS_MAX_LEN];
   int i;

   // Since strtok consumes the first arg, make a copy
   strcpy(copy, str);

   for (i = 1, act = copy; i <= index; i++, act = NULL) {
	//Serial.print(".");
	sub = strtok_r(act, delim, &ptr);
	if (sub == NULL) break;
   }
   return sub;

}
