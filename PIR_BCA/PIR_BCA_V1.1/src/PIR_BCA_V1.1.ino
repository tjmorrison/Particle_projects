/*
 * Project Dist_Radar_V3
 * Description: This code is deployment code for the PIR motion sensor with the ability to read in the BCA 12V ping for the
 * particle board for trail head detection system. 
 * Note: this code is designed to be powered from 3.7 V battery
 * Author: Travis Morrison
 * Date: June 24 2021 
 * 
 * Project to Do:
 * Priority:
 * Add sleep interupt if battery falls below threshold- may not be necessary with charge controller
 * 
 * Secondary
 * Write every hour on the hour?
 * 
 */

//======================================================================================================
//Define I/O pins used
const pin_t Pin_PIR = D2;
const pin_t Pin_Beacon = D3;


//Define global variables 
int PIR_cnt = 0; //PIR counter and so forth
int Beacon_cnt = 0; //BCA pin counter and so forth


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

      // Publish vitals from the Particle Device Cloud
      Particle.publishVitals(); // publish vitals 1x day
      lastSync = millis();//reset the lastsync
      
    }
    

}


//Publish to google sheets function
void PublishToGoogleSheet() {
    char buf[128];
    snprintf(buf, sizeof(buf),"[%d,%d,%.2f]", PIR_cnt, Beacon_cnt, fuel.getVCell());
    Particle.publish(eventName, buf, PRIVATE);
    Log.info("published: %s", buf);
}