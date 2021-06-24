/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/Radar_PIR_BCA_V1/src/Radar_PIR_BCA_V1.ino"
/*
 * Project Dist_Radar_V0
 * Description: This code is testing the RCWL 05516 distance Radar with the
 *  particle board for trail head detection system. 
 * Author: Travis Morrison
 * Date: April 6 2021 
 * 
 * Project to Do:
 * Prioity:
 * Need to have a catch error which won't reset counters if publishing fails
 * Add sleep
 * Add sleep interupt if battery falls below threshold
 * prototype enclosure - may not be nessecary with charge controller
 * backup data protection?
 * 
 *  Testing making comments for Github here
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
void CheckDIO(int signal_data[][3]);
#line 34 "c:/Users/tjmor/OneDrive/Documents/Particle_projects/Radar_PIR_BCA_V1/src/Radar_PIR_BCA_V1.ino"
#define Pin_Radar 2
#define Pin_PIR 3
#define Pin_Beacon 4



SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;//

// How often to publish a value
const std::chrono::milliseconds publishPeriod = 30s; //300s is every 5 min, 3600s is 1 hr

// The event name to publish with
const char *eventName = "sheetTest1";

unsigned long lastPublish;
int signal_data[3][3]={0}; // This will be the radar array cols =  [signal,state, counter] rows = [Radar, PIR, BCA]

char message = 'Apollo_1 has landed'; //Message which shows connection has been made


//Decleration of functions~ tells the compiler which functions are available and how to use them
void PublishToGoogleSheet();
void CheckDIO(int *signal);




// setup() runs once, when the device is first turned on.
void setup() {
    
  //Make variable public
  //Particle.variable("Signal_data_matri", signal_data);
 
  //Set the digital pins to input
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);

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



  // Read sesnor and store value
  signal_data[1][1] =  digitalRead(Pin_Radar);
  signal_data[2][1] =  digitalRead(Pin_PIR);
  signal_data[3][1] = digitalRead(Pin_Beacon);

  CheckDIO(signal_data);

  // Sleep command
  //SystemSleepConfiguration config;
  //config.mode(SystemSleepMode::STOP).duration(600s); 
  //System.sleep(config);

  //Use millis to write to google sheets every defined period
  if (millis() - lastPublish >= publishPeriod.count()) {
        if (Particle.connected()) {
            lastPublish = millis();
             
            //Log.info( "voltage=%.2f", fuel.getVCell() );
            // Publish
            PublishToGoogleSheet();

            //reset counter after data is published
            // Need to have a catch error which won't reset counters if publishing fails
            signal_data[1][3] = 0; 
            signal_data[2][3] = 0; 
            signal_data[3][3] = 0; 
        }
    }
     
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
        signal_data[ii][3] = signal_data[ii][3]+1; // we only count at the start of motion
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
