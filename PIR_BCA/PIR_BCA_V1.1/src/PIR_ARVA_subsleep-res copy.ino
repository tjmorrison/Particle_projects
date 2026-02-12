/*
 * Project: PIR/Radar & Beacon Detection System
 * Description: Trail head monitoring system using PIR motion sensor and
 *              beacon detection on A4 (analog input). Logs detection data every 15 minutes,
 *              publishes when connected (typically hourly). Retains data if connection fails.
 *              Sends acknowledgment pulse on D4 when beacon is detected.
 * 
 * Author: Travis Morrison
 * Date: May 25, 2021 (Original)
 *       February 2, 2026 (Updated - Added resilient data storage for connectivity issues)
 * 
 * Hardware:
 * - PIR motion sensor connected to D2
 * 
 * Wiring:
 * - PIR: VIN (3V3 or VUSB), OUT (D2), GND
 * - Beacon Input: A4 (analog input, high signal when beacon detected)
 * - Beacon Acknowledgment Output: D4 (3.3V pulse when beacon detected)
 * - Battery Monitor: A3 (with 5:1 voltage divider)
 * 
 * 
 * Project To Do:
 * Priority:
 * - Add sleep interrupt if battery falls below threshold
 */

//======================================================================================================
// At the very top, after #define statements
#define DEBUG_MODE true  // Set to false for deployment

//Define I/O pins used
const pin_t Pin_PIR = D2;
const pin_t Pin_Beacon = A4;
const pin_t Pin_Beacon_Ack = D4;  // Acknowledgment pulse output
const pin_t Pin_Battery = A3;

// Analog threshold for beacon detection (12-bit ADC: 0-4095)
// ~2048 corresponds to ~1.65V (half of 3.3V reference)
#define BEACON_ANALOG_THRESHOLD 2048

//Define time intervals
#define COLLECTION_INTERVAL_MILLIS (15 * 60 * 1000)  // 15 minutes - data collection interval
#define MAX_INTERVALS 12  // Store up to 3 hours of data (12 x 15min)
#define TARGET_INTERVALS 4  // Try to publish every hour (4 x 15min)

//Define global variables
int PIR_cnt = 0; //PIR counter for current interval
int Beacon_cnt = 0; //Beacon counter for current interval
unsigned long intervalStartTime = 0; // Track when current collection interval started

// Arrays to store data - sized for 3 hours to handle connection issues
int PIR_intervals[MAX_INTERVALS];
int Beacon_intervals[MAX_INTERVALS];
String interval_timestamps[MAX_INTERVALS];

int current_interval = 0;  // Track which interval we're on (0-11)

int Batt_read = 0;
float Batt_volt = 0;

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000) //every day
#define MAX_CONNECTION_ATTEMPTS 3  // Try to connect up to 3 times
#define CONNECTION_TIMEOUT_MS 60000  // Wait up to 60 seconds for connection

unsigned long lastSync = millis();

SYSTEM_THREAD(ENABLED); //allows the code to run before connecting to the cloud and will run without cloud connection
SerialLogHandler logHandler;
SystemSleepConfiguration config;
FuelGauge fuel; //defines the fuel gauge class

// The event name to publish with has to be same as webhook
const char *eventName = "sheetTest1";

//Declaration of functions
bool PublishToGoogleSheet();
void SendBeaconAck();
void InitializeIntervals();

// setup() runs once, when the device is first turned on.
void setup() {

  //Set the digital pins to input/output
  pinMode(Pin_PIR, INPUT);
  pinMode(Pin_Beacon, INPUT);  // Analog pin, INPUT mode for analogRead
  pinMode(Pin_Beacon_Ack, OUTPUT);
  digitalWrite(Pin_Beacon_Ack, LOW); // Initialize to LOW
  
  // Enable automatic time sync
  Particle.syncTime();
  
  // Initialize interval arrays
  InitializeIntervals();

  // Initialize interval start time
  intervalStartTime = millis();

}



