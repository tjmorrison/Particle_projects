/*
 * Project Dist_Radar_V3
 * Description: This code is testing the RCWL 05516 distance Radar with interuptts the
 *  particle board for trail head detection system. 
 * Author: Travis Morrison
 * Date: May 25 2021 

 */

//======================================================================================================
//Define I/O pins used
const pin_t Pin_PIR = D2;
const pin_t Pin_Beacon = D3;
const pin_t Pin_Battery =  A3;

//Define global variables 
int PIR_cnt = 0; //PIR counter and so forth
int Beacon_cnt = 0; //BCA pin counter and so forth
int Batt_read = 0;
double Batt_volt = 0;  // Changed from float to double

// Monitoring variables
int signal_strength = 0;
int signal_quality = 0;
int wake_reason = 0; // 0=timer, 1=PIR, 2=Beacon, 3=both sensors
retained unsigned long last_publish_time = 0;

// Define time period to update time, 1x per day
#define WRITE_DATA_MILLIS (60 * 60 * 1000) //needs to be same as sleep interval
//#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000) //every day

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

  // Expose variables to Particle Console
  Particle.variable("PIR_count", PIR_cnt);
  Particle.variable("Beacon_count", Beacon_cnt);
  Particle.variable("Battery_V", Batt_volt);
  Particle.variable("Signal_Str", signal_strength);
  Particle.variable("Signal_Qual", signal_quality);
  Particle.variable("Wake_Reason", wake_reason);
  Particle.variable("Last_Publish", last_publish_time);

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

    // Determine wake reason and update counters
    wake_reason = 0;
    bool pir_triggered = digitalRead(Pin_PIR) == HIGH;
    bool beacon_triggered = digitalRead(Pin_Beacon) == HIGH;
    
    if (pir_triggered && beacon_triggered) {
      wake_reason = 3;
    } else if (pir_triggered) {
      wake_reason = 1;
    } else if (beacon_triggered) {
      wake_reason = 2;
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
        
        // Get signal strength
        CellularSignal sig = Cellular.RSSI();
        signal_strength = sig.getStrength();
        signal_quality = sig.getQuality();
       
        // Calls Publish to Google sheet function
        PublishToGoogleSheet();
        
        PIR_cnt = 0; 
        Beacon_cnt = 0; 

        delay(500ms);
        //reset last write
        lastWrite = millis();
    }

    // Sync time 1x per day
    /*if(millis() - lastSync > ONE_DAY_MILLIS) {

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
      
    }*/
    

}


//Publish to google sheets function
void PublishToGoogleSheet() {
    char buf[128];
    snprintf(buf, sizeof(buf),"[%d,%d,%.2f]", PIR_cnt, Beacon_cnt, Batt_volt);
    if (Particle.publish(eventName, buf, PRIVATE)) {
        last_publish_time = Time.now();
        Log.info("published: %s", buf);
    } else {
        Log.error("publish failed");
    }
}