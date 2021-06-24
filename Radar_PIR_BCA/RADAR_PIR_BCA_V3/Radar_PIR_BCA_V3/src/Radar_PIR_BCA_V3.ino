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
const pin_t Pin_Radar =  D2;
const pin_t Pin_PIR = D3;
const pin_t Pin_Beacon = D4;
const pin_t Pin_Battery = A0;

//Define global variables 
int Radar_cnt = 0; //Radar counter and so forth
int PIR_cnt = 0; //Radar counter and so forth
int Beacon_cnt = 0; //Radar counter and so forth

// Define time period to update time, 1x per day
#define WRITE_DATA_MILLIS (5 * 60 * 1000) //needs to be same as sleep interval
#define ONE_DAY_MILLIS ( 60 * 60 * 1000) //every hour
#define WAIT_FOR_CONNECTION_SECONDS 60s
unsigned long lastSync = millis();
unsigned long lastWrite = millis();

SYSTEM_THREAD(ENABLED); //allows the code to run before connecting to the cloud and will run without cloud conncetion
SerialLogHandler logHandler; //
SystemSleepConfiguration config;

// The event name to publish with has to be same as webhook
const char *eventName = "sheetTest1";
const char *RadarEvent = "RadarEvent";
//Decleration of functions~ tells the compiler which functions are available and how to use them
void PublishToGoogleSheet();

// setup() runs once, when the device is first turned on.
void setup() {
    
  //Make variable public

  //Set the digital pins to input
  pinMode(Pin_Radar, INPUT);
  pinMode(Pin_PIR, INPUT);
  pinMode(Pin_Beacon, INPUT);

  //Set battery volt monitor input
  pinMode(Pin_Battery, INPUT);
  
}

// main loop for code, runs continuously
void loop() {
    //make sure where connected before sleep
    if(Particle.connected() == false) {
          Particle.connect();
          waitUntil(Particle.connected);
    }
    // Sleep command until one of the signals gets a signal
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).network(NETWORK_INTERFACE_CELLULAR).duration(900s).gpio(Pin_Radar, RISING).gpio(Pin_PIR, RISING).gpio(Pin_Beacon, RISING); 
    System.sleep(config);

    
    if (digitalRead(Pin_Radar)== HIGH)
    {
      Radar_cnt= Radar_cnt+1;
      //debugging
      //String Radar_str = String::format("%d", Radar_cnt);
      //Particle.publish(RadarEvent, Radar_str);
      
    }
    if (digitalRead(Pin_PIR)== HIGH)
    {
      PIR_cnt= PIR_cnt+1;
    }
    if (digitalRead(Pin_Beacon)== HIGH)
    {
      Beacon_cnt= Beacon_cnt+1;
    }
     //delay(1s);
    //write to google sheets every time it wakes up from sleep routine and time 
    if (millis() - lastWrite > WRITE_DATA_MILLIS) {
        if(Particle.connected() == false) {
          Particle.connect();
          waitUntil(Particle.connected);
        }
        // Publish
        PublishToGoogleSheet();
        //char buf[128];

        //snprintf(buf, sizeof(buf), "[%d,%d,%d]", Radar_cnt, PIR_cnt, Beacon_cnt);

        //Particle.publish(eventName, buf, PRIVATE);
        //Log.info("published: %s", buf);
        //delay(5s); // delay 2 seconds to finish writing?

        //reset counter after data is published
        // only resets if board was connected~ perhaps may be good, but not telling if data was written
        Radar_cnt = 0; 
        PIR_cnt = 0; 
        Beacon_cnt = 0; 

        //reset last write
        lastWrite = millis();
    }

    // Synce time 1x per day
    if(Particle.connected() && millis() - lastSync > ONE_DAY_MILLIS) {

      if(Particle.connected() == false) {
          Particle.connect();
          waitUntil(Particle.connected);
      }

      // Request time synchronization from the Particle Device Cloud
      Particle.syncTime(); // sync time
      waitUntil(Particle.syncTimeDone);
      Particle.publishVitals(); // publish vitals 1x day
      lastSync = millis();//reset the lastsync
      
    }
    

}


//Publish to google sheets function
void PublishToGoogleSheet() {
    char buf[128];
    
    String Radar_str = String::format("%d", Radar_cnt);
    String PIR_str = String::format("%d", PIR_cnt);
    String Beacon_str = String::format("%d", Beacon_cnt);
    snprintf(buf, sizeof(buf), "[%d,%d,%d]", Radar_cnt, PIR_cnt, Beacon_cnt);

    Particle.publish(eventName, buf, PRIVATE);
    Log.info("published: %s", buf);
}