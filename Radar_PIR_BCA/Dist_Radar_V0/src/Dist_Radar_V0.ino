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
#define Pin_Radar 2
#define Pin_PIR 3
#define Pin_Beacon 4

//Define fuel guage for battery monitoring 
FuelGauge fuel;
// PROTOTYPE

SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;//

// How often to publish a value
const std::chrono::milliseconds publishPeriod = 30s; //300s is every 5 min, 3600s is 1 hr

// The event name to publish with
const char *eventName = "sheetTest1";

unsigned long lastPublish;
int RadarSignal = 0;
int PIRSignal = 0;
int BeaconSignal = 0;
int RadarCnt = 0; //Radar counter over avg period
int PIRCnt = 0; //PIR sensor counters over avg period
int BeaconCnt = 0; // Beacon sensor counter over avg period.
char message = 'Apollo_1 has landed'; //Message which shows connection has been made


//Decleration of functions~ tells the compiler which functions are available and how to use them
void PublishToGoogleSheet();
int CheckDIO(int signal);
float getVCell(); //battery voltage status


// setup() runs once, when the device is first turned on.
void setup() {
  
  //Make variable public
  Particle.variable("RadarSignal", RadarSignal);
 
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
  // Read Radar sesnor
  RadarSignal =  digitalRead(Pin_Radar);
  PIRSignal =  digitalRead(Pin_PIR);
  BeaconSignal = digitalRead(Pin_Beacon);


  // add to the radar counter
  RadarCnt = RadarCnt + CheckDIO(RadarSignal);
  PIRCnt = PIRCnt + CheckDIO(PIRSignal);
  BeaconCnt = BeaconCnt + CheckDIO(BeaconSignal);

  //Use millis to write to google sheets every defined period
  if (millis() - lastPublish >= publishPeriod.count()) {
        if (Particle.connected()) {
            lastPublish = millis();
             
            //Log.info( "voltage=%.2f", fuel.getVCell() );
            // Publish
            PublishToGoogleSheet();
            //reset counter after data is published
            RadarCnt = 0; 
            PIRCnt = 0;
            BeaconCnt = 0;
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

    snprintf(buf, sizeof(buf), "[%d,%d,%d]", RadarCnt,PIRCnt,BeaconCnt);

    Particle.publish(eventName, buf, PRIVATE);
    Log.info("published: %s", buf);
} 

//Function to handle DIO input
int CheckDIO(int signal) {
  int addcnt = 0;
  if (signal == HIGH) {               // check if the input is HIGH
         if (signal == LOW) {
           // we have just turned on 
           signal = HIGH;
           // add 1 motion count
           addcnt = 1; // we only count at the start of motion
         }
    } else {
          if (signal == HIGH){
           // we have just turned off
           signal = LOW;
         }
    }// end DIO logic
    // this will return 0 or 1, number of counts to add to counter
    return addcnt;
} 

//Google maps API function to find location with cell towers
//void locationCallback(float lat, float lon, float accuracy) {
  // Handle the returned location data for the device. This method is passed three arguments:
  // - Latitude
  // - Longitude
  // - Accuracy of estimated location (in meters)
//}