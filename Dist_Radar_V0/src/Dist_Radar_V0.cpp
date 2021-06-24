/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/Dist_Radar_V0/src/Dist_Radar_V0.ino"
/*
 * Project Dist_Radar_V0
 * Description: This code is testing the RCWL 05516 distance Radar with the
 *  particle board. 
 * Author: Travis Morrison
 * Date: April 6 2021
 */
#include "Particle.h"
//#include "google-maps-device-locator.h"

void setup();
void loop();
void locationCallback(float lat, float lon, float accuracy);
#line 11 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/Dist_Radar_V0/src/Dist_Radar_V0.ino"
GoogleMapsDeviceLocator locator;

//Define I/O pins used
#define Pin_Radar 2

SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;

// How often to publish a value
const std::chrono::milliseconds publishPeriod = 30s;

// The event name to publish with
const char *eventName = "sheetTest1";

unsigned long lastPublish;
int IsMotion = 0;
char *message = 'Apollo_1 has landed';
String aString;

//google sheet publish function
void publishTest();

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  Particle.variable("IsMotion", IsMotion);
  if (Particle.variable("mess", message) == false)
  {
      // variable not registered!
  }
  Particle.variable("mess2", aString);
  
  //Set the digital pin 2 to input
  pinMode(D2, INPUT);


  // Scan for visible networks and publish to the cloud every 30 seconds
  // Pass the returned location to be handled by the locationCallback() method
  locator.withSubscribe(locationCallback).withLocatePeriodic(30);
  locator.withEventName("Apollo has landed");
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  IsMotion = digitalRead(Pin_Radar);
  if (millis() - lastPublish >= publishPeriod.count()) {
        if (Particle.connected()) {
            lastPublish = millis();
            publishTest();
        }
    }
  locator.loop();
  delay(1000);
}


void locationCallback(float lat, float lon, float accuracy) {
  // Handle the returned location data for the device. This method is passed three arguments:
  // - Latitude
  // - Longitude
  // - Accuracy of estimated location (in meters)
}

//Publish to google sheets function
void publishTest() {
    char buf[128];

    snprintf(buf, sizeof(buf), "[%d,%d]", IsMotion, rand());

    Particle.publish(eventName, buf, PRIVATE);
    Log.info("published: %s", buf);
}  