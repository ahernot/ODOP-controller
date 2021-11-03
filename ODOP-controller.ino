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


#define X_DEG_PER_STEP 1.8
#define X_DISTANCE_PER_STEP 4
#define X_MICROSTEP 8
#define X_REDUCTION_RATIO 27.


#define X_MAX_SPEED 500
#define X_ACCELERATION 500
#define Y_MAX_SPEED 500
#define Y_ACCELERATION 500

boolean motorsDisabled;
boolean isCalibrated; // store calibration bool value


AccelStepper stepperX(4, X_STP, X_DIR);
AccelStepper stepperY(4, Y_STP, Y_DIR);
String command = ""; 

void setup() {
  
  Serial.begin(9600);

  stepperX.setMaxSpeed(500); // 0-10000, good value 100 // s1000a100 too much
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



void updateStatus (String command, boolean status_) {
  if (status == true) {
    Serial.println(command + ": success");
  }
  else {
    Serial.println(command + ": failure");
  }
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


  // -------------------------------- TODO
  // Move (absolute motion)
  if (command.startsWith("move ")) {

    // Read axis and value
    String subcommand = command.substring(5)
    long a = command.substring(6).toInt();  // RIGHT?

    // Process command (x)
    if (subcommand.startsWith("x ") {
      stepperX.move(10000);
      stepperX.runToPosition();
    }

    // Process command (y)
    else if (subcommand.startsWith("y ") {
      
    }
  }



  // --------------------------------
  // Angle (relative motion)
  if (command.startsWith("angle ")) {

    // Read axis and value
    String subcommand = command.substring(5)
    float a = command.substring(6).toFloat();

    // Process command (x)
    if (subcommand.startsWith("x ") {
      float stepsTrue = a / X_DEG_PER_STEP * X_MICROSTEP * X_DISTANCE_PER_STEP * X_REDUCTION_RATIO;
      stepperX.move(stepsTrue);
      stepperX.runToPosition();
      Serial.println("finished");

      // updateStatus("angle_x", true, 
    }

    // Process command (y)
    else if (subcommand.startsWith("y ") {
      float stepsTrue = a / X_DEG_PER_STEP * X_MICROSTEP * X_DISTANCE_PER_STEP * X_REDUCTION_RATIO;
      stepperY.move(stepsTrue);
      stepperY.runToPosition();
      Serial.println("finished");
    }
  }
  
 
  // Reset command
  if (command != "") {
    command = ""; 
  }


  delay(5000);
  i++;
}
