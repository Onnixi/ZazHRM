#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math.
#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library.   

#include <SoftwareSerial.h>

////////// Arduino variables //////////

const int led13 = 13;                 // The on-board Arduino LED, close to PIN 13.

const int rxPin = 10;                 // Using the software serial allows to keep on using the Arduino serial plotter in parallel.
const int txPin = 11;                 // 

SoftwareSerial softSerial(rxPin, txPin);

////////// PulseSensor variables ////////// 

const int pulseWire = 0;            // PulseSensor PURPLE WIRE connected to ANALOG PIN 0; actually, the long white one in this design.
const int pulseBpmLedPin = 7;       // ON: pulse mode, OFF: BPM mode
const int bmpIssueLedPin = 4;       // ON: when HB has troubled being acquired by the PulseSensor

int threshold = 550;                // Determine which Signal to "count as a beat" and which to ignore.
PulseSensorPlayground pulseSensor;  // Creates an instance of the PulseSensorPlayground object called "pulseSensor"
int skippedHb = 0;                  // Count the number of periods without having seen a heartbeat

////////// App variables ////////// 

const int pulseRate = 50;       
const int bpmRate = 500;
const int bmpAlertCode = 5;
int state = 0;
bool appReady = false;

////////// Bluetooth variables //////////

int communicationInterval = 500;    // Default is 500 ms for communicating the BPM, for the pulse it is 50 ms 
unsigned long timeNow = 0;

////////// Setup //////////

// May want to wrap in different object structures like in ZazouMon

void setup() {
    
  pinMode(pulseBpmLedPin, OUTPUT);
  digitalWrite(pulseBpmLedPin, LOW);
  pinMode(bmpIssueLedPin, OUTPUT);
  digitalWrite(bmpIssueLedPin, HIGH);
 
  Serial.begin(9600);                    // 
  softSerial.begin(9600);                // 

  // Configuring the PulseSensor object
  pulseSensor.analogInput(pulseWire);   
  pulseSensor.blinkOnPulse(led13);       // Auto-magically blink Arduino's LED with heartbeat
  pulseSensor.setThreshold(threshold);   
  
  if (!pulseSensor.begin()) {
    for(;;) {
      // Flash the led to show things didn't work.
      digitalWrite(led13, LOW);
      delay(50);
      digitalWrite(led13, HIGH);
      delay(50);
    } 
  }
}

/// sol 2 ///
bool comLock = false;
/////////////

void loop() {

  int bpm = pulseSensor.getBeatsPerMinute(); 
  int pulse = analogRead(pulseWire); 

  /// sol 2 ///
  int validBpm;
  
  if (pulseSensor.sawStartOfBeat() and !comLock) {          // 
    validBpm = bpm;                                         // 
    comLock = true;
  }
  /////////////

  if(softSerial.available() > 0) {
    state = softSerial.read();
  }

  switch(state) {
    case 'B':
      digitalWrite(pulseBpmLedPin, LOW);
      communicationInterval = bpmRate;  // 
      Serial.flush();            
      softSerial.flush();
      state = 0;
      break;
    case 'P': 
      digitalWrite(pulseBpmLedPin, HIGH);
      digitalWrite(bmpIssueLedPin, LOW);
      communicationInterval = pulseRate;
      state = 0;
    case 'S':
      appReady = true;
      break;
    case 'T':
      appReady = false;
      break;
  }

  if (millis() > timeNow + communicationInterval) {
    timeNow = millis();
    if (appReady) {
      
      ////////// Sending Bpm or Pulse, depending on chosen rate //////////
      if (communicationInterval == bpmRate) {
        skippedHb += 1;
        
        /// sol 2 ///
        if (comLock) {
          skippedHb = 0;
          if (validBpm > 255) { validBpm = 255; }
          Serial.println(validBpm);
          softSerial.write(validBpm);              
          digitalWrite(bmpIssueLedPin, LOW);
          comLock = false;
        }
        /////////////
        
        /// sol 1 ///
        //if (pulseSensor.sawStartOfBeat()) { 
        //  skippedHb = 0; 
        //  if (bpm > 255) { bpm = 255; }
        //  Serial.println(bpm);
        //  softSerial.write(bpm);              
        //  digitalWrite(bmpIssueLedPin, LOW); 
        //}
        ////////////

        if (skippedHb > 10) {
          Serial.println(bmpAlertCode);            
          softSerial.write(bmpAlertCode);          // Alerting App side, trigger #1.
                                                   // From experiments, triggers #2 and #3, in the App, seem seldom being called;
                                                   // the measurement discontinuities come mostly from trigger #1
          digitalWrite(bmpIssueLedPin, HIGH);      // Alerting Arduino side
        }
      }
      else if (communicationInterval == pulseRate) {    
        Serial.println(pulse);                          
        byte val = map(pulse,0,1023,0,255);           
        softSerial.write(val);
        digitalWrite(bmpIssueLedPin, LOW); 
      } 
    }
  }
//  
}
