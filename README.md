# 59975X V2 Robot

The V2 robot code for **Team 59975X**, competing in the **VEX V5 Robotics
Competition (V5RC)**.

This private repository contains the C++ VEX V5 project for the team's second
robot.

## V2 Robot Hardware

### Four-Motor Drivetrain

The V2 robot uses **four drivetrain motors** for navigation:

- **Left side:** two motors, on Brain ports **14** (front) and **20** (back).
- **Right side:** two motors.

The left and right sides provide forward/reverse movement and turning through
differential drive. Device configuration and drive-control code must use this
four-motor layout.

### Claw

The claw motor is connected to Brain port **11**. **R1** opens the claw and
**R2** closes it. Port 11 is separate from both left drivetrain motor ports.

### Toggle Mechanisms

The V2 design includes two toggle mechanisms:

| Mechanism | Location | Details |
|---|---|---|
| Back toggle | Rear of the robot | The robot retains a back toggle. Its actuation and controls still need to be documented. |
| Motor-controlled side toggle | Left side of the robot | An additional toggle operated by a motor, separate from the drivetrain motors. |

The remaining motor ports, drivetrain gearing, wheel dimensions, toggle travel
limits, and controller bindings still need verification. Confirm these details against
the physical robot before implementing device configuration and mechanism
controls.

## Current Status

The source files, headers, and build files were copied from the team's
`for_now_use_this` project. They include device definitions, pre-autonomous
initialization, autonomous selection and routines, PID movement, and separate
driver-control tasks for the drivetrain, arm, claw, and toggle.

**Hardware adaptation is in progress.** The code now configures four drivetrain
motors (two per side), the arm and claw, and two toggle motors. The left drivetrain
and claw ports have been corrected to match the confirmed wiring. The left toggle
motor is on port 13 and the right toggle motor is on port 3. Verify all remaining device ports, motor cartridges,
gearing, controls, and autonomous movements before downloading or running this
code on V2.

V2's own README and VS Code project settings are retained.

## Toggle Driver Control

- **Hold Button X:** run both toggle motors in reverse at
  100% power.
- **Release Button X:** stop powering the motor with **coast**, without active
  braking or position holding. The mechanism can continue moving from momentum.

The passive-toggle optical-sensor example remains commented out and inactive.
Driver Button X control remains manual and does not use the optical sensors.

## Active Toggle Autonomous Test

- **Left optical sensor:** Brain port **5**, controls the left toggle motor on
  port **13**.
- **Right optical sensor:** Brain port **6**, controls the right toggle motor on
  port **3**.
- During pre-autonomous selection, use **Left/Right** to choose routine **6**,
  **Up** to select **Red**, or **Down** to select **Blue**, then **A** to confirm.
  The controller displays both the routine and color. The default color is Red.
  Color selection affects the active toggle only; it does not mirror existing
  autonomous routes.
- Position the robot at the toggles before starting autonomous. Routine 6 does
  not drive into position or run the other autonomous sequences.
- Each toggle runs independently in reverse at **100%** only while its sensor
  detects a nearby object of the wrong color. If no object is nearby, that motor
  coasts until an object appears or the routine ends.
- When a side detects the selected color, its motor coasts and remains stopped
  for the rest of that invocation. The routine succeeds when both sides finish.
- The temporary timeout is **3 seconds**. Timeout, a missing/disconnected sensor,
  or leaving autonomous stops motor power; failures print a message and rumble
  the controller. Sensor lights are turned off on normal cleanup.

The implementation uses sensor lights at 50%, red hue below 30 or above 330,
and blue hue from 180 through 260. Verify detection under competition lighting
and tune the hue thresholds and `TIMEOUT_MS` in [src/Toggle.cpp](src/Toggle.cpp)
on the robot. Coast allows momentum to move the mechanism after motor power is
removed; test the 100% setting carefully. Existing routines 0 through 5 do not
automatically invoke the active toggle.

## Getting Started

1. Clone this repository:
   ```sh
   git clone https://github.com/elliext/V2-robot.git
   ```
2. Open the `V2-robot` folder in Visual Studio Code with the VEX Robotics
   extension installed.
3. Install the VEX V5 C++ toolchain and the project SDK,
   `V5_20240802_15_00_00`, through the extension.
4. Adapt the copied device configuration, autonomous routines, and driver
   controls to the V2 hardware described above.
5. Build the project, connect the V5 Brain, and download the program using the
   VEX extension. The project is currently configured for program slot **1**.

Repository access is required to clone this private project. Collaborators must
accept their GitHub invitation first.

### Download Checklist

1. Open this project's `V2-robot` folder in VS Code, not the original
   `for_now_use_this` folder, and select V2 as the active VEX project.
2. Install the recommended **VEX Robotics** and **C/C++** extensions. Ensure the
   VEX extension has installed the V5 C++ toolchain and the SDK version listed
   above.
3. Use the VEX extension to build the project. A successful build produces
   `build/V2-robot.bin`, the program binary to download.
4. Power on the V5 Brain and connect it to the computer using a USB data cable.
   Confirm that the VEX extension detects and selects the intended Brain.
5. Confirm the destination program slot. This project's default is **slot 1**;
   downloading there replaces any program already in that slot.
6. Use the VEX extension's download action. If the Brain is not detected, check
   its power, the USB data cable, and device selection before retrying.

The repository includes its source, headers, build rules, and VEX project
settings. The toolchain, SDK, and connected Brain are external requirements;
generated binaries are not committed to Git.

**Download readiness does not mean the code is ready to run on V2.** Adapt and
verify the hardware configuration first, and do not use a download-and-run
action before that verification.

## Project Structure

| Path | Purpose |
|---|---|
| [src/main.cpp](src/main.cpp) | Competition setup, pre-autonomous initialization, autonomous routine, and driver-control loop. |
| [src/robot-config.cpp](src/robot-config.cpp) | Device definitions and `vexcodeInit()`. |
| [include/robot-config.h](include/robot-config.h) | Device declarations shared by the robot code. |
| [include/vex.h](include/vex.h) | VEX SDK includes and common macros. |
| [src/Drivetrain.cpp](src/Drivetrain.cpp) and [src/PID.cpp](src/PID.cpp) | Joystick driving and PID-based movement. |
| [src/auton-selector.cpp](src/auton-selector.cpp) and [src/autonomos.cpp](src/autonomos.cpp) | Autonomous selection and routines. |
| [src/Arm.cpp](src/Arm.cpp), [src/Claw.cpp](src/Claw.cpp), and [src/Toggle.cpp](src/Toggle.cpp) | Original robot mechanism controls to adapt for V2. |
| [.vscode/](.vscode/) | VEX project settings and C++ editor configuration. |
| [makefile](makefile) and [vex/](vex/) | Build configuration and rules. |

Generated build outputs are excluded from Git.

## Development

- Keep motor ports, gear cartridges, and reversal settings consistent with the
  physical V2 robot.
- Verify the two drive motors per side and the left/right toggle motor and sensor
  pairings against the physical robot. Confirm the back toggle's actuation.
- Establish safe travel limits and stopping behavior for the motor-controlled
  side toggle before testing it.
- Document driver controls and autonomous routines here as they are implemented.
- Build and test changes on the robot before using them in competition.
