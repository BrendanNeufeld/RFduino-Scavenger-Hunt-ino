/*
 Copyright (c) 2013 OpenSourceRF.com.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
ProximityMovingAverage
by RFduino
12/6/2013

This sketch is for Bluetooth Low Energy 4 RSSI signal strength
investigation.

The sketch displays a row on the serial monitor for each RSSI 
sample containing:
- the current RSSI moving average
- an estimated distance in ft
- the intensity value used for the led

The distance is an estimate, and the accuracy will vary based
on the accuracy of the samples provided and the wireless
characteristics of your environment.  It will most likely only
be accurate to a few feet, as triangulation is not used.
However, it does provide some entertainment value.

To start the RSSI sampling, use any of the iPhone apps and
establish a connection with the sketch.

RSSI information is available to the RFduino after a connection
has been established.  The iPhone application is able to access
the RSSI value prior to connection if needed.
*/

#include <RFduinoBLE.h>

float average = 1.0;  // must be outside value range (-0dBm to -127dBm)

// number of samples in the average
int samples = 20;

// sample #1 from environment (rssi -45dBm = distance 1.5ft)
float r1 = -45;
float d1 = .5;
//loat d1 = 1.5;

// sample #2 from environment (rssi -75dBm = distance 20ft)
float r2 = -75;
float d2 = 20;

// constant to account for loss due to reflection, polzarization, etc
// n will be ~2 for an open space
// n will be ~3 for "typical" environment
// n will be ~4 for a cluttered industrial site
float n = (r2 - r1) / (10 * log10(d1 / d2));

float distance;

bool collecting = false;
float strength = 10;

// green led on the rgb shield
int ledStrength = 4;
// red
int led = 3;

void setup() {
  pinMode(led, OUTPUT);
  //pinMode(ledStrength, OUTPUT);
  
  Serial.begin(9600);
  
  // dump the value of n in case your curious
  Serial.println(n);
  
  RFduinoBLE.deviceName = "RFduino";
  // this is the data we want to appear in the advertisement
  // (if the deviceName and advertisementData are too long to fix into the 31 byte
  // ble advertisement packet, then the advertisementData is truncated first down to
  // a single byte, then it will truncate the deviceName)
  RFduinoBLE.advertisementData = "tester-2";
  RFduinoBLE.advertisementInterval = MILLISECONDS(300);
  
  // 128 bit base uuid
  // (generated with http://www.uuidgenerator.net)
  //RFduinoBLE.customUUID = "c97433f0-be8f-4dc8-b6f0-5343e6100eb4";
  
  // set the transmit power level in dBm (-20 dBm to +4 dBm)
  // (this will conserve battery life, and decrease the range
  // of the transmittion for near proxmity applications)
  //txPowerLevel values only : +4, 0, -4, -8, -12, -16 and -20 (multiple of 4).
  //--RFduinoBLE.txPowerLevel = -20;
  
  // start the BLE stack
  RFduinoBLE.begin();
  
  // stop the BLE stack
  //RFduinoBLE.end();
  
  // RFduino_pinWake(5, HIGH);
  //RFduino_resetPinWake(5);
  
  // do iBeacon advertising
  //RFduinoBLE.iBeacon = true;
}

void loop() {
  // switch to lower power mode
  //RFduino_ULPDelay(INFINITE);
  RFduino_ULPDelay( SECONDS(1) );
  //float temp = RFduino_temperature(CELSIUS);
  //Serial.println(collecting);
  if(collecting == true){
    if(strength <= 0){
      collecting = false;
      strength = 10;
      //RFduinoBLE.sendFloat(strength);
    }else{
      strength--;
      RFduinoBLE.sendFloat(strength);
    }
  }else{
    RFduinoBLE.sendFloat(distance); 
  }
  /*if(collecting == true){
    if(strength <= 0){
      collecting = false;
      
      //RFduinoBLE.sendFloat(strength);
    }
    RFduinoBLE.sendFloat(strength);
    
    strength--;
    
    int intensity = strength/10 * 255;
    analogWrite(ledStrength, intensity);
    
  }else{
    //Serial.println("Collecting:"+String(strength));
    //if(strength < 10){
      //strength++;
    //}else{
     //strength = 10; 
    //}
    strength = 10;
    RFduinoBLE.sendFloat(distance);
    analogWrite(ledStrength, HIGH);
  }*/
  
  
  
}

void RFduinoBLE_onConnect()
{
}

void RFduinoBLE_onDisconnect()
{
  digitalWrite(led, LOW);
  collecting = false;
}

// returns the dBm signal strength indicated by the receiver
// received signal strength indication (-0dBm to -127dBm)
void RFduinoBLE_onRSSI(int rssi)
{
  // initialize average with instaneous value to counteract the initial smooth rise
  if (average == 1.0)
    average = rssi;

  // single pole low pass recursive IIR filter
  average = rssi * (1.0 / samples) + average * (samples - 1.0) / samples;

  // approximate distance  
  distance = d1 * pow(10, (r1 - average) / (10 * n));

  // intensity (based on distance[linear] not rssi[logarithmic])
  float d = distance;
  if (d < d1)
    d = d1;
  else if (d > d2)
    d = d2;
  int intensity = (d2 - d) / (d2 - d1) * 255;
  analogWrite(led, intensity);

  /*
  if(distance > 20){
    Serial.println("out of range");
    RFduinoBLE.sendFloat(4);
  }else if(distance <= 20){
    Serial.println("far");
    RFduinoBLE.sendFloat(3);
  }else if(distance <= 10){
    Serial.println("close");
    RFduinoBLE.sendFloat(2);
  }else if(distance <= 2){
    Serial.println("you found it!");
    RFduinoBLE.sendFloat(1);
  }
  */
  //RFduinoBLE.sendFloat(distance);
  
  
  /*
  Serial.print((int)average);
  Serial.print("\t");
  Serial.print((int)distance);
  Serial.print("\t");
  Serial.print(intensity);
  Serial.println("");
  */
}

void RFduinoBLE_onAdvertisement(bool start)
{
  // turn the green led on if we start advertisement, and turn it
  // off if we stop advertisement
  /*
  if (start)
    digitalWrite(led, HIGH);
  else
    digitalWrite(led, LOW);
   */
}

void RFduinoBLE_onReceive(char *data, int len)
{
  Serial.println("RFduinoBLE_onReceive, data: " + String(data));
  if(String(data) == "collect"){
    collecting = true;
    Serial.println("Start collecting");
  }
}


//see timing critical for pausing loop while - RFduinoBLE.radioActiveo
// send the sample to the iPhone
  //RFduinoBLE.sendFloat(temp); - see temparure
  //RFduinoBLE.send(buf, 20) - see bulk data transfer
