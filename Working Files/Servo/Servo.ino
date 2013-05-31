

// Sweep
// by BARRAGAN <http://barraganstudio.com> 
// This example code is in the public domain.

/********** SPECIFICATIONS ***********
*
*  Servo sets to 90 degrees, the center point
*  Flash LED with 0.5 second gaps for when everything is set
*  If there is a problem, repeatidely flash LED with 0.2 gaps

********** SERVO *****************
* RED pin goes to 5V
* BLACK pin goes to gnd
* YELLOW pin goes to pin 9

******** MAG *************
* GND pin to GND
* 3.3V to 3.3V
* SDA pin to A4
* SCL pin to A5
*/

#include <Servo.h> 
#include <Wire.h> //I2C Arduino Library
#include <HMC5883L.h>

#define address 0x1E //0011110b, I2C 7bit address of HMC5883

// Store our compass as a variable.
HMC5883L compass;
// Record any errors that may occur in the compass.
int error = 0;
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
 
int pos = 0;    // variable to store the servo position 
int ledPin = 13;  // Onboard LED pin 

int startPosition = 0;  // Wherever the compass is facing, that will be 0.

void setup() { 
  initializeMag(); // setup the magnometer
  
  initializeServo(); // setup the servo
  
  pinMode(ledPin, HIGH); // Configure LED pin as output
  
  /* Blink the LED 10 times, with 0.5 seconds between each gap  */
  for(int i = 0; i < 10; i++){ 
      digitalWrite(ledPin, HIGH);  // Turn LED on
      delay(250);                  // Wait 0.25 seconds
      digitalWrite(ledPin, LOW);  // Turn LED off
      delay(250);                 // Wait 0.25 seconds
  }
 
} 
 
void loop() { 
  readMag();
}

void readMag(){
  // Retrive the raw values from the compass (not scaled).
  MagnetometerRaw raw = compass.ReadRawAxis();
  // Retrived the scaled values from the compass (scaled to the configured scale).
  MagnetometerScaled scaled = compass.ReadScaledAxis();
  
  // Values are accessed like so:
  int MilliGauss_OnThe_XAxis = scaled.XAxis;// (or YAxis, or ZAxis)

  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float heading = atan2(scaled.YAxis, scaled.XAxis);
  
  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
  // Find yours here: http://www.magnetic-declination.com/
  // Mine is: 2� 37' W, which is 2.617 Degrees, or (which we need) 0.0456752665 radians, I will use 0.0457
  // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
  float declinationAngle = 0.0457;
  heading += declinationAngle;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;
   
  // Convert radians to degrees for readability.
  int headingDegrees = heading * 180/M_PI; 
  
  // Subtract the initial heading with our current one, seeing as the initial haeading is the direction we're facing.
  int gameDegrees = headingDegrees - startPosition;
  // If we're over 360 degrees, we're back to the start.
  if(gameDegrees < 0){
    gameDegrees = gameDegrees + 360;
  }
  
  
  // Split the compass into two halfs.
  if(gameDegrees > 180){
    if(gameDegrees < 270) { // If we're less then 270, restrict it to 0 (0 being out far left value for the servo)
      gameDegrees = 0;
    } else {
      gameDegrees = gameDegrees - 270;  // If we're more then 270, convert the value so it's less then 90 (so we can use it for the servo)
    }
  } else { // This is the other side of the compass, the right side
    if (gameDegrees > 90){
      gameDegrees = 170;  // We're more then 90, so restrict it to 180 for the servo, since that's far right
    } else {
      gameDegrees = gameDegrees + 90;  // We're less then 90, so make sure the values are between 90 and 180 (right side for the servo)
      if(gameDegrees > 170){
        gameDegrees = 170;  // When the servo reaches 170, it's 180 in real life. So restrict it.
      }  
    }
  }
  
  // move the servo to the degrees ranging from and including 0 to 180
  moveServo(gameDegrees);
  // Output the data via the serial port.
  Output(raw, scaled, heading, headingDegrees, gameDegrees);

  /* Normally we would delay the application by 66ms to allow the loop
  * to run at 15Hz (default bandwidth for the HMC5883L).
  * However since we have a long serial out (104ms at 9600) we will let
  * it run at its natural speed.
  */
   //delay(66);
  
}

/**
* Move the servo to the specific position.
* Param: int pos
* pos must range from and include, 0 to 180 degrees 
*/
void moveServo(int pos){ 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    //delay(15);                       // waits 15ms for the servo to reach the position 
   
}

// Output the data down the serial port.
void Output(MagnetometerRaw raw, MagnetometerScaled scaled, float heading, int headingDegrees, int gameDegrees)
{
   Serial.print("Raw:\t");
   Serial.print(raw.XAxis);
   Serial.print("   ");   
   Serial.print(raw.YAxis);
   Serial.print("   ");   
   Serial.print(raw.ZAxis);
   Serial.print("   \tScaled:\t");
   
   Serial.print(scaled.XAxis);
   Serial.print("   ");   
   Serial.print(scaled.YAxis);
   Serial.print("   ");   
   Serial.print(scaled.ZAxis);

   Serial.print("   \tHeading:\t");
   Serial.print(heading);
   Serial.print(" Radians   \t");
   Serial.print(headingDegrees);
   Serial.print(" Degrees   \t");
   
   Serial.print(gameDegrees);
   Serial.println(" Game Degrees   \t");
   
}

/**
* Setups up the magnometor  so that it's ready to run
*/
void initializeMag(){
 // Initialize the serial port.
  Serial.begin(9600);

  Serial.println("Starting the I2C interface.");
  Wire.begin(); // Start the I2C interface.

  Serial.println("Constructing new HMC5883L");
  compass = HMC5883L(); // Construct a new HMC5883 compass.
    
  Serial.println("Setting scale to +/- 1.9 Ga");
  error = compass.SetScale(1.9); // Set the scale of the compass.
  if(error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));
  
  Serial.println("Setting measurement mode to continous.");
  error = compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous
  if(error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));
    
  // Get the first compass value as that will be north for the game
  // Retrive the raw values from the compass (not scaled).
  MagnetometerRaw raw = compass.ReadRawAxis();
  // Retrived the scaled values from the compass (scaled to the configured scale).
  MagnetometerScaled scaled = compass.ReadScaledAxis();
  
  // Values are accessed like so:
  int MilliGauss_OnThe_XAxis = scaled.XAxis;// (or YAxis, or ZAxis)

  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float heading = atan2(scaled.YAxis, scaled.XAxis);
  
  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
  // Find yours here: http://www.magnetic-declination.com/
  // Mine is: 2� 37' W, which is 2.617 Degrees, or (which we need) 0.0456752665 radians, I will use 0.0457
  // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
  float declinationAngle = 0.0457;
  heading += declinationAngle;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;
   
  // Convert radians to degrees for readability.
  int headingDegrees = heading * 180/M_PI; 

  startPosition = headingDegrees;
  
  Serial.print("Start Position: ");
  Serial.println(startPosition);
  
}

/**
* Prepares the servo
*/
void initializeServo(){
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object 
  myservo.write(90);  // Make sure the servop is facing forward
}

