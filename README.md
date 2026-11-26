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
- **Hold Button A:** use the optical sensors to turn each toggle to the selected
  alliance color. Each side runs at 100% until its own sensor sees a nearby
  object of the target color, and stays stopped after reaching the target until
  A is released. This is nonblocking and remains active only while A is held;
  unlike autonomous, there is no automatic 3-second timeout.
- **Release both A and X:** stop powering both motors with **coast**, without
  active braking or position holding. The mechanism can continue moving from
  momentum. Releasing A cancels optical control; press it again to start fresh.
- **A and X together:** X manual control takes priority. Releasing X while A is
  still held starts a fresh optical-control session.
- Sensor failure stops both motors in optical mode and reports the error once
  per hold. Release A before retrying. Driver-control exit also coasts the motors.

The passive-toggle optical-sensor example remains commented out and inactive.
Driver Button X control remains manual and does not use the optical sensors.
Button A no longer activates the arm's -30/+30-degree preset; the arm's existing
manual controls and position holding remain in place. A still confirms the
autonomous selection during pre-autonomous setup.

## Optical Sensor Controller Display

Controller lines 1 and 2 show live readings for the left (port 15) and right
(port 5) optical sensors:

```text
L H:25.0 Near:Y
R H:220.0 Near:N
```

`H` is the hue in degrees; `Near:Y` means the sensor detects a nearby object,
and `Near:N` means it does not. A disconnected sensor shows `sensor missing`
instead of a numeric reading. Hue is most useful when an object is nearby.

Readings refresh approximately every 100 ms during autonomous selection and,
after pre-autonomous setup finishes, while disabled, in autonomous, and in
driver control. Line 3 remains reserved for the autonomous number and alliance
color. The display only reads the sensors; it does not change motor control or
sensor lighting.

## Active Toggle Autonomous Test

- **Left optical sensor (`toggleOptical2`):** Brain port **15**, controls the left toggle motor on
  port **13**.
- **Right optical sensor (`toggleOptical1`):** Brain port **5**, controls the right toggle motor on
  port **3**.
- During pre-autonomous selection, use **Left/Right** to choose routine **6**,
  **Up** to select **Red**, or **Down** to select **Blue**, then **A** to confirm.
  The controller displays both the routine and color. The default color is Red.
  Color selection affects the active toggle only; it does not mirror existing
  autonomous routes.
- Position the robot at the toggles before starting autonomous. Routine 6 does
  not drive into position or run the other autonomous sequences.
- Each toggle runs independently in reverse at **100%** until its sensor
  detects a nearby object of the selected color. It keeps turning when no object
  is detected; the autonomous timeout or releasing the driver button stops it.
- When a side detects the selected color, its motor coasts and remains stopped
  for the rest of that invocation. The routine succeeds when both sides finish.
- The temporary timeout is **3 seconds**. Timeout, a missing/disconnected sensor,
  or leaving autonomous stops motor power; failures print a message and rumble
  the controller. Sensor lights remain on after a toggle session so the
  controller readings use the same lighting as the color checks.

The implementation uses sensor lights at 50%, an object-detection threshold of
40, red hue at most 12 or at least 340, blue hue from 180 through 260, and yellow
hue from 18 through 60. The display labels these colors R, B, Y, or ?.
Verify detection under competition lighting
and tune the hue thresholds and `TIMEOUT_MS` in [src/Toggle.cpp](src/Toggle.cpp)
on the robot. Coast allows momentum to move the mechanism after motor power is
removed; test the 100% setting carefully. Existing routines 0 through 5 do not
automatically invoke the active toggle.

### Calling the Active Toggle from an Autonomous Routine

With [include/autonomos.h](include/autonomos.h) included, call:

```cpp
if(!activeToggle()){
  return;
}
```

The no-argument function uses the selected Red/Blue alliance color and the
current competition state. It blocks until both toggles reach the target color
or the 3-second timeout ends. It returns `true` on success and `false` on a
timeout, sensor failure, or an invalid/ended autonomous mode, so the routine can
avoid continuing after a failure. Position the robot at the toggles and stop
drivetrain motion before calling it. Autonomous option 6 uses this same call.

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

### Turn PID Timeout

`turnPID(target, maxIntegral, tolerance, timeoutMs)` accepts an optional fourth
parameter in milliseconds. Omit it or use `0` for no timeout:

```cpp
turnPID(90, 20, 0.5);        // No timeout.
turnPID(90, 20, 0.5, 3000);  // Brake after at most 3 seconds of turning.
```

The integral is capped at both `-maxIntegral` and `+maxIntegral`. A timed-out
turn brakes the drivetrain, prints the remaining error, and rumbles the
controller. The existing 50 ms settling delay follows braking. Negative or
non-finite timeouts are rejected with braking and an error notification.
With no timeout, a turn that never reaches its tolerance can keep running.

- Keep motor ports, gear cartridges, and reversal settings consistent with the
  physical V2 robot.
- Verify the two drive motors per side and the left/right toggle motor and sensor
  pairings against the physical robot. Confirm the back toggle's actuation.
- Establish safe travel limits and stopping behavior for the motor-controlled
  side toggle before testing it.
- Document driver controls and autonomous routines here as they are implemented.
- Build and test changes on the robot before using them in competition.
