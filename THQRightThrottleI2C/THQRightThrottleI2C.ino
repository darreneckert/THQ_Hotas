#include <Wire.h>
#include <Joystick.h>

#define DEBUGTHQ TRUE
#define HID_ID    0x26          // Unique HID Device ID, randomly chosen

// Slew sensor definitions
volatile byte SLEW_ADR = 0x41;  // I2C Addrewss of the warthog slew sensor
volatile byte CTR_REG1 = 0x0F;  // Control Register 1
volatile byte CTR_REGT = 0x2D;  // Timing Control Register
volatile byte CTR_REG2 = 0x2E;  // Control Register 2
volatile byte ID_CODE  = 0x0C;  // 
volatile byte X_REG    = 0x10;  // X Register
volatile byte Y_REG    = 0x11;  // Y Register
volatile byte XP_REG   = 0x12;  // Positive X Reg
volatile byte XN_REG   = 0x13;  // Negative X Reg
volatile byte YP_REG   = 0x14;  // Positive Y Reg
volatile byte YN_REG   = 0x15;  // Negative Y Reg

const int uBtns    = 9;         // Number of buttons on the device
const int hBtns    = 4;         // Hat switches (U, D, L, R)
const int dTime    = 100;       // Delay time between loops
int i;                          // Generic counter variable

int xSlew = 0;                  // Slew X-Axis value;
int ySlew = 0;                  // Slew Y-Axis value;
int sBtn_CurrState;             // Current state of the button
int sHat_CurrState;             // or hat direction being checked
int sBtn_LastState[uBtns];      // The last recorded state of each button
int sHat_LastState[hBtns];      // and hat directions
int sBtn[uBtns] = {             // Digital pins assigned to each button
  4,                            // Slew push
  5, 6,                         // Speedbrake forward and back
  7, 8, 9,                      // Comm 1, Comm 2 and push
  10, 16,                       // Boat forward and back
  A2                            // Hat push
};
int sHat[hBtns] = {14, 15, A0, A1};       // Digital pins for the 5 way hat. Up, down, left, right
bool hVal_Changed = false;  
byte slewRead;

// Joystick (HID ID, Type, Buttons, Hats, X-Aix, Y-Axis, Z-Axis, Rx-Axis, Ry-Axis, Rz-Axis, Rudder, Throttle, Accelerator, Brake, Steering);
Joystick_ THQ(HID_ID, JOYSTICK_TYPE_JOYSTICK, uBtns, 1, true, true, false, false, false, false, false, false, false, false, false);
const bool initAutoSendState = true;


char xZero = 0;
char yZero = 0;

void setup() {
  // Wait before attempting to initialise sensor
  delay(250);  
  
  // Initialise the I2C connection
  Serial.begin(9600);
  while (!Serial) {;}
  Serial.println("Begining Setup ... ");
  Wire.begin();

  SlewInit();
  SlewSetup(xZero, yZero, 25, 25);

  // Initialise the pin mode, pullup resistors and the last known state of each button and hat direction
  for (i=0; i<uBtns; i++) {
    pinMode(sBtn[i], INPUT_PULLUP);
    digitalWrite(sBtn[i], HIGH);
    sBtn_LastState[i] = 0;
  }
  for (i=0; i< hBtns; i++) {
    pinMode(sHat[i], INPUT_PULLUP);
    digitalWrite(sHat[i], HIGH);
    sHat_LastState[i] = 0;
  }

  THQ.begin(initAutoSendState);

}

void loop() {
  // Read X-Axis and Y-Axis values from slew via I2C
  //ReadXAxis();
  //ReadYAxis();

  // Check for change in state of each hat direction and if it has changed send new state to HID
  for (i=0; i<hBtns; i++)  {
    sHat_CurrState = !digitalRead(sHat[i]);
    if (sHat_CurrState != sHat_LastState[i]) {
      hVal_Changed = true;
      sHat_LastState[i] = sHat_CurrState;
    }
  }

  // Something has changed since last check
  if (hVal_Changed) {
    // No switch is currently active, release hat switch
    if ((sHat_LastState[0] == 0) && 
        (sHat_LastState[1] == 0) &&
        (sHat_LastState[2] == 0) &&
        (sHat_LastState[3] == 0)) {
          THQ.setHatSwitch(0, -1);
        }

    // Up Switch pressed
    if (sHat_LastState[0] == 1) {
      THQ.setHatSwitch(0, 0);
      #ifdef DEBUGTHQ 
        Serial.println("POV Up pressed!"); 
      #endif
    }
    // Right Switch pressed
    if (sHat_LastState[1] == 1) {
      THQ.setHatSwitch(0, 90);
       #ifdef DEBUGTHQ 
        Serial.println("POV Right pressed!"); 
      #endif
    }
    // Down Switch pressed
    if (sHat_LastState[2] == 1) {
      THQ.setHatSwitch(0, 180);
       #ifdef DEBUGTHQ 
        Serial.println("POV Down pressed!"); 
      #endif
    }
    // Left Switch pressed
    if (sHat_LastState[3] == 1) {
      THQ.setHatSwitch(0, 270);
       #ifdef DEBUGTHQ 
        Serial.println("POV Left pressed!"); 
      #endif
    }

  }

  // Check for change in state of each button and if it has changed send new state to HID
  for (i=0; i<uBtns; i++) {
    sBtn_CurrState = !digitalRead(sBtn[i]);
    if (sBtn_CurrState != sBtn_LastState[i]) {
      THQ.setButton(i, sBtn_CurrState);
      sBtn_LastState[i] = sBtn_CurrState;  
      #ifdef DEBUGTHQ
        if (sBtn_CurrState == HIGH) {
          Serial.print("Button ");
          Serial.print(i);
          Serial.println(" pressed");
        }
        if (sBtn_CurrState == LOW) {
          Serial.print("Button ");
          Serial.print(i);
          Serial.println(" released");
        }
      #endif
    }
  }

  delay (dTime);
}

