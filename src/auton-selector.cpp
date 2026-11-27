# include "auton-selector.h"
# include "Toggle.h"
# include "AIVision.h"
AutonWing selectedWing = AutonWing::Right;
bool redAlliance = true;

namespace {
void showSelection(bool confirmed = false){
    char text[21];
    snprintf(text, sizeof(text), "%s%s Wing %s", confirmed ? "Set " : "",
             selectedWing == AutonWing::Right ? "Right" : "Left",
             redAlliance ? "Red" : "Blue");
    Controller.Screen.setCursor(3,1);
    Controller.Screen.print("%-20s", text);
    printf("Auton selector: %s\n", text);
}
}

void selectAuton(){
    selectedWing = AutonWing::Right;
    showSelection();
    // A held while launching the program must not confirm the selection.
    while(Controller.ButtonA.pressing()){
        vex::wait(20,vex::msec);
    }
    while(Controller.ButtonA.pressing()==false){
        updateOpticalDisplay();
        updateAIVisionDisplay();
        bool selectionChanged = false;
        if(Controller.ButtonRight.pressing() && !Controller.ButtonLeft.pressing() &&
           selectedWing != AutonWing::Right){
            selectedWing = AutonWing::Right;
            selectionChanged = true;
        }
        else if(Controller.ButtonLeft.pressing() && !Controller.ButtonRight.pressing() &&
                selectedWing != AutonWing::Left){
            selectedWing = AutonWing::Left;
            selectionChanged = true;
        }
        // Up/Down select Red/Blue; shoulder buttons are unused during selection.
        if(Controller.ButtonUp.pressing() && !redAlliance){
            redAlliance = true;
            selectionChanged = true;
        }
        else if(Controller.ButtonDown.pressing() && redAlliance){
            redAlliance = false;
            selectionChanged = true;
        }
        if(selectionChanged){
            showSelection();
        }
        vex::wait(20,vex::msec);
    }
    showSelection(true);
    Controller.rumble(".");
}
