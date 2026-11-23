# include "auton-selector.h"
int autonNumber;
void selectAuton(){
    const int MAX_AUTON_NUMBER = 5;
    bool buttonHeld = false;
    autonNumber=1;
    Controller.Screen.clearLine(3);
    Controller.Screen.setCursor(3,1);
    Controller.Screen.print(autonNumber);
    while(Controller.ButtonA.pressing()==false){
        if(buttonHeld ==  false){
            if (Controller.ButtonRight.pressing()&& autonNumber < MAX_AUTON_NUMBER){
                autonNumber++;
                buttonHeld= true;
                Controller.Screen.clearLine(3);
                Controller.Screen.setCursor(3,1);
                Controller.Screen.print(autonNumber);
            }
            else if (Controller.ButtonLeft.pressing() && autonNumber>0){
                autonNumber--;
                buttonHeld= true;
                Controller.Screen.clearLine(3);
                Controller.Screen.setCursor(3,1);
                Controller.Screen.print(autonNumber);
            }
            
            
        }
        if(!Controller.ButtonLeft.pressing()&&!Controller.ButtonRight.pressing()){
            buttonHeld = false;
        }
        wait(20,msec);
    }
    Controller.Screen.clearLine(3);
    Controller.Screen.setCursor(3,1);
    Controller.Screen.print("Selected aouton is:");
    Controller.Screen.print(autonNumber);
    for(int i=0; i<autonNumber; i++){
        Controller.rumble(".");
    }
}
