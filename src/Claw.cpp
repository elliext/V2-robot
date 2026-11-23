#include "Claw.h"

using namespace vex;

namespace {
  Claw clawMechanism(claw);
}

Claw::Claw(vex::motor& motor) : motor_(motor) {}

void Claw::open() {
  motor_.spin(reverse, 80, vex::percent);
}

void Claw::close() {
  motor_.spin(vex::forward, 80, vex::percent);
}

void Claw::stop() {
  motor_.stop(vex::brake);
}

void clawOpen(){
  clawMechanism.open();
}
void manualClawOpen(){
  if (Controller.ButtonLeft.pressing()){
    claw.spin(reverse,90,percent);
  }
}
void clawClose(){
  clawMechanism.close();
}
void manualClawClose(){
  if (Controller.ButtonRight.pressing()){
    claw.spin(forward, 100, percent);
  }
}
void clawHold(){
  bool motorRunning = false;
  bool prevPressed = false;
  if(Controller.ButtonY.pressing()&&!prevPressed){
    motorRunning=!motorRunning;
    prevPressed=true;
  }
  if(motorRunning){
    claw.stop(brake);
  }
  else if (!Controller.ButtonY.pressing()&&prevPressed){
    prevPressed= false;
  }
}
void clawControll(bool resetHold){
  if(Controller.ButtonR2.pressing()){
    clawClose();
  }
  else if(Controller.ButtonR1.pressing()){
    clawOpen();
  }
  else if(Controller.ButtonLeft.pressing()){
    manualClawOpen();
  }
  else if(Controller.ButtonY.pressing()){
    clawHold();
  }
  else if(Controller.ButtonRight.pressing()){
    manualClawClose();
  }
  else{
    clawMechanism.stop();
  }
}

int clawTask(void* competition){
  auto& competitionState = *static_cast<vex::competition*>(competition);
  while(competitionState.isEnabled() && competitionState.isDriverControl()){
    clawControll();
    wait(20, msec);
  }
  return 0;
}
