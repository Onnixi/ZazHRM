/*
MIT License

Copyright (c) 2020 Alan Do

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate beat per minute (BPM) math.
#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library (https://pulsesensor.com/)  

#include <SoftwareSerial.h>            // Using software serial allows to keep on using the Arduino serial monitor and
                                       // plotter in parallel for debugging purposes.   

////////// Bluetooth variables //////////

int communicationInterval = 500;    // Default is every 500 ms for communicating the BPM, for the pulse it is every 50 ms. 
const int bluetoothPower = 2;       // Pin where to send 5V to the VCC pin of the HC-05 Bluetooth module.
unsigned long timeNow = 0;          // Timer used to send data to the App by interval.

const int rxPin = 10;               // Pin used to listen the App commands.
const int txPin = 11;               // Pin used to send BPM or pulse data to the App.

SoftwareSerial softSerial(rxPin, txPin);

////////// PulseSensor variables ////////// 

PulseSensorPlayground pulseSensor;  // Creates an instance of the PulseSensorPlayground object.
const int pulseWire = 0;            // PulseSensor long white wire connected to analog pin 0; the extension of the purple wire.
int threshold = 550;                // Determine which Signal to "count as a beat" and which to ignore; this should be be made eventually modifiable from the App. 

const int led13 = 13;               // The on-board Arduino LED, close to PIN 13; triggered when signal above threshold (550) 
const int okLedPin = 8;             // ON: no issue to report in BPM measurements; green color.
const int pulseBpmLedPin = 7;       // ON: pulse mode, OFF: BPM mode; yellow color.
const int bmpIssueLedPin = 4;       // ON: when BPM has troubled being acquired by the PulseSensor; red color.

int skippedHb = 0;                  // Count the number of periods without having seen a heartbeat; when above a certain threshold, an alert #1 is triggered.

////////// App variables //////////

const int pulseRate = 50;           // Pulse data will be transfered to App every 50 ms. 
const int bpmRate = 500;            // BPM data will be transfered to App every half second.

const int bpmAlertCode1 = 5;        // After the PulseSensor is failing to acquire the BPM for a time superior to bmpAlertDelay, a BPM of 5 is sent.
int bpmAlertDelay = 5000;           // Default bmpAlertDelay is 5000 ms, but can be changed from App.

int state = 0;                      // Used to communicate commands from the App to the Arduino side.
bool appReady = false;              // To know whether the App is ready to receive and display BMP or pulse data. 


////////// Setup //////////

void setup() {
    
  pinMode(okLedPin, OUTPUT);
  digitalWrite(okLedPin, LOW);
  pinMode(pulseBpmLedPin, OUTPUT);
  digitalWrite(pulseBpmLedPin, LOW);
  pinMode(bmpIssueLedPin, OUTPUT);
  digitalWrite(bmpIssueLedPin, HIGH);
    
  pinMode(bluetoothPower, OUTPUT);
  digitalWrite(bluetoothPower, HIGH);
 
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

bool comLock = false;
const int maxPulse = 1024;               // The maximal value that should be sent by the PulseSensor.

void loop() {

  int bpm = pulseSensor.getBeatsPerMinute(); 
  int pulse = analogRead(pulseWire);                        

  if (pulse > maxPulse) {
    Serial.println("********************* Pulse above 1024:");
    Serial.println(pulse);
    pulse = maxPulse;                    // Range should remain in [0, 1024], but just in case.
  }
  
  int validBpm;
  
  if (pulseSensor.sawStartOfBeat() and !comLock) {          // 
    validBpm = bpm;                                         // 
    comLock = true;
  }

  if(softSerial.available() > 0) {
    
    state = softSerial.read();
    bool switchHasState = false;
    
    switch(state) {
      case 'B':                             // The App has switched to BPM mode. 
        digitalWrite(pulseBpmLedPin, LOW);
        digitalWrite(okLedPin, HIGH);
        communicationInterval = bpmRate;
        switchHasState = true;
        break;
      case 'P':                             // The App has switched to pulse mode.
        digitalWrite(pulseBpmLedPin, HIGH);
        digitalWrite(bmpIssueLedPin, LOW);
        digitalWrite(okLedPin, LOW);
        communicationInterval = pulseRate;
        switchHasState = true;
        break;
      case 'S':                             // The App's "Start HRM" button was pressed, the App is ready to received data and log it.
        appReady = true;
        switchHasState = true;
        break;
      case 'T':                             // The App's "Stop HRM" button was pressed, the flow of data to the App is stopped.
        appReady = false;
        switchHasState = true;
        break;
      default:                              // The App has changed the delay before an alert in triggered, "Alert D" on the App's screen.
        if (state == 5 || 10 || 20 || 30 || 60) {
          bpmAlertDelay = state * 1000;
          switchHasState = true;  
        }
        break;
    }
    
    if (switchHasState == true) {
      Serial.println("********************* State has changed:");
      Serial.println(state);
      Serial.flush();            
      softSerial.flush();
      state = 0;
    }
  }

  if (millis() > timeNow + communicationInterval) {
    timeNow = millis();
    if (appReady) {
      
      ////////// Sending Bpm or Pulse, depending on chosen rate //////////
      if (communicationInterval == bpmRate) {
        skippedHb += 1;
        
        if (comLock) {
          skippedHb = 0;
          if (validBpm > 255) { validBpm = 255; }
          Serial.println(validBpm);
          softSerial.write(validBpm);              
          digitalWrite(bmpIssueLedPin, LOW);
          digitalWrite(okLedPin, HIGH);
          comLock = false;
        }

        if (skippedHb > bpmAlertDelay / bpmRate) { // With default bpmAlertDelay, corresponds to 10 skipped heart beat. 
          Serial.println(bpmAlertCode1);            
          softSerial.write(bpmAlertCode1);         // The Arduino Board is alerting the App side, bpmAlertCode1 is trigger #1.
                                                   // When BT or Arduino looses power, the App side has its own trigger, trigger #2.
          digitalWrite(bmpIssueLedPin, HIGH);      // Alerting Arduino side
          digitalWrite(okLedPin, LOW); 
        }
      }
      else if (communicationInterval == pulseRate) {    
        Serial.println(pulse);                          
        byte val = map(pulse,0,maxPulse,0,255);    //          
        softSerial.write(val);
        digitalWrite(bmpIssueLedPin, LOW);
        digitalWrite(okLedPin, LOW);  
      } 
    }
  }
  //delay(20); // Not sure this delay is usefull, may need to be evaluated
}
