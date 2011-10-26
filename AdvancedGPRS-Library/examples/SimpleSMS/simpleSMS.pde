/*
    SMS with Advanced GPRS Shield - SiGAlabs (www.sigalabs.com)

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

 /* 
 
    Description
    -----------------------------------
    This is a demo sketch just to see how easily you can send a text message
    using the Advanced GPRS Shield.
    
    1)Insert a SIM card with PIN code disabled to the back of the shield.
    2)Program your Arduino board with this sketch.
    3)Power off your Arduino board.
    4)Connect the Advanced GPRS Shield on your Arduino board.
    5)Power on the Arduino board.
    
    Arduino will power on the SIM900D module on the shield and try to register to your provider network.
    When this is done it will send a text message to your mobile number (see the code) and then stay in the loop() function.
    Just press the reset button to send again!!
    
    Have fun!!
    
 */

#include "GSM.h"  

// definition of instance of GSM class
GSM gsm;


void setup()
{
  // initialization of serial line
  gsm.InitSerLine(9600);		
  // turn on GSM module
  gsm.TurnOn();


// wait until a GSM module is registered in the GSM network
  while (!gsm.IsRegistered()) {
    gsm.CheckRegistration();
    delay(1000);
  }
     
   //Send SMS->   Number   ,         Text Message  
   gsm.SendSMS("6937800917", "Hello! I am the Advanced GPRS Shield by SiGAlabs!"); 


}


void loop()
{
 
}