void SlewInit() {
  #ifdef DEBUGTHQ 
    Serial.print("Resetting ... ");
  #endif

  // Initiate Soft Reset
  WriteI2CReg(SLEW_ADR, CTR_REG1, 0x02);
  WriteI2CReg(SLEW_ADR, CTR_REG1);

  byte Reset_Status = 0;
  while (Reset_Status != 0xF1) {
    Reset_Status = ReadI2CReg(SLEW_ADR, CTR_REG1);
  }
  Serial.print("Got code 0x");
  Serial.print(Reset_Status, HEX);
  Serial.println(" Done");


  //WriteI2CReg(SLEW_ADR, CTR_REGT, 0x06);

  char X, Y;                
  char xTemp=0, yTemp=0;
  byte temp;
  ReadXAxis();  
  ReadYAxis(); 
  delay(5);
  for (byte i = 0; i<4; i++)
  {
    xTemp += ReadXAxis();
    yTemp += ReadYAxis();
    delay(5);
  }
  xZero = xTemp/4;
  yZero = yTemp/4;

  Serial.print("xZero = ");
  Serial.println(xZero, DEC);

  Serial.print("yZero = ");
  Serial.println(yZero, DEC);

  //temp = ReadI2CReg(SLEW_ADR, ID_CODE);

  //if ( temp != 0x0C) {
  //  Serial.print("Slew sensor failed to respond. got value: ");
  //  Serial.println(temp, HEX);
  //}
}

void SlewSetup(char xNull, char yNull, byte xDelta, byte yDelta) {
  char xp, xn, yp, yn;
  xp = xNull + xDelta;
  xn = xNull - xDelta;
  yp = yNull + yDelta;
  yn = yNull - yDelta;
  WriteI2CReg(SLEW_ADR, XP_REG, xp);
  WriteI2CReg(SLEW_ADR, XN_REG, xn);
  WriteI2CReg(SLEW_ADR, YP_REG, yp);
  WriteI2CReg(SLEW_ADR, YN_REG, yn);

  Serial.print("XPos = ");
  Serial.println(xp ,DEC);

  Serial.print("XNeg = ");
  Serial.println(xn ,DEC);

  Serial.print("YPos = ");
  Serial.println(yp ,DEC);

  Serial.print("YNeg = ");
  Serial.println(yn ,DEC);

  byte temp = ReadI2CReg(SLEW_ADR, CTR_REG1);
  temp = temp & 0b11110111;
  temp = temp & 0b01111111;
  temp = temp & 0b10001111;
  temp = temp | 0b01010000;
  temp = temp | 0b00000100;
  WriteI2CReg(SLEW_ADR, CTR_REG1, temp);

  WriteI2CReg(SLEW_ADR, CTR_REG2, 0x86);
  WriteI2CReg(SLEW_ADR, CTR_REG1, 0x80);

  Serial.println("Setup Done.");
}

char ReadI2CReg(byte slave_adr, byte reg) {
  byte temp;
  
  Wire.beginTransmission(slave_adr);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(slave_adr, 1, 1);
  temp = Wire.read();
  /* Serial.print("Communicating with device ");
  Serial.print(slave_adr, HEX);
  Serial.print(", reading ");
  Serial.print(temp, HEX);
  Serial.print(" from register ");
  Serial.println(reg, HEX);*/
  return temp;
}

void WriteI2CReg(byte slave_adr, byte reg) {
 byte error;
 
  Wire.beginTransmission(slave_adr);
  Wire.write(reg);
  error = Wire.endTransmission();
  if (error) {
    Serial.print("WriteI2C :: Error code: ");
    Serial.println(error, HEX);
  }
}

void WriteI2CReg(byte slave_adr, byte reg, byte buf) {
  byte error;
 
  Wire.beginTransmission(slave_adr);
  Wire.write(reg);
  Wire.write(buf);
  error = Wire.endTransmission();
  if (error) {
    Serial.print("WriteI2C :: Error code: ");
    Serial.println(error, HEX);
  }
}

char ReadXAxis(){
  return ReadI2CReg(SLEW_ADR, X_REG);
}

char ReadYAxis(){
  return ReadI2CReg(SLEW_ADR, Y_REG);
}