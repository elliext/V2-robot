# include "auton-selector.h"
# include "Toggle.h"
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
    Brain.Screen.clearScreen();
    Brain.Screen.setPenColor(vex::color::white);
    Brain.Screen.setFillColor(vex::color::black);
    Brain.Screen.printAt(10, 22, true, confirmed ? "Pre-auton confirmed" : "Pre-auton: choose wing and color");
    Brain.Screen.drawRectangle(10, 40, 220, 60);
    Brain.Screen.drawRectangle(250, 40, 220, 60);
    Brain.Screen.drawRectangle(10, 110, 220, 60);
    Brain.Screen.drawRectangle(250, 110, 220, 60);
    Brain.Screen.drawRectangle(10, 190, 460, 60);
    Brain.Screen.printAt(20, 75, true, "Left Wing%s",
                        selectedWing == AutonWing::Left ? " [selected]" : "");
    Brain.Screen.printAt(260, 75, true, "Right Wing%s",
                        selectedWing == AutonWing::Right ? " [selected]" : "");
    Brain.Screen.printAt(20, 145, true, "Red%s", redAlliance ? " [selected]" : "");
    Brain.Screen.printAt(260, 145, true, "Blue%s", redAlliance ? "" : " [selected]");
    Brain.Screen.printAt(20, 225, true, confirmed ? "Confirmed: %s Wing %s" : "Confirm (or controller A)",
                        selectedWing == AutonWing::Right ? "Right" : "Left",
                        redAlliance ? "Red" : "Blue");
    printf("Auton selector: %s\n", text);
}
}

void selectAuton(){
    selectedWing = AutonWing::Right;
    showSelection();
    // Require a fresh press/tap, not input held while launching the program.
    while(Controller.ButtonA.pressing() || Brain.Screen.pressing()){
        vex::wait(20,vex::msec);
    }
    bool touchHeld = false;
    while(true){
        if(Controller.ButtonA.pressing()){
            break;
        }
        updateOpticalDisplay();
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
        const bool touching = Brain.Screen.pressing();
        if(touching && !touchHeld){
            const int x = Brain.Screen.xPosition();
            const int y = Brain.Screen.yPosition();
            if(x>=10 && x<470 && y>=190 && y<250){
                break;
            }
            if(y>=40 && y<100){
                if(x>=10 && x<230 && selectedWing != AutonWing::Left){
                    selectedWing = AutonWing::Left;
                    selectionChanged = true;
                } else if(x>=250 && x<470 && selectedWing != AutonWing::Right){
                    selectedWing = AutonWing::Right;
                    selectionChanged = true;
                }
            } else if(y>=110 && y<170){
                if(x>=10 && x<230 && !redAlliance){
                    redAlliance = true;
                    selectionChanged = true;
                } else if(x>=250 && x<470 && redAlliance){
                    redAlliance = false;
                    selectionChanged = true;
                }
            }
        }
        touchHeld = touching;
        if(selectionChanged){
            showSelection();
        }
        vex::wait(20,vex::msec);
    }
    showSelection(true);
    Controller.rumble(".");
}
