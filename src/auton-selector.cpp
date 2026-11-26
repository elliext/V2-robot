# include "auton-selector.h"
# include "Toggle.h"
# include "AIVision.h"
int autonNumber;
bool redAlliance = true;
bool isRight = false;

namespace {
void showSelection(){
    Controller.Screen.clearLine(3);
    Controller.Screen.setCursor(3,1);
    Controller.Screen.print("Auton %d %s %s", autonNumber,
                            redAlliance ? "Red" : "Blue",
                            isRight ? "Right" : "Left");
}
}

void selectAuton(){
    const int MAX_AUTON_NUMBER = 6;
    bool buttonHeld = false;
    autonNumber=1;
    showSelection();
    while(Controller.ButtonA.pressing()==false){
        updateOpticalDisplay();
        updateAIVisionDisplay();
        if(buttonHeld ==  false){
            if (Controller.ButtonRight.pressing()&& autonNumber < MAX_AUTON_NUMBER){
                autonNumber++;
                buttonHeld= true;
                showSelection();
            }
            else if (Controller.ButtonLeft.pressing() && autonNumber>0){
                autonNumber--;
                buttonHeld= true;
                showSelection();
            }
            
            
        }
        if(!Controller.ButtonLeft.pressing()&&!Controller.ButtonRight.pressing()){
            buttonHeld = false;
        }
        if(Controller.ButtonUp.pressing() && !redAlliance){
            redAlliance = true;
            showSelection();
        }
        else if(Controller.ButtonDown.pressing() && redAlliance){
            redAlliance = false;
            showSelection();
        }
        if(Controller.ButtonL1.pressing() && isRight){
            isRight = false;
            showSelection();
        }
        else if(Controller.ButtonR1.pressing() && !isRight){
            isRight = true;
            showSelection();
        }
        vex::wait(20,vex::msec);
    }
    Controller.Screen.clearLine(3);
    Controller.Screen.setCursor(3,1);
    Controller.Screen.print("Set %d %s %s", autonNumber,
                            redAlliance ? "Red" : "Blue",
                            isRight ? "Right" : "Left");
    for(int i=0; i<autonNumber; i++){
        Controller.rumble(".");
    }
}
