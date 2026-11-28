#include "PID.h"
#include "vex.h"
#include "robot-config.h"
void drivePID(double target, double maxIntegral, double tolerance){
  
  //Initialize remaining variables
  double error = target;

  double derivative = 0;
  double integral = 0;
  double lastError = error;
  double total = 0;

  double kP = 3.4;
  double kI = 0.7;
  double kD = 0.008;
  const double GEAR_RATIO = (double) 5/3;
  const double WHEEL_CIRCUMFERENCE = 3.25* M_PI;
  LeftBack.setPosition(0,degrees);
  //Main loop, when the robot is more than the desired distance from the target
  while(fabs(error)>tolerance){

    //Calculate how far the robot is from the target
    // Positive drive commands use reverse motor rotation.
    double measuredValue = -LeftBack.position(degrees)/360 *
        WHEEL_CIRCUMFERENCE/GEAR_RATIO;
    error=target-measuredValue;
    //Find integral value
    integral+= error*15/1000;
    //Prevent integral windup by capping the maximum integral value
    if (integral>maxIntegral){
      integral=maxIntegral;
    }
    if(integral<-maxIntegral){
      integral=-maxIntegral;
    }
    //Find derivative value
    derivative=(error-lastError)/0.015;
    lastError=error;
    //Calculate the target speed for the drivetrain
    total=kP*error+kD*derivative+kI*integral;
    //Spin drivetrain at the target speed
    
    Drive.spin(reverse,total,percent);
    


    //Short wait to save resources
    wait(15, msec);
  }
  //Brake motors after driving is finished
  Drive.stop(brake);
  //Short delay to come to a full stop
  wait(50, msec);
}
void turnPID(double target, double maxIntegral, double tolerance, double timeoutMs){
  if(!isfinite(target) || !isfinite(maxIntegral) || maxIntegral<0 ||
     !isfinite(tolerance) || tolerance<=0){
    Drive.stop(brake);
    printf("Turn PID: target must be finite, integral cap nonnegative, and tolerance positive.\n");
    Controller.rumble("---");
    return;
  }
  if(!isfinite(timeoutMs) || timeoutMs<0){
    Drive.stop(brake);
    printf("Turn PID: timeout must be finite and nonnegative.\n");
    Controller.rumble("---");
    return;
  }
  if(!inertialSensor.installed() || inertialSensor.isCalibrating()){
    Drive.stop(brake);
    printf("Turn PID: inertial sensor is missing or still calibrating.\n");
    Controller.rumble("---");
    return;
  }
  
  double error = target;
  double integral = 0;
  double lastError = error;
  const double kP = 1.1;
  const double kI = 0.12;
  const double kD = 0.12;
  const double settleTimeMs = 150;
  const double settleSpeedDegreesPerSecond = 5;
  
  const bool timeoutEnabled = timeoutMs>0;
  timer turnTimer;
  inertialSensor.setRotation(0,degrees);
  turnTimer.reset();
  double previousTimeMs = 0;
  double settledSinceMs = -1;
  bool settled = false;
  bool sensorFault = false;
  while(true){
    if(!inertialSensor.installed() || inertialSensor.isCalibrating()){
      printf("Turn PID: inertial sensor became unavailable during the turn.\n");
      sensorFault = true;
      break;
    }
    const double measuredValue = inertialSensor.rotation();
    if(!isfinite(measuredValue)){
      printf("Turn PID: non-finite inertial rotation during the turn.\n");
      sensorFault = true;
      break;
    }
    const double elapsedMs = turnTimer.time(msec);
    const double dt = (elapsedMs-previousTimeMs)/1000.0;
    error=target-measuredValue;
    const double derivative = dt>0 ? (error-lastError)/dt : 0;
    const bool withinTolerance = fabs(error)<=tolerance;
    if(withinTolerance && fabs(derivative)<=settleSpeedDegreesPerSecond){
      if(settledSinceMs<0){
        settledSinceMs = elapsedMs;
      }
      if(elapsedMs-settledSinceMs>=settleTimeMs){
        settled = true;
        break;
      }
    } else {
      settledSinceMs = -1;
    }
    if(timeoutEnabled && elapsedMs>=timeoutMs){
      break;
    }
    if(withinTolerance){
      integral = 0;
      Drive.stop(brake);
    } else {
      if((error>0 && lastError<0) || (error<0 && lastError>0)){
        integral = 0;
      }
      const double candidateIntegral =
          fmax(-maxIntegral, fmin(maxIntegral, integral+error*dt));
      const double candidateOutput =
          kP*error+kD*derivative+kI*candidateIntegral;
      // Do not wind up the integral against an already saturated motor command.
      if(fabs(candidateOutput)<=100 || candidateOutput*error<0){
        integral = candidateIntegral;
      }
      const double total =
          fmax(-100, fmin(100, kP*error+kD*derivative+kI*integral));
      LeftDrive.spin(reverse,total,percent);
      RightDrive.spin(forward,total,percent);
    }
    lastError=error;
    previousTimeMs=elapsedMs;
    wait(timeoutEnabled ? fmax(0, fmin(15, timeoutMs-turnTimer.time(msec))) :
                          15, msec);
  }
  Drive.stop(brake);
  if(sensorFault){
    Controller.rumble("---");
  } else if(!settled){
    printf("Turn PID: timed out after %.2f ms; remaining error %.2f degrees; not settled.\n",
           turnTimer.time(msec), error);
    Controller.rumble("---");
  } else {
    printf("Turn PID: settled at %.2f degrees for target %.2f; error %.2f; elapsed %.2f ms.\n",
           target-error, target, error, turnTimer.time(msec));
  }
  wait(50, msec);
}
void armPID(double target, double maxIntegral, double tolerance){
  
  //Initialize remaining variables
  double error = target;

  double derivative = 0;
  double integral = 0;
  double lastError = error;
  double total = 0;

  double kP = 8.5;
  double kI = .01;
  double kD = 0.3;
  LeftBack.setPosition(0,degrees);
  //Main loop, when the robot is more than the desired distance from the target
  while(fabs(error)>tolerance){

    //Calculate how far the robot is from the target
    double measuredValue = LeftBack.position(degrees);
    error=target-measuredValue;
    //Find integral value
    integral+= error*15/1000;
    //Prevent integral windup by capping the maximum integral value
    if (integral>maxIntegral){
      integral=maxIntegral;
    }
    if(integral<-maxIntegral){
      integral=-maxIntegral;
    }
    //Find derivative value
    derivative=(error-lastError)/0.015;
    lastError=error;
    //Calculate the target speed for the drivetrain
    total=kP*error+kD*derivative+kI*integral;
    //Spin drivetrain at the target speed
    
    Arm.spin(forward,total,percent);
    
    


    //Short wait to save resources
    wait(15, msec);
  }
  //Brake motors after driving is finished
  Arm.stop(brake);
  
  //Short delay to come to a full stop
  wait(50, msec);
}
/// how PID works:
//PID is a control loop feedback mechanism that calculates an error value as the difference between a desired setpoint and a measured process variable. It uses three terms to adjust the output: Proportional (P), Integral (I), and Derivative (D).
// Proportional: The proportional term produces an output value that is proportional to the current error value. It is calculated by multiplying the error by a constant (kP). A larger kP will result in a larger output for a given error, which can help the system respond more quickly to changes, but can also lead to overshooting the target if too high.
// Integral: The integral term accounts for the accumulation of past errors. It is calculated by summing the error over time and multiplying it by a constant (kI). This term helps eliminate steady-state error, but can also lead to overshooting if too high.
// Derivative: The derivative term predicts future error based on the rate of change of the error. It is calculated by taking the difference between the current error and the previous error, divided by the time interval, and multiplying it by a constant (kD). This term helps reduce overshooting, but can also introduce noise if too high.