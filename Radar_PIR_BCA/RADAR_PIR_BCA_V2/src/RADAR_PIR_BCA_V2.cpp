/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/RADAR_PIR_BCA_V2/src/RADAR_PIR_BCA_V2.ino"
/*
 * Project Dist_Radar_V2
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


//Define I/O pins used
void setup();
void loop();
#line 30 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/RADAR_PIR_BCA_V2/src/RADAR_PIR_BCA_V2.ino"
int Pin_Radar = D2;
int Pin_PIR = D3;
int Pin_Beacon = D4;
int Battery_Volt = A1;

//Define global variables 
int Radar_cnt = 0; //Radar counter and so forth
int PIR_cnt = 0; //Radar counter and so forth
int Beacon_cnt = 0; //Radar counter and so forth

// Define time period to update time, 1x per day
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastSync = millis();

SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;//


// The event name to publish with
const char *eventName = "sheetTest1";

char message = 'Apollo_1 has landed'; //Message which shows connection has been made

//Decleration of functions~ tells the compiler which functions are available and how to use them
void PublishToGoogleSheet(int Radar_cnt, int PIR_cnt, int Beacon_cnt);
void Beacon_plus();
void PIR_plus();
void Radar_plus();

// setup() runs once, when the device is first turned on.
void setup() {
    
  //Make variable public
  //Particle.variable("Signal_data_matri", signal_data);
 
  //Set the digital pins to input
  pinMode(Pin_Radar, INPUT);
  pinMode(Pin_PIR, INPUT);
  pinMode(Pin_Beacon, INPUT);

  //Set battery volt monitor input
  pinMode(A1, INPUT);

  // Set the digital pins to interrupts, otherwise we sleep
  attachInterrupt(Pin_Radar, Radar_plus, RISING);
  attachInterrupt(Pin_PIR, PIR_plus, RISING);
  attachInterrupt(Pin_Beacon, Beacon_plus, RISING);
  //attachInterrupt(Battery_Volt, Battery_sleep, RISING); // will call something like this ~ config.mode(SystemSleepMode::STOP).analog(A2, 1500, AnalogInterruptMode::BELOW);

}

// main loop for code, runs continously
void loop() {

  // pesduo logic we wnat to impliment for TH counting
  // -Disconnect cellular save battery
  // -Every day (1x), connects to cellular for 1-hr and writes data from the day. Having it online for 1-hr 
  //  means we can push new code, etc. During this period the sensor should not allow interupts (disable them)
  // -Sleep when no people are being counted -> means using an interupt with sensors, so each sensor has an intrup
  //  which is triggered when the signal goes high (rising)- Aside, lets make the thing blink when someone or something is counted
  // -Store 1 day's worth of data on RAM, so it can go back and write if something happens, updates every day. 3068 bytes section of backup RAM
  // -Have an analog interupt for when the battery voltage is too low (idk, 10V for us?)- I think the charge 
  //  controller may already do this, so it might be redundent
  // -

    // Sleep command until one of the signals gets a signal
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).gpio(Pin_Radar, RISING).gpio(Pin_PIR, RISING).gpio(Pin_Beacon, RISING).duration(15min); 
    System.sleep(config);

    //disable interupt?
     
    //write to google sheets every time it wakes up from sleep routine
    if (Particle.connected()) {
        // Publish
        PublishToGoogleSheet(Radar_cnt, PIR_cnt, Beacon_cnt);

        //reset counter after data is published
         // only resets if board was connected~ perhaps may be good, but not telling if data was written
        Radar_cnt = 0; 
        PIR_cnt = 0; 
        Beacon_cnt = 0; 
    }

    // Synce time 1x per day
    if (millis() - lastSync > ONE_DAY_MILLIS) {
    // Request time synchronization from the Particle Device Cloud
    Particle.syncTime();
    lastSync = millis();
    }

  //reengage interupt?
}


// Interupt service routines (ISR) these functions can have no inputs/returns. They just add +1 to the value
void Radar_plus(){
    Radar_cnt++;
}
void PIR_plus(){
    PIR_cnt++;
}

void Beacon_plus(){
    Beacon_cnt++;
}


//Publish to google sheets function
void PublishToGoogleSheet(int Radar_cnt, int PIR_cnt, int Beacon_cnt) {
    char buf[128];
    
    char tmp1 = (char) Radar_cnt;
    char tmp2 = (char) PIR_cnt;
    char tmp3 = (char) Beacon_cnt;

    snprintf(buf, sizeof(buf), "[%d,%d,%d]", tmp1, tmp2, tmp3);

    Particle.publish(eventName, buf, PRIVATE);
    Log.info("published: %s", buf);
}