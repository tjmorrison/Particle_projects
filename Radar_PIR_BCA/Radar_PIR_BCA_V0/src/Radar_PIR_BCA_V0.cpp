/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/Radar_PIR_BCA_V0/src/Radar_PIR_BCA_V0.ino"
/*
 * Project Dist_Radar_V0
 * Description: This code is testing the RCWL 05516 distance Radar with the
 *  particle board for trail head detection system. 
 * Author: Travis Morrison
 * Date: April 6 2021 
 * 
 * Project to Do:
 * Prioity:
 * Add sleep
 * add logic to count beacons
 * Add PIR sensor logic 
 * prototype enclosure
 * backup data protection?
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

//Google maps logic
//#include "Particle.h"
//#include <google-maps-device-locator.h>

//GoogleMapsDeviceLocator locator;

//Define I/O pins used
void setup();
void loop();
void CheckDIO(int signal_data[][3]);
#line 37 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/Radar_PIR_BCA_V0/src/Radar_PIR_BCA_V0.ino"
#define Pin_Radar 2
#define Pin_PIR 3
#define Pin_Beacon 4

//Define fuel guage for battery monitoring 
FuelGauge fuel;
// PROTOTYPE

SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;//

// How often to publish a value
const std::chrono::milliseconds publishPeriod = 3600s; //300s is every 5 min, 3600s is 1 hr

// The event name to publish with
const char *eventName = "sheetTest1";

unsigned long lastPublish;
int signal_data[3][3]={0}; // This will be the radar array cols =  [signal,state, counter] rows = [Radar, PIR, BCA]

char message = 'here'; //Message which shows connection has been made


//Decleration of functions~ tells the compiler which functions are available and how to use them
void PublishToGoogleSheet();
void CheckDIO(int *signal);
float getVCell(); //battery voltage status


// setup() runs once, when the device is first turned on.
void setup() {
  
  //Make variable public
  Particle.variable("Signal_data", signal_data);
 
  //Set the digital pins to input
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);

  // Scan for visible networks and publish to the cloud every 30 seconds
  // Pass the returned location to be handled by the locationCallback() method
  //locator.withSubscribe(locationCallback).withLocatePeriodic(30);
  //locator.withEventName("Apollo has landed");
}

// main loop for code, runs continously
void loop() {
  // Read sesnor and store value
  signal_data[1][1] =  digitalRead(Pin_Radar);
  signal_data[2][1] =  digitalRead(Pin_PIR);
  signal_data[3][1] = digitalRead(Pin_Beacon);

  CheckDIO(signal_data);

  //Use millis to write to google sheets every defined period
  if (millis() - lastPublish >= publishPeriod.count()) {
        if (Particle.connected()) {
            lastPublish = millis();
             
            //Log.info( "voltage=%.2f", fuel.getVCell() );
            // Publish
            PublishToGoogleSheet();
            //reset counter after data is published
            signal_data[1][3] = 0; 
            signal_data[2][3] = 0; 
            signal_data[3][3] = 0; 
        }
    }
     
  //Find location with google maps API
  //locator.loop();
  
  //run every second
  delay(1000);
}

//Publish to google sheets function
void PublishToGoogleSheet() {
    char buf[128];

    snprintf(buf, sizeof(buf), "[%d,%d,%d]", signal_data[1][3],signal_data[2][3],signal_data[3][3]);

    Particle.publish(eventName, buf, PRIVATE);
    Log.info("published: %s", buf);
}

//Function to handle DIO input
void CheckDIO(int signal_data[][3]){
  for (int ii = 0; ii < 3; ii++){
    if (signal_data[ii][1] == HIGH){ // check if the input is HIGH
      if (signal_data[ii][2] == LOW){
        // we have just turned on
        signal_data[ii][2] = HIGH;
        // add 1 motion count
        signal_data[ii][3] = 1; // we only count at the start of motion
      }
    }
    else{
      if (signal_data[ii][2] == HIGH){
        // we have just turned off
        signal_data[ii][2] = LOW;
      }
    } // end DIO logic
  }
}

//Google maps API function to find location with cell towers
//void locationCallback(float lat, float lon, float accuracy) {
  // Handle the returned location data for the device. This method is passed three arguments:
  // - Latitude
  // - Longitude
  // - Accuracy of estimated location (in meters)
//}