// Then in loop(), wrap the sleep command:
void loop() {

    #if !DEBUG_MODE  // Only sleep if NOT in debug mode
    // Sleep until 15-min interval ends OR wake on PIR trigger
    // PIR GPIO trigger wakes the device; beacon (analog A4) is polled each wake cycle
    unsigned long remainingTime = 0;
    unsigned long elapsed = millis() - intervalStartTime;
    if (elapsed < COLLECTION_INTERVAL_MILLIS) {
      remainingTime = COLLECTION_INTERVAL_MILLIS - elapsed;
    }

    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .flag(SystemSleepFlag::WAIT_CLOUD)
          .duration(remainingTime)
          .gpio(Pin_PIR, RISING);
    // Note: Beacon on A4 (analog) cannot use GPIO wake - polled on each wake cycle
    System.sleep(config);
    #else
    delay(5000);  // Just delay 5 seconds instead of sleeping
    #endif

    // Check for detections - PIR via GPIO wake, beacon via analog read
    if (digitalRead(Pin_PIR) == HIGH){
      PIR_cnt = PIR_cnt + 1;
      Log.info("PIR detected, count for interval: %d", PIR_cnt);
    }
    int beaconAnalogVal = analogRead(Pin_Beacon);
    if (beaconAnalogVal >= BEACON_ANALOG_THRESHOLD){
      Beacon_cnt = Beacon_cnt + 1;
      SendBeaconAck();
      Log.info("Beacon detected (analog: %d), count for interval: %d", beaconAnalogVal, Beacon_cnt);
    }

    // Check if the 15-minute collection interval has elapsed
    if (millis() - intervalStartTime >= COLLECTION_INTERVAL_MILLIS) {

      // Store data for this 15-minute interval (if we have space)
      if (current_interval < MAX_INTERVALS) {
        PIR_intervals[current_interval] = PIR_cnt;
        Beacon_intervals[current_interval] = Beacon_cnt;
        interval_timestamps[current_interval] = Time.format(Time.now(), "%Y-%m-%d %H:%M:%S");

        Log.info("Interval %d complete: PIR=%d, Beacon=%d at %s",
                 current_interval, PIR_cnt, Beacon_cnt,
                 interval_timestamps[current_interval].c_str());

        // Move to next interval
        current_interval++;
      } else {
        Log.warn("Max intervals reached! Data buffer full, attempting publish...");
      }

      // Reset counters for next interval
      PIR_cnt = 0;
      Beacon_cnt = 0;
      intervalStartTime = millis();
    }

    // Attempt to publish if we've reached target intervals OR if buffer is full
    if (current_interval >= TARGET_INTERVALS) {
      
      Log.info("Attempting to publish %d intervals...", current_interval);
      
      // Try to connect with timeout
      if(Particle.connected() == false) {
          Log.info("Not connected, attempting to connect...");
          Particle.connect();
          
          // Wait for connection with timeout
          unsigned long connectStart = millis();
          while(!Particle.connected() && (millis() - connectStart < CONNECTION_TIMEOUT_MS)) {
            delay(100);
          }
          
          if(!Particle.connected()) {
            Log.warn("Connection timeout after %d ms", CONNECTION_TIMEOUT_MS);
          }
      }
      
      // Only try to publish if connected
      if(Particle.connected()) {
        // Read battery voltage
        Batt_read = analogRead(Pin_Battery);
        Batt_volt = (Batt_read*3.3/4095.0)*5.0;
        
        // Attempt to publish
        bool publishSuccess = PublishToGoogleSheet();
        
        if(publishSuccess) {
          Log.info("Publish successful! Resetting intervals.");
          // Only reset after successful publish
          current_interval = 0;
          InitializeIntervals();
        } else {
          Log.warn("Publish failed! Retaining %d intervals for next attempt.", current_interval);
          // Don't reset - keep the data for next attempt
        }
        
        delay(500ms);
      } else {
        Log.warn("Not connected. Retaining %d intervals for next attempt.", current_interval);
        // Don't reset - keep the data for next attempt
      }
    }

    // Sync time 1x per day
    if(millis() - lastSync > ONE_DAY_MILLIS) {

      if(Particle.connected() == false) {
          Particle.connect();
          unsigned long connectStart = millis();
          while(!Particle.connected() && (millis() - connectStart < CONNECTION_TIMEOUT_MS)) {
            delay(100);
          }
      }

      if(Particle.connected()) {
        // Request time synchronization from the Particle Device Cloud
        Particle.syncTime();
        waitUntil(Particle.syncTimeDone);
        Particle.publishVitals();
        Log.info("Daily time sync completed");
        lastSync = millis();
      }
    }
}


// Initialize interval arrays to zero
void InitializeIntervals() {
    for(int i = 0; i < MAX_INTERVALS; i++) {
        PIR_intervals[i] = 0;
        Beacon_intervals[i] = 0;
        interval_timestamps[i] = "";
    }
}

//Publish to google sheets function - sends all stored intervals
bool PublishToGoogleSheet() {
    // Create JSON array with all interval data
    // Format: {"battery": 12.6, "intervals": [[time1,pir1,bcn1],[time2,pir2,bcn2],...]}
    
    String json = "{\"battery\":" + String(Batt_volt, 2) + ",\"intervals\":[";
    
    for(int i = 0; i < current_interval; i++) {
        json += "[\"" + interval_timestamps[i] + "\"," + 
                String(PIR_intervals[i]) + "," + 
                String(Beacon_intervals[i]) + "]";
        if(i < current_interval - 1) {
            json += ",";
        }
    }
    
    json += "]}";
    
    // Check if message is too large (max is 1024 bytes for Particle.publish)
    if(json.length() > 1000) {
        Log.error("Message too large (%d bytes)! Splitting not implemented.", json.length());
        // Could implement message splitting here if needed
        return false;
    }
    
    bool success = Particle.publish(eventName, json, PRIVATE);
    
    if(success) {
        Log.info("Published %d intervals: %s", current_interval, json.c_str());
    } else {
        Log.error("Particle.publish failed!");
    }
    
    return success;
}

// Send acknowledgment pulse on D4 when beacon is detected
void SendBeaconAck() {
    digitalWrite(Pin_Beacon_Ack, HIGH);  // Set D4 HIGH (3.3V)
    delay(100);                           // 100ms pulse duration
    digitalWrite(Pin_Beacon_Ack, LOW);   // Set D4 back to LOW
    Log.info("Beacon acknowledgment pulse sent on D4");
}