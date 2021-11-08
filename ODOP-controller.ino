#include <AccelStepper.h>

/* TODO
 *  enable_motors
 *  disable_motors
 */

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


#define STATUS_VERBOSE true
#define HELP_VERBOSE true

// X-axis parameters
#define X_DEG_PER_STEP 1.8
#define X_DISTANCE_PER_STEP 4
#define X_MICROSTEP 8
#define X_REDUCTION_RATIO 27.

// Y-axis parameters
#define Y_DEG_PER_STEP 1.8
#define Y_DISTANCE_PER_STEP 4
#define Y_MICROSTEP 8
#define Y_REDUCTION_RATIO 27.

// Motor parameters
#define X_MAX_SPEED 500
#define X_ACCELERATION 500
#define Y_MAX_SPEED 500
#define Y_ACCELERATION 500

boolean motorsDisabled;
boolean isCalibrated = false; // store calibration bool value


// Only used for uncalibrated motion
boolean xLimMin = false;
boolean xLimMax = false;


AccelStepper stepperX(4, X_STP, X_DIR);
AccelStepper stepperY(4, Y_STP, Y_DIR);
String command = ""; 

void setup() {
  
  Serial.begin(9600);

  pinMode (X_LIM, INPUT);

  stepperX.setMaxSpeed(X_MAX_SPEED); // 0-10000, good value 100 // s1000a100 too much
  stepperX .setAcceleration(X_ACCELERATION); // 0-5000, good value 200

  stepperY.setMaxSpeed(Y_MAX_SPEED); // 0-10000, good value 100
  stepperY .setAcceleration(Y_ACCELERATION); // 0-5000, good value 200

}




float getCurrentPositionSteps(char axis) {
  if (axis == 'x') { return stepperX.currentPosition() / X_DISTANCE_PER_STEP; }
  else if (axis == 'y') { return stepperY.currentPosition() / Y_DISTANCE_PER_STEP; }
  else { Serial.println("error"); return 0.; }
}
float getCurrentPositionDeg(char axis) {
  if (axis == 'x') { return stepperX.currentPosition() / X_DISTANCE_PER_STEP / X_REDUCTION_RATIO / X_MICROSTEP * X_DEG_PER_STEP; }
  else if (axis == 'y') { return stepperY.currentPosition() / Y_DISTANCE_PER_STEP / Y_REDUCTION_RATIO / Y_MICROSTEP * Y_DEG_PER_STEP; }
  else { Serial.println("error"); return 0.; }
}

void printStatus () {

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

  Serial.print("Current positions: ");
  Serial.print("    X-axis (swing arm): "); Serial.print(getCurrentPositionSteps('x')); Serial.print(" steps / "); Serial.print(getCurrentPositionDeg('x')); Serial.println(" deg");
  Serial.print("    Y-axis (turntable): "); Serial.print(getCurrentPositionSteps('y')); Serial.print(" steps / "); Serial.print(getCurrentPositionDeg('y')); Serial.println(" deg");


  // reportLimits();
  Serial.println("Status ok");
}


void printStatusBool (String command, boolean status_) {
  if (STATUS_VERBOSE == true) {
    if (status_ == true) {
      Serial.println(command + ": success");
    }
    else {
      Serial.println(command + ": failure");
    }
  }
}
void printStatusStr (String command, String message) {
  if (STATUS_VERBOSE == true) {
    Serial.println(command + ": " + message);
  }
}
void printHelp (String helpMessage) {
  if (HELP_VERBOSE == true) {
    Serial.println("    " + helpMessage);
  }
}



void updateLimits () {
  if (digitalRead(X_LIM) == 0) {
    if (stepperX.distanceToGo() > 0) {
      xLimMin = false; xLimMax = true;
    }
    else if (stepperX.distanceToGo() < 0) {
      xLimMin = true; xLimMax = false;
    }
    else {
      xLimMin = false; xLimMax = false;
    }
  }
}


void moveRelative (String command) {
  
  // Flags
  boolean status_ = true;  // success (default value)
  boolean statusMessage = false;  // no status message sent

  // Read axis and value
  String subcommand = command.substring(6);
  float commandVal = command.substring(8).toFloat();

  // Process command (x)
  if (subcommand.startsWith("x ")) {
    
    float stepsTrue = commandVal / X_DEG_PER_STEP * X_MICROSTEP * X_DISTANCE_PER_STEP * X_REDUCTION_RATIO;

    // Give order
    stepperX.move(stepsTrue);

    int itersUnchecked = 1024;  // Number of consecutive run commands without end stop switch check
    // Run command
    while (stepperX.distanceToGo() != 0) {
      
      for (int i=0; i < itersUnchecked; i ++) {
        stepperX.run();
        if (stepperX.distanceToGo() == 0) { break; }
      }
      boolean requestStop = false;

      // Check end stop switch
      if (isCalibrated == true) {
        if (digitalRead(X_LIM) == 0) {
          if (stepperX.currentPosition() >= 40 && stepperX.distanceToGo() > 0) {  // +90 and increasing
              requestStop = true;
          }
          else if (stepperX.currentPosition() <= 40 && stepperX.distanceToGo() < 0) {  // -90 and decreasing
              requestStop = true;
          }
        }
      }
      else if (isCalibrated == false) {
        
        // update limits
        updateLimits();

        // update xLimMin = false; xLimMax = true; ??
        if (xLimMax == true && stepperX.distanceToGo() > 0) {  // max and increasing
          Serial.println("max and increasing, stopping");
          requestStop = true;
        }
        else if (xLimMin == true && stepperX.distanceToGo() < 0) {  // min and decreasing
          Serial.println("min and decreasing, stopping");
          requestStop = true;
        }
      }

      // Process stop request
      if (requestStop == true) {
        stepperX.move(0.);  // Reset distanceToGo
        printStatusStr("angle_x", "limit reached");  // Print status
        
        statusMessage = true;  // sent
        status_ = false;
        break;
      }
    }

    // Print status
    if (statusMessage == false) {
      printStatusBool("angle_x", status_);
      statusMessage = true;
    }
  }

  // Process command (y)
  else if (subcommand.startsWith("y ")) {

    // Calculate steps
    float stepsTrue = commandVal / Y_DEG_PER_STEP * Y_MICROSTEP * Y_DISTANCE_PER_STEP * Y_REDUCTION_RATIO;

    // Run command
    stepperY.move(stepsTrue);
    stepperY.runToPosition();

    // Print status
    if (statusMessage == false) {
      printStatusBool("angle_y", status_);
      statusMessage = true;
    }
  }

  // Unknown subcommand
  else {
    printStatusStr("unknown_command", "\"" + command + "\"");
    printHelp("Usage: \"angle [axis:x,y] [value]\"");
  }
}


void moveAbsolute () {
  
}



int i = 0;
void loop() {

  Serial.println(); Serial.println("========== NEW LOOP ==========");
  Serial.println("isCalibrated: " + String(isCalibrated));
  Serial.println("min=" + String(xLimMin) + ", max=" + String(xLimMax));
   
  // ================================
  // Read messages from Serial
  while (Serial.available()) {
    delay(3);
    char c = Serial.read();  // Get one byte from serial buffer
    if (c == '\n') { break; }
    command += c;  // Build the string command
  }

  if ((command != "") && (STATUS_VERBOSE == true)) {
    Serial.println("Processing command \"" + command + "\"");
  }

  // ================================
  // Move to estimated zero position
  if (command.startsWith("estimate_zero")) {    
    moveRelative("angle x -90");
    delay(1000);
    moveRelative("angle x 15");

    if (digitalRead(X_LIM) != 0) {
      printStatusBool ("estimate_zero", true);
    }
    else {
      printStatusBool ("estimate_zero", false);
    }
  }


  // ================================
  // Set current position as new absolute zero
  if (command.startsWith("set_zero")) {
    stepperX.setCurrentPosition(0);
    isCalibrated = true;
  }


  // ================================
  // Status
  if (command.startsWith("status")) {
    printStatus();
  }


  // ================================ TODO
  // Move (absolute motion)
  if (command.startsWith("move ")) {
    // moveAbsolute(command);
  }
    
  // ================================
  // Angle (relative motion)
  if (command.startsWith("angle ")) {
    moveRelative(command);
  }
  
 
  
  /*
  else {
    printStatusStr("unknown_command", "\"" + command + "\"");
    printHelp("Available commands: angle, move");
  }*/



  // ================================
  // Reset command variable
  if (command != "") {
    command = ""; 
  }

  delay(2000);
  i++;
}
