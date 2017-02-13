//XBEE 2.4GHZ Transmitter System For Delivering Location Relative Bearing in Degrees.
//Finalized by Jack Maydan based off Adam St. Amard's earlier versions.
//Edited by Robert Belter 10/30/2015
//Updated by Chase Pellazar 02/09/2017

//
//This program works based on the Spark Fun Arduino FIO v3.3 with an XBEE transmitter hooked to an extended antennae.
//The board also is hooked to a 3 axis magnetometer. 
//
//The entire module rotates, calculates the bearing based off magnetomer, 
//and transmits it through the patch antennae.
//
//This code is open source and free to use, its derivatives should follow the same guidelines.

#include <XBee.h>
#include <SparkFunLSM9DS1.h>

// SDO_XM and SDO_G are both pulled high, so our addresses are:
#define LSM9DS1_M  0x1E // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG  0x6B // Would be 0x6A if SDO_AG is LOW

//--------------------CALIBRATION FOR MAGNETOMETER---------------------
//In order to ensure that your transmitter will read the correct heading,
//we have provided a calibration mode that will print the values over the
//Xbees. Make sure to use with XB_RX_Calibration

//Uncomment the below line to activate print out over serial for magnetometer x,y,z
//#define calibration_mode

//Uncomment the below line to activate output above ^ over XBee 
//#define output_calibration

//declination to get a ore accurate heading. Calculate yours here:
// http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION 8.58  //Declination (degrees) in Boulder, CO.
//Axis offsets for magnetometer
int xoff = -0.135;
int yoff = 0.535;
int zoff = 0;   

//Axis scales for magnetometer
float xscale = 1.02173913;
float yscale = 0.97826087;
float zscale = 1;

#ifdef output_calibration
union{
  int i[3];
  uint8_t b[6];
}calibration_converter;
#endif

//Current readings from magnetometer axis
int xout, yout, zout; 

//-----------------------END CALIBRATION FOR MAGNETOMETER------------------

XBee xbee = XBee();
int compassAddress = 0x42 >> 1;

LSM9DS1 imu;

uint8_t payload[12];
int payload_size = 4;

union{
  float f;
  uint8_t b[4];
}heading_converter;

void setup(){
  Serial.begin(57600);
  Serial1.begin(57600);
  xbee.setSerial(Serial1);

  //set the sensor's communication mode and addresses
  imu.settings.device.commInterface = IMU_MODE_I2C; //Set mode to I2C
  imu.settings.device.mAddress = LSM9DS1_M; //Set mag address to 0x1E
  imu.settings.device.agAddress = LSM9DS1_AG; //Set ag address to 0x6B

  imu.begin();
  if (!imu.begin())
  {
    Serial.println("Failed to communicate with LSM9DS1.");
    Serial.println("Double-check wiring.");
    Serial.println("Default settings in this sketch will " \
                  "work for an out of the box LSM9DS1 " \
                  "Breakout, but may need to be modified " \
                  "if the board jumpers are.");
    while (1)
      ;
  }
  
}

void loop(){
  getVectorLSM();
  Serial.print("Theta: ");
  Serial.println(heading_converter.f, 2); // print the heading/bearing

  //Create payload
  makePayload();
  
  //Address of receiving device can be anything while in broadcasting mode
  Tx16Request tx = Tx16Request(0x5678, payload, payload_size);
  xbee.send(tx);
  
  //Delay must be longer than the readPacket timeout on the receiving module
  delay(10);
}

void makePayload(){
#ifndef output_calibration
  //Copy heading into payload
  memcpy(payload, heading_converter.b, 4);
#else
  //Use the calibration values as the output
  calibration_converter.i[0] = xout;
  calibration_converter.i[1] = yout;
  calibration_converter.i[2] = zout;
  memcpy(payload, calibration_converter.b, 12);
  payload_size = 12;
#endif
}


/*--------------------------------------------------------------
This the the fucntion which gathers the heading from the compass.
----------------------------------------------------------------*/
void getVectorLSM() {
  float heading = -1;
  float x,y,z;

  //Read in raw ADC magnetometer values in the x,y, and z directions from 9DoF stick sensor and convert to DPS values
  imu.readMag();
  x = imu.calcMag(imu.mx);
  y = imu.calcMag(imu.my);
  z = imu.calcMag(imu.mz);
  
  //Adjust values by offsets
  x += xoff;
  y += yoff;
  z += zoff;
  //Scale axes
  x *= xscale;
  y *= yscale;
  z *= zscale;

  //print out current magnetormeter readings
#ifdef calibration_mode
  xout = x;
  yout = y;
  zout = z;
  char output[100];
  sprintf(output, "x: %d, y: %d, z: %d", xout, yout, zout);
  Serial.println(output);
#endif

//calculate the heading
  if (y == 0)
    heading = (x < 0) ? PI : 0;
  else
    heading = atan2(y,x);
    
    heading -= DECLINATION * (PI/180);
  
  if (heading > 2*PI) heading -= (2 * PI);
  else if (heading < -PI) heading += (2 * PI);
  else if (heading < 0) heading += (2 * PI);
  
  // Convert everything from radians to degrees:
  heading *= (180.0 / PI);
  heading_converter.f = heading;
}
