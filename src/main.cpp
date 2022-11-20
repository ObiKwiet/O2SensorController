#include "Arduino.h"
#include <math.h>

#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;


void ClearBufferS1();

int ParseMOXY( char response[], double *pressure);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }

  Serial1.begin(19200);
  Serial1.setTimeout(500);

  dac.begin(0x62);

}

void loop() {

  /****************
   * Arduino runs the setup() function and then loop(), which, uh, loops. Setup has global scope for 
   * objects but not variables? I think they expect you to just declare everything in
   * global scope which is bad practice. I don't know but it's kind of dumb. You want a loop in your main function, 
   * but you don't want to declare variables in a loop. So I just declare the local variables
   * and then nest a loop after that. 
   */
   
  char serial1InputBuffer[256] = {0};
  int serial1MessageSize;  //Change to size_t?
  int i;
  int err, dacIntVal;
  double pressure;
  int dacOut;
  
  while(1) {
    

    /*
    Serial.print("Beginning logo...");
    delay(5000); 
    Serial1.write( "#LOGO" );
    Serial1.write( "\r\n" ); */
    
    ClearBufferS1();
  
    delay(500);
  
    Serial.print("Reading O2...");
    
    Serial1.write( "#MOXY" );
    Serial1.write( "\r\n" );
    
    serial1MessageSize = Serial1.readBytesUntil('\r', serial1InputBuffer, 255); //Read serial buffer until CR charater detected. 

    ClearBufferS1();
  
    Serial.write(serial1InputBuffer, serial1MessageSize);
    Serial.print("\r\n");

    err = ParseMOXY(serial1InputBuffer, &pressure);

    if (err == 0) {
      Serial.print("Partial Pressure:");
      Serial.print(pressure, 8);
      Serial.print("\r\n");

    } else {
      Serial.print("MOXY ERROR:");
      Serial.print(err, DEC);
      Serial.print("\r\n");
    }
  
    for(i=0;i<255;i++) {
      serial1InputBuffer[i] = 0;
      
    }

    dacIntVal = int((63 * pressure) / (0.845 * 0.0909));
    dacOut = int((63 * pressure) / (0.845 * 0.0909));  

    /*
    63mV is the expected output value for PPO2 of 1. 
    pressure is the PPO2 measured.
    .845mV is the voltage step for DAC voltage rail of 3.46V
    0.0909 is the output voltage divider. 
    */

    Serial.print("DAC Level:");
    Serial.print(dacIntVal, DEC);
    Serial.print("\r\n");


    dac.setVoltage(dacOut, false);

  }

}


void ClearBufferS1() {

  while (Serial1.available() > 0) {   
  Serial1.read();
  }

}

int ParseMOXY( char response[], double *pressure) {

  int exponant = 0;
  int stringIdx;
  int stringNum;

  // Check for "#MOXY" header

  char handShake[7] = "#MOXY ";

  for(stringIdx = 0; stringIdx < 6; stringIdx++) {
    
    if (handShake[stringIdx] != response[stringIdx]) {
      return -1; //Error, no command echo for #MOXY
    } 

  }

  // Parse PPO2

  if ( response[6] == '-')
    return -2; //Error, PPO2 should not be negative.

  // Find end of PPO2 string
  while (stringIdx <= 19) {   // #MOXY and space are six places, plus a maximum of 12 for the PPO2 number
    if(response[stringIdx] == ' ')
       break;
    stringIdx++;
  }

  if (response[stringIdx] != ' ')
    return -3; //Error, no space found at end of number.

  stringIdx--;
  *pressure = 0;

  // Accumulate digits into number back to front.
  for ( ; stringIdx >= 6; stringIdx--) {

      if( ('0' <= response[stringIdx]) && (response[stringIdx] <= '9')) {
        stringNum = response[stringIdx] - '0';
        *pressure = *pressure + stringNum * pow( 10.0, exponant++);
      } else {
        return -4; // Error, all characters should be numbers.
      }

    }

  *pressure = *pressure / 1000; // convert to hPa. 

  *pressure = *pressure / 1013; // convert to PPO2 fraction. 

  return 0; //No errors

}
  
  
