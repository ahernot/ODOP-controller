#include <AccelStepper.h>

//Direction pin
#define X_DIR       5 
#define Y_DIR       6
#define Z_DIR       7

// Step pin
#define X_STP       2
#define Y_STP       3 
#define Z_STP       4

// End stops (https://forum.arduino.cc/t/using-end-stops-on-cnc-shield-design-for-gbrl/510231/2)
#define X_LIM       9
#define Y_LIM       10
#define Z_LIM       11


boolean motorsDisabled;
boolean isCalibrated; // store calibration bool value


AccelStepper stepperX(4, X_STP, X_DIR);
AccelStepper stepperY(4, Y_STP, Y_DIR);
String command = ""; 

void setup() {
  
  Serial.begin(9600);

  stepperX.setMaxSpeed(100); // 0-10000, good value 100
  stepperX .setAcceleration(500); // 0-5000, good value 200

  stepperY.setMaxSpeed(100); // 0-10000, good value 100
  stepperY .setAcceleration(500); // 0-5000, good value 200

}


// Use byte?
int limCheck () {
  int xLim = digitalRead (X_LIM);
  return xLim;
}



// Calibration function (run on startup)
void calibrate (int maxIter) {
  
  int calStep = 24; // 3 steps
  int iter = 0;
  int xLim = limCheck();
  Serial.println(xLim);
  
  while (xLim != 0) {
    Serial.println(stepperX.currentPosition());
    stepperX.move(calStep);
    stepperX.run();
    xLim = limCheck();

    if (iter > maxIter) { break; }
    iter ++;
  }

  if (iter <= maxIter) {
    Serial.println("motion range start found");
    // Save angle now, equate to ANGLE_MIN_DEG
    
  } else {
    Serial.println("ERROR: exceeded iterations");
  }
  
}




void getStatus () {

  if (motorsDisabled) {
    Serial.println("Motors are disabled");
  } else {
    Serial.println("Motors are enabled");
  }
  
  if (isCalibrated) {
    Serial.println("System is calibrated");
  } else {
    Serial.println("System is not calibrated");
  }
  

  /*
  Serial.print("Home1 position (steps): ");
  Serial.println(home1Position);
  Serial.print("Home1 position (degrees): ");
  Serial.println(home1Position);
  Serial.print("Home2 position (steps): ");
  Serial.println(home2Position);
  Serial.print("Home2 position (degrees): ");
  Serial.println(ANGLE_RANGE);
  Serial.print("Current position (steps): ");
  Serial.println(stepper.currentPosition());
  Serial.print("Current position (degrees): ");
  float curPosSteps=(float)stepper.currentPosition();
  float curPosDeg=ANGLE_RANGE*(curPosSteps/(float)(home2Position-home1Position));
  Serial.println(curPosDeg);
  reportLimits();
  Serial.println("Status ok");
  */

  
}


int i = 0;
void loop() {

  Serial.println(); Serial.println("========== NEW LOOP ==========");
   
  // READ MESSAGES FROM SERIAL
  while (Serial.available()) {
    delay(3);
    char c = Serial.read();  //gets one byte from serial buffer
    if (c == '\n'){
      break;
    }
    command += c; //makes the string command
    //Serial.println(command);
  }
  
  // Hard-coded command (will be received from SERIAL)
  // String command = "calibrate"; // calibrate, step, …
  


  // Calibration
  if (command.startsWith("calibrate")) {
    calibrate(2000);
    isCalibrated = true;
  }

  // Status
  if (command.startsWith("status")) {
    getStatus();
  }

  // Move (absolute motion)
  if (readString.startsWith("move ")) {
      long a = readString.substring(5).toInt();
      // TODO
  }

  // Angle (relative motion)
  if (readString.startsWith("angle ")) {
    float a = readString.substring(5).toFloat();
    // TODO
  }
  
 
  // Reset command
  if (command != "") {
    command = ""; 
  }


  delay(5000);
  i++;
}
