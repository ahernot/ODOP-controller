#include <AccelStepper.h>

/* TODO
 *  Add end stops during movement
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
boolean isCalibrated; // store calibration bool value


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




// USE ONLY FOR CALIBRATION (in the dark)
//global variables:
int xDirection = 1;
boolean xLimMax = false;
boolean xLimMin  = false;

void updateLimits () {
  if (digitalRead(X_LIM) == 0) {
    if (xDirection == 1) {
      xLimMax = true;
      xLimMin = false;
    }
    else {
      xLimMax = false;
      xLimMin = true;
    }
  }
  
  else {
    xLimMax = false;
    xLimMin = false;
  }
}

boolean isCalibrated = false;
/*
 * FOR UNCALIBRATED DEVICE ONLY (isCalibrated == false)
 * before every movement: update xDirection
 * during every movement / every movement%10: call updateLimits()
 * 
 * FOR CALIBRATED DEVICE (isCalibrated == true)
 * digitalRead(X_LIM) == 0 && signe de la position absolue
 */



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





void moveRelative (String command) {
  Serial.println("RUNNING: " + command);
  
  // Flags
  boolean status_ = true;  // success (default value)
  boolean statusMessage = false;  // no status message sent

  // Read axis and value
  String subcommand = command.substring(6);
  float a = command.substring(8).toFloat();


  // Process command (x)
  if (subcommand.startsWith("x ")) {

    // Calculate steps
    float stepsTrue = a / X_DEG_PER_STEP * X_MICROSTEP * X_DISTANCE_PER_STEP * X_REDUCTION_RATIO;

    // Run command
    stepperX.move(stepsTrue);
    while (stepperX.distanceToGo() > 0) {
      //Serial.println(stepperX.distanceToGo() );
      stepperX.run();        
      if (digitalRead(X_LIM) == 0) {
        stepperX.move(0.);  // Reset distanceToGo
        
        printStatusStr("angle_x", "limit reached");  // Print status
        statusMessage = true;  // sent
        status_ = false;
        break;
      }  
    }

    // Print status
    if (statusMessage == false) {
      //printStatusBool("angle_x", status_);
      printStatusStr("angle_x", command); //temporary
      statusMessage = true;
    }
  }

  // Process command (y)
  else if (subcommand.startsWith("y ")) {

    // Calculate steps
    float stepsTrue = a / Y_DEG_PER_STEP * Y_MICROSTEP * Y_DISTANCE_PER_STEP * Y_REDUCTION_RATIO;

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






int i = 0;
void loop() {

  Serial.println(); Serial.println("========== NEW LOOP ==========");
   
  // ================================
  // Read messages from Serial
  while (Serial.available()) {
    delay(3);
    char c = Serial.read();  // Get one byte from serial buffer
    if (c == '\n') { break; }
    command += c;  // Build the string command
  }


  // ================================
  // Move to estimated zero position
  if (command.startsWith("estimate_zero")) {    
    moveRelative("angle x 90");
    delay(100);
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

    // Read axis and value
    String subcommand = command.substring(5);
    long a = command.substring(6).toInt();  // RIGHT?

    // Process command (x)
    if (subcommand.startsWith("x ")) {
      stepperX.move(10000);
      stepperX.runToPosition();
    }

    // Process command (y)
    else if (subcommand.startsWith("y ")) {
      
    }
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

  delay(5000);
  i++;
}
