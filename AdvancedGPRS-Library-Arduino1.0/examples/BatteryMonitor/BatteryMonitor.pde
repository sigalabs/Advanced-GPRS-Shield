/*
    BatteryMonitoring with Advanced GPRS Shield - SiGAlabs (www.sigalabs.com)

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
    This is a demo sketch to show you how to handle the battery monitoring and charging status
    Have fun!!
 */

#include "GSM.h"  

// definition of instance of GSM class
GSM gsm;


void setup()
{
  //Arduino's LED
  pinMode(13,OUTPUT);
  // initialization of serial line
  gsm.InitSerLine(9600);		
  // turn on GSM module
  gsm.TurnOn();
}


void loop()
{
  delay(1000);
  
  //must be called in every loop to update battery state
  gsm.CheckBattery();
 
  if(gsm.batteryLevel < 15)// Warning Low Battery 15%
  {
     //What do you want to do if battery is LOW? 
  }
  
  //Monitoring battery voltage
  // Usefull if you are powering other devices or Arduino from the same battery, because battery voltage can go very low.
  //For example you can power off the GPRS module if voltage drops a lot.
  if(gsm.batteryVoltage < 3300) 
  {
    //Do something for example power off GPRS module
  }
  
  //If charger is connected the LED on Arduino's pin 13 will light!
  if(gsm.IsChargerConnected()==1)
     digitalWrite(13, HIGH);
  else
    digitalWrite(13, LOW);
 
  //Other functions also available:  IsBatteryFull() and IsCharging()
 
}
