/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/PIR_BCA_V1/PIR_BCA_V1/src/PIR_BCA_V1.ino"
/*
 * Project Dist_Radar_V3
 * Description: This code is testing the RCWL 05516 distance Radar with interuptts the
 *  particle board for trail head detection system. 
 * Author: Travis Morrison
 * Date: May 25 2021 
 * 
 * Project to Do:
 * Prioity:
 * Add sleep interupt if battery falls below threshold- may not be nessecary with charge controller
 * backup data protection?
 * prototype enclosure 
 * 
 * Secondary
 * DI/O protection circuit
 * Write every hour on the hour
 * Set clock 1x every day
 * 
 * Wiring
 * RCWL-0516, delay is 5 sec <1mA, range 4 m, 360 deg field of view, can see through acrylic 
 * Based on post 11 here https://github.com/jdesbonnet/RCWL-0516/issues/11#issuecomment-771810294
 * I think R-GN follows Resistance = 0.1089e^(0.5375*distance), so for 3 m this is 0.55 Mohm
 * VIN - appears can take 3.3 V (3V3 pin) or 5 V (VUSB pin) 
 * OUT - D2 (digital pin 2)
 * GND - Ground
 */

//======================================================================================================
//Define I/O pins used
void setup();
void loop();
#line 30 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/PIR_BCA_V1/PIR_BCA_V1/src/PIR_BCA_V1.ino"
const pin_t Pin_PIR = D2;
const pin_t Pin_Beacon = D3;
const pin_t Pin_Battery =  A3;

//Define global variables 
int PIR_cnt = 0; //PIR counter and so forth
int Beacon_cnt = 0; //BCA pin counter and so forth
int Batt_read = 0;
float Batt_volt = 0;

// Define time period to update time, 1x per day
#define WRITE_DATA_MILLIS (30 * 60 * 1000) //needs to be same as sleep interval
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000) //every day

unsigned long lastSync = millis();
unsigned long lastWrite = millis();

SYSTEM_THREAD(ENABLED); //allows the code to run before connecting to the cloud and will run without cloud conncetion
SerialLogHandler logHandler; //
SystemSleepConfiguration config;
FuelGauge fuel; //defines the fuel gauge class

// The event name to publish with has to be same as webhook
const char *eventName = "sheetTest1";

//Decleration of functions~ tells the compiler which functions are available and how to use them
void PublishToGoogleSheet();

// setup() runs once, when the device is first turned on.
void setup() {

  //Set the digital pins to input
  //note that analog pins don't need analog read
  pinMode(Pin_PIR, INPUT);
  pinMode(Pin_Beacon, INPUT);


}

// main loop for code, runs continuously
void loop() {
    //make sure where connected before sleep~don't think I need that
    
    // Sleep command until one of the signals gets a signal
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).flag(SystemSleepFlag::WAIT_CLOUD).duration(WRITE_DATA_MILLIS).gpio(Pin_PIR, RISING).gpio(Pin_Beacon, RISING); 
    //config.mode(SystemSleepMode::STOP).network(NETWORK_INTERFACE_CELLULAR).flag(SystemSleepFlag::WAIT_CLOUD).duration(WRITE_DATA_MILLIS).gpio(Pin_PIR, RISING).gpio(Pin_Beacon, RISING); 
    System.sleep(config);

    if (digitalRead(Pin_PIR)== HIGH){
      PIR_cnt = PIR_cnt + 1;
    }
    if (digitalRead(Pin_Beacon)== HIGH){
      Beacon_cnt = Beacon_cnt + 1;
    }

    //write to google sheets every time it wakes up from sleep routine and since last write is greater than write interval
    if (millis() - lastWrite > WRITE_DATA_MILLIS) {
      if(Particle.connected() == false) {
          Particle.connect();
          waitUntil(Particle.connected);
      }
        // read battery voltage
        Batt_read = analogRead(Pin_Battery);
        Batt_volt = (Batt_read*3.3/4095.0)*5.0; //multiply by 3.3/4095 to convert to voltage, mult. by 5 bc of 5 to 1 voltage divider
        
       
        // Calls Publish to Google sheet function
        PublishToGoogleSheet();
        
        PIR_cnt = 0; 
        Beacon_cnt = 0; 

        delay(1s);
        //reset last write
        lastWrite = millis();
    }

    // Sync time 1x per day
    if(millis() - lastSync > ONE_DAY_MILLIS) {

      if(Particle.connected() == false) {
          Particle.connect();
          waitUntil(Particle.connected);
      }

      // Request time synchronization from the Particle Device Cloud
      //Particle.syncTime(); // sync time
      //waitUntil(Particle.syncTimeDone);
      Particle.publishVitals(); // publish vitals 1x day
      //Log.info( "voltage=%.2f", fuel.getVCell() ); // log battery voltage, redundent
      lastSync = millis();//reset the lastsync
      
    }
    

}


//Publish to google sheets function
void PublishToGoogleSheet() {
    char buf[128];
    snprintf(buf, sizeof(buf),"[%d,%d,%.2f]", PIR_cnt, Beacon_cnt, Batt_volt);
    Particle.publish(eventName, buf, PRIVATE);
    Log.info("published: %s", buf);
}