


/* ODOP controller
 Copyright (C) 2021 Fusang Wang & Anatole Hernot, Mines Paris (PSL Research University). All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 github.com/Fusang-Wang
 github.com/ahernot
 */


#include <AccelStepper.h>
#include <SoftwareSerial.h>


/* TODO
 *  absolute motion
 *  enable_motors
 *  disable_motors
 *  y microstepping
 *  impedance calibration of drivers
 *  digicam control
 *  remove loop delay of 2s
 *  weld 5V connections to 
 *  
 *  redo limit checking in moveRelative using updateLimits() and xLimMin, xLimMax
 */

// Direction pins
#define X_DIR       5 
#define Y_DIR       6
// #define Z_DIR    7

// Step pins
#define X_STP       2
#define Y_STP       3 
// #define Z_STP    4

// End stop pins
#define X_LIM       9
#define Y_LIM       10
// #define Z_LIM    11

// X-axis (swing) parameters
#define X_DEG_PER_STEP 1.8
#define X_DISTANCE_PER_STEP 4
#define X_MICROSTEP 8
#define X_REDUCTION_RATIO 27.

// Y-axis (turntable) parameters
#define Y_DEG_PER_STEP 1.8
#define Y_DISTANCE_PER_STEP 4
#define Y_MICROSTEP 16
#define Y_REDUCTION_RATIO 1.

// Motor parameters
#define X_MAX_SPEED 500
#define X_ACCELERATION 500
#define Y_MAX_SPEED 250
#define Y_ACCELERATION 500

// Communication
#define VERSION_STRING "Version 0.17"
#define READY_MSG "Controller ready"
#define BAUD_RATE 9600  // 38400

boolean VERBOSE_STATUS = true;
boolean VERBOSE_DEBUG = false;
boolean VERBOSE_HELP = false;

// Global variables
String command;
boolean motorsDisabled;
boolean isCalibrated;
boolean xLimMin;  // X-axis minimum
boolean xLimMax;  // X-axis maximum

// Stepper motors
AccelStepper stepperX(4, X_STP, X_DIR);  // X-axis (swing)
AccelStepper stepperY(4, Y_STP, Y_DIR);  // Y-axis (turntable)




void setup() {

  // Begin serial communication
  Serial.begin(BAUD_RATE);
  Serial.println(VERSION_STRING);

  // Initialise limit pins
  pinMode (X_LIM, INPUT);  // minimum (X-axis)
  pinMode (Y_LIM, INPUT);  // maximum (X-axis)

  // Reset calibration flag
  isCalibrated = false;

  // X-axis (swing)
  stepperX.setMaxSpeed(X_MAX_SPEED);  // 0-10000, good value 100 // s1000a100 too much
  stepperX .setAcceleration(X_ACCELERATION);  // 0-5000, good value 200

  // Y-axis (turntable)
  stepperY.setMaxSpeed(Y_MAX_SPEED);  // 0-10000, good value 100
  stepperY .setAcceleration(Y_ACCELERATION);  // 0-5000, good value 200

  // Print status
  Serial.println(READY_MSG);
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


void printStatusBool (String command, boolean status_) {
  if (VERBOSE_STATUS == true) {
    if (status_ == true) {
      Serial.println(command + ": success");
    }
    else {
      Serial.println(command + ": failure");
    }
  }
}
void printStatusStr (String command, String message) {
  if (VERBOSE_STATUS == true) {
    Serial.println(command + ": " + message);
  }
}
void printHelp (String helpMessage) {
  if (VERBOSE_HELP == true) {
    Serial.println("    " + helpMessage);
  }
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

  if (isCalibrated) {
    Serial.println("Current positions: ");
    Serial.print("    X-axis (swing arm): "); Serial.print(getCurrentPositionSteps('x')); Serial.print(" steps / "); Serial.print(getCurrentPositionDeg('x')); Serial.println(" deg");
    Serial.print("    Y-axis (turntable): "); Serial.print(getCurrentPositionSteps('y')); Serial.print(" steps / "); Serial.print(getCurrentPositionDeg('y')); Serial.println(" deg");
  } else {
    Serial.println("Absolute positions unavailable");
  }


  // reportLimits();
  Serial.println("Status ok");
}



/**
 * Update global limit variables (function needs to be called to refresh xLimMin and xLimMax before they are used)
 */
void updateLimits () {
  // Minimum (0 for switch pressed)
  if (digitalRead(X_LIM) == 0) {
    xLimMin = true;
  } else {
    xLimMin = false;
  }

  // Maximum (0 for switch pressed)
  if (digitalRead(Y_LIM) == 0) {
    xLimMax = true;
  } else {
    xLimMax = false;
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

      /*
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
          //Serial.println("max and increasing, stopping");
          requestStop = true;
        }
        else if (xLimMin == true && stepperX.distanceToGo() < 0) {  // min and decreasing
          //Serial.println("min and decreasing, stopping");
          requestStop = true;
        }
      }
      */

      // Process stop request
      if (requestStop == true) {
        stepperX.move(0.);  // Reset distanceToGo
        //printStatusStr("angle_x", "limit reached");  // Print status
        
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


/*
 * Estimate zero for calibration
 */
void estimateZero () {
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
void setZero () {
  stepperX.setCurrentPosition(0);
    isCalibrated = true;
    printStatusBool ("set_zero", true);
}




void loop() {

  if (VERBOSE_DEBUG == true) {
    Serial.println(); Serial.println("========== NEW LOOP ==========");
    Serial.println("isCalibrated: " + String(isCalibrated));
    // Serial.println("min=" + String(xLimMin) + ", max=" + String(xLimMax));
  }
   
  // ================================
  // Read messages from Serial
  while (Serial.available()) {
    delay(3);
    char c = Serial.read();  // Get one byte from serial buffer
    if (c == '\n') { break; }
    command += c;  // Build the string command
  }
  if ((command != "") && (VERBOSE_DEBUG == true)) {
    //Serial.println("Processing command \"" + command + "\"");
  }

  // ================================
  // Status
  if (command.startsWith("status")) {
    printStatus();
  }

  // ================================
  // Angle (relative motion)
  else if (command.startsWith("angle")) {
    moveRelative(command);
  }

  // ================================ TODO
  // Move (absolute motion)
  else if (command.startsWith("move")) {
    // moveAbsolute(command);
  }

  // ================================
  // Move to estimated zero position
  else if (command.startsWith("estimate_zero")) {    
    estimateZero();
  }

  // ================================
  // Set current position as new absolute zero
  else if (command.startsWith("set_zero")) {
    setZero();
  }

  // ================================
  // Move to estimated zero position
  else if (command.startsWith("reset")) {
    setup();
    printStatusBool ("reset", true);
  }

  // ================================
  else if (command.startsWith("debug")) {
    if (VERBOSE_DEBUG) { VERBOSE_DEBUG = false; }
    else { VERBOSE_DEBUG = true; }
  }

  // ================================
  // Unrecognised command
  else if (command != "") {
    printStatusStr("Unknown command", "\"" + command + "\"");
    printHelp("Available commands: angle, move");
  }


  // ================================
  // Reset command variable
  if (command != "") {
    command = "";
  }
}
