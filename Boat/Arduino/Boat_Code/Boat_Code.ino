#include "SPI.h"
#include "NRFLite.h"
#include <Servo.h>

//begin Radio Setup
const static uint8_t RADIO_ID = 1;       // Our radio's id.  The transmitter will send to this id.

struct RadioPacket // Any packet up to 32 bytes can be sent. Sets up what we want the radio to transmit everytime
{
    int8_t JoystickX;
    int8_t JoystickY;

    int8_t thrust;
    int8_t twist;

    bool keyEnabled;
    bool isAuton;
};

NRFLite radio;
RadioPacket radioData;
//End radio Setup

//Pins
  //Analog
    //In
      int leftAutonMotorPin = A0;
      int rightAutonMotorPin = A1;
      
    //Out

  //Digital
    //In

    //Out
      int LEDbuzzerPin = 2;
      int LEDgreenPin = 3;
      int LEDyellowPin = 4;
      int LEDredPin = 5;
      
      int leftMotorPin = 22;
                          
      int rightMotorPin = 24;
                          
      int pololuPin = 26;

    //Transceiver
      int CEpin = 49;
      int CSNpin = 53;
      int SCKpin = 52;
      int MOpin = 50;
      int MIpin = 51;
      
Servo pololu;

Servo leftMotor;
Servo rightMotor;

int connectivityTimer;

void setup() {
  digitalWrite(LEDgreenPin, HIGH);
  digitalWrite(LEDbuzzerPin, LOW);
  // put your setup code here, to run once:
  Serial.begin(115200);

    // By default, 'init' configures the radio to use a 2MBPS bitrate on channel 100 (channels 0-125 are valid).
    // Both the RX and TX radios must have the same bitrate and channel to communicate with each other.
    // You can run the 'ChannelScanner' example to help select the best channel for your environment.
    // You can assign a different bitrate and channel as shown below.
    //   _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS, 100) // THE DEFAULT
    //   _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE1MBPS, 75)
    //   _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS, 0)
    
    if (!radio.init(RADIO_ID, CEpin, CSNpin))
    {
        Serial.println("Cannot communicate with radio");
        while(!radio.init(RADIO_ID, CEpin, CSNpin)){//Checks if radio works and repeatedly prints it
          Serial.println("Cannot communicate with radio");
          delay(1000);
        } // Wait here forever.
    }

  analogReference(DEFAULT);

  pinMode(LEDbuzzerPin, OUTPUT);
  pinMode(LEDgreenPin, OUTPUT);
  pinMode(LEDyellowPin, OUTPUT);
  pinMode(LEDredPin, OUTPUT);

  pololu.attach(pololuPin);

  leftMotor.attach(leftMotorPin);
  rightMotor.attach(rightMotorPin);
}

bool isManual(){
  if(radioData.keyEnabled && !radioData.isAuton){
    Serial.print("_manual_");
    return true;
  }
  else{
    return false;
  }
}

bool isAutonomous(){
  if(radioData.keyEnabled && radioData.isAuton){
    Serial.print("_auton_");
    return true;
  }
  else{
    return false;
  }
}

void activatePololu(){
  Serial.println("enabled");
  pololu.writeMicroseconds(1750);
}

void deactivatePololu(){
  Serial.println("disabled");
  pololu.writeMicroseconds(0);
}

void setPercent(Servo motor, double percentage){
  if(percentage > 10 || percentage < -10){//Adds deadband so that when at rest it doesn't need to be perfect
    motor.writeMicroseconds((percentage * 400) + 1500);//Multiplies the given percentage by half of the ESCs PWM input range and adds 1500 
                                                       //so that the percentage corresponds to the correct PWM value
  }
  else{//Sets the motors to stationary
    motor.writeMicroseconds(1500);
  }
}

void tankSteering(){//manual controlled tank steering, one axis controls one motor each
  setPercent(leftMotor, radioData.thrust/255);
  setPercent(rightMotor, radioData.JoystickX/255);
}

void autonSteering(){//reads values from the Pi AI and sends it to the motors
  setPercent(leftMotor, analogRead(leftAutonMotorPin));
  setPercent(rightMotor, analogRead(rightAutonMotorPin));
  
}

bool isKeyOn(){//checks if the boat is safe to enable
  if(radioData.keyEnabled == HIGH && radio.hasData()){
    connectivityTimer = 0;
    return true;
  }
  else if(radio.hasData() && radioData.keyEnabled == LOW){
    connectivityTimer = 0;
    
  }
  else{
    if(connectivityTimer < 1300 && radioData.keyEnabled == HIGH){
      connectivityTimer++;
      return true;
    }
    else{
      return false;
    }
  }
}

void LEDactivation(int color){//Flashes LEDs/buzzer to show what state the boat is in

  switch(color)
  {
    case 0://For green
      digitalWrite(LEDgreenPin, HIGH);
      digitalWrite(LEDyellowPin, LOW);
      digitalWrite(LEDredPin, LOW);

      digitalWrite(LEDbuzzerPin, LOW);

      break;

    case 1://For yellow
      digitalWrite(LEDgreenPin, LOW);
      halfSecondOn(LEDyellowPin);
      digitalWrite(LEDredPin, LOW);

      digitalWrite(LEDbuzzerPin, LOW);

      break;

    case 2://For red
      digitalWrite(LEDgreenPin, LOW);
      digitalWrite(LEDyellowPin, LOW);
      halfSecondOn(LEDredPin);

      halfSecondOn(LEDbuzzerPin);

      break;

  }
}

void halfSecondOn(int pin){
  //Turns the pin to HIGH for half of a second
  if((millis() % 1000) > 500){//gets milliseconds,gets the remainder of dividing by 1000(for one second), if the upper half of 1000 activates LED/Buzzer
    digitalWrite(pin, HIGH);
  }
  else{
    digitalWrite(pin, LOW);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  if(isKeyOn()){
    
  while (radio.hasData()){//SAFETY: Only runs code when radio is connected

    radio.readData(&radioData); // Note how '&' must be placed in front of the variable name.

    if(isManual() & isKeyOn()){//Sets to manual mode

      activatePololu();

      LEDactivation(1);
      tankSteering();

    }
    else if(isAutonomous() & isKeyOn()){//sets to autonomous mode

      activatePololu();

      LEDactivation(2);
      autonSteering();

    }
    else{//Deactivates the ⛴ (boat)

      deactivatePololu();

      LEDactivation(0);

    }
  }}
  else{
    //Serial.print("noData__");
    deactivatePololu();
  }
  //if(radio.hasData()){
    //Serial.print("hasData");
  //}
 //if(!radio.hasData() /*&& isKeyOn()*/){
   // deactivatePololu();
    //Serial.println("no Data");
  //}
  
}