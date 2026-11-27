# include "match-setup.h"
# include "Toggle.h"
# include "AIVision.h"
bool redAlliance = true;
bool isRight = false;

namespace {
void showSelection(bool confirmed = false){
    char text[21];
    snprintf(text, sizeof(text), "%s %s %s", confirmed ? "Set" : "Setup",
             redAlliance ? "Red" : "Blue", isRight ? "Right" : "Left");
    Controller.Screen.setCursor(3,1);
    Controller.Screen.print("%-20s", text);
    printf("Match setup: %s\n", text);
}
}

void selectMatchSetup(){
    showSelection();
    // A held while launching the program must not confirm the selection.
    while(Controller.ButtonA.pressing()){
        vex::wait(20,vex::msec);
    }
    while(Controller.ButtonA.pressing()==false){
        updateOpticalDisplay();
        updateAIVisionDisplay();
        bool selectionChanged = false;
        // Up/Down select Red/Blue; L1/R1 select Left/Right wing.
        if(Controller.ButtonUp.pressing() && !redAlliance){
            redAlliance = true;
            selectionChanged = true;
        }
        else if(Controller.ButtonDown.pressing() && redAlliance){
            redAlliance = false;
            selectionChanged = true;
        }
        if(Controller.ButtonL1.pressing() && isRight){
            isRight = false;
            selectionChanged = true;
        }
        else if(Controller.ButtonR1.pressing() && !isRight){
            isRight = true;
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
