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

### Auton Selector: Initial Match Configuration

At program startup, select the match configuration on the controller once,
before autonomous or driver control:

- **Up:** select **Red** alliance; **Down:** select **Blue** alliance.
- **A:** confirm. Controller line 3 shows the wing name and alliance,
  for example `Right Wing Red`, then `Set Left Wing Blue` after confirmation.
- **Left directional arrow:** select **Left Wing**.
- **Right directional arrow:** select **Right Wing**.
  Holding an arrow does not cause repeated redraws; pressing both keeps the
  current wing unchanged.
- **L1/R1:** unused during selection.

Release **A** after launching the program, then press it again to confirm.
Selection redraws are batched and padded instead of clearing the line.
Console messages beginning with `Auton selector:` report initial selection,
changes, and confirmation to help diagnose controller input.

The initial configuration is Right Wing, Red. The selected `selectedWing` and
`redAlliance` settings in [include/auton-selector.h](include/auton-selector.h)
are available to autonomous and driver-control code. `selectedWing` uses the
`AutonWing::Right` and `AutonWing::Left` enum values, not numeric selections.
`selectAuton()` in [src/auton-selector.cpp](src/auton-selector.cpp) is called
from the VEX template's `pre_auton()` startup function, not from either
competition-phase callback. Restart the program to configure the next match.
This is manual pre-match configuration,
not automatic sensor-based detection.

For selection-only hardware testing, connect a powered Brain and controller.
Startup reports a missing inertial sensor and skips calibration; neither the
inertial nor AI Vision sensor is required to select the alliance and wing.
Autonomous turns still require the inertial sensor on port 1. Disconnect motor
Smart Cables for this test: existing startup powers the claw after **A** confirms.
A one-off download to slot 4 can be used without changing the normal slot 1
project setting.

For an isolated motor-free selection test, build with
`DEFINES="-DVexV5 -DAUTON_SELECTION_TEST"` and a separate output directory,
for example `make all BUILD=build/selection-test P=Selection-test
DEFINES="-DVexV5 -DAUTON_SELECTION_TEST"` using the installed VEX SDK/toolchain.
This test entry point refuses field-control/competition-switch connections,
enables driver input, and runs the selector without registering competition
callbacks or running normal pre-autonomous motor initialization. **A** only
confirms selection; it does not start the claw. Restart the test program to
select again. Download this separate binary to slot 4; normal builds omit the
flag and retain the competition program.

**Right Wing** always calls `rightWing()` and **Left Wing** always calls
`leftWing()` in [src/autonomos.cpp](src/autonomos.cpp). Neither wing path calls
the active motorized toggle automatically; `activeToggle()` remains available
for use in other autonomous code.
Normal startup confirms alliance and wing before registering motor-control
competition callbacks, so the path cannot start with an unconfirmed selection.
The selected wing, not alliance color, determines the path. Both functions are declared in
[include/autonomos.h](include/autonomos.h) and can also be called directly
from an autonomous case. Each has its own post-scoring drive/turn commands,
PID settings, and claw timing so the two sides can be tuned independently
for the robot's uneven mass distribution. The initial turn values are:

| Wing | Turn after scoring | Passive-toggle turn | Final turn |
|---|---|---|---|
| Right Wing | -60 degrees | +82 degrees | +42.5 degrees |
| Left Wing | +60 degrees | -82 degrees | -42.5 degrees |

The paths initially retain their existing distances, PID settings, and mechanism
actions, but no longer use a `turnDirection` multiplier. Edit either function
without changing the other. `leftWing()` calls `scoreLeftAllianceGoal()` and
`rightWing()` calls `scoreRightAllianceGoal()` in
[src/autonomos.cpp](src/autonomos.cpp). Each scoring function has its own
reverse-drive distance/PID settings, arm speed, arm wait, and claw release
duration/speed; edit these separately to tune scoring on each side.
Both still call the shared `pickUpPin()` helper for the initial pickup.
The final turns retain the previous mirrored values. Verify
both independently tuned paths on the physical
field before competition. Only Right Wing and Left Wing are currently selectable.
Alliance color still selects the target color for active toggles.

`make test` also runs [tests/AutonTest.cpp](tests/AutonTest.cpp) against the
actual selector and autonomous code with fake controller inputs and motors.
It checks both alliances, ignored shoulder buttons, Right Wing / Left Wing
mapping, direct arrow selection, held/simultaneous-button behavior,
wing-name confirmation text, both direct scoring- and wing-function calls
and wing dispatch, and unchanged initial turn/drive/PID/mechanism
commands, and alliance forwarding to the active toggle. These host tests do not
verify physical field geometry or download anything to the robot.

### AI Vision Sensor: Detection and Approximate Distance Display

The AI Vision Sensor is configured on **Brain Smart Port 6**. Connect it to the
Brain with a Smart Cable, and connect the Brain to the computer by USB to
download the program. Direct sensor-to-computer USB is not needed for runtime
detection.

This display enables **pre-trained AI model detection and AprilTag detection**
(color detection stays off). It shows detected class names, IDs, confidence
scores, and center coordinates on the Brain screen during initial match setup
and after pre-autonomous setup. See [AprilTag Goal Detection](#apriltag-goal-detection)
for the tag line.
It does not steer, approach objects, change autonomous paths, or assign an
assist button. Existing controller optical readouts and driver controls remain
unchanged; **L1 already controls the arm**.

In [src/AIVision.cpp](src/AIVision.cpp):

- `kTargetClassName` is empty by default, so all detected model classes appear.
  Set it to an exact reported class name to filter the display.
- `kRefreshIntervalMs` controls the display/snapshot interval (default 250 ms).
- Up to 24 detections are sampled, with up to 3 matching objects displayed.
  Each object has two rows so class, ID, score, and center coordinates remain
  visible alongside bounding-box size and approximate distance.

The sensor can recognize only classes in its installed model. Hardware testing
with the Override model confirmed `Cup` (ID 4) and `RedYellowPin` (ID 2), and
confirmed that detections clear when the objects leave the view. Other pin
classes and goals have not yet been verified. No class names or IDs are
hard-coded as detection filters. Distance estimation supports only the confirmed
`Cup` and `RedYellowPin` class names. Missing hardware and invalid
snapshot counts are reported on the Brain screen and in console output.
Detection resumes automatically after a detected disconnection and reconnection.

To check recognition, power on the Brain, download/run the program, and place
each desired game object in view. The Brain should show its class and position
if supported. This display adds no motor commands, but the **existing robot
program is not a motor-free test**: pre-autonomous setup and enabled competition
tasks retain their existing behavior. Keep the robot supported and mechanisms
clear while testing. Hold-to-center, approach assist, and autonomous integration
are deferred until recognition and driver preferences are confirmed.

#### AprilTag Goal Detection

[src/AprilTag.cpp](src/AprilTag.cpp) identifies goals from their AprilTags
using the field facts in [knowledge/field-setup.md](knowledge/field-setup.md):

| Tag | Goal | Quadrant number |
|---|---|---|
| 0 | Center | n/a |
| 1 | Neutral | 1 (Red 1 / Blue 1) |
| 2 | Alliance | 1 (Red 1 / Blue 1) |
| 3 | Alliance | 2 (Red 2 / Blue 2) |
| 4 | Neutral | 2 (Red 2 / Blue 2) |

Tags 1-4 repeat on both alliances, so a tag gives the goal's role and quadrant
number but not its alliance. `goalQuadrantName()` turns a tag into a quadrant
name only when the caller supplies the alliance. `detectGoalTags()` takes a tag
snapshot, ignores unknown IDs and missing or non-tag objects, and returns the tag
ID, role, quadrant number, center, and size. It overwrites `AIVision.objects`,
so the display reads model objects first. Autonomous code does not use it yet.

Footer line of the display: `Tags 3: 0=C 1=N1 2=A1` (`C` center, `A` alliance,
`N` neutral, followed by the quadrant number), `Tags: none`, or
`Tags: snapshot error`. It replaces the earlier tilt/occlusion note. Tilt and
occlusion still make the Cup/pin distance approximate.

#### Distance Calibration (Display Only)

The known cup height is **164.5 mm** and the pin length is **165.1 mm**.
The display uses the larger of bounding-box width and height as apparent length:

```text
focal length (pixels) = calibration length (pixels) * known distance (mm)
                       / known object length (mm)
distance (mm) = known object length (mm) * focal length (pixels)
                / apparent length (pixels)
```

After an explicitly requested download/run, calibrate using the Brain screen:

1. Place exactly one supported cup or pin upright, fully visible, near the
   image center at **812.8 mm (32 inches)** from the camera lens. Measure to the object
   center plane, not the robot bumper, and keep its long axis parallel to the
   image plane. Ensure the object does not touch any image edge.
2. Tap **Cal 812.8mm** on the Brain screen. The status must show `Cal OK`.
3. Move the object to measured distances and compare the displayed estimate.
   Supported detections show pixel dimensions and approximate millimeters.

Change `kCalibrationDistanceMm` in [src/AIVision.cpp](src/AIVision.cpp) if a
different reference distance is needed; the button and instructions follow it.
Calibration is held in memory only: restart or detected sensor
disconnection requires recalibration. A held touch calibrates only once;
release and tap again to recalibrate. Failed calibration does not overwrite
a previously valid reference. Missing/ambiguous supported objects and invalid
or edge-touching boxes are reported explicitly. A snapshot error cancels a
pending calibration tap.

Distances show `N/A` before calibration, for unsupported classes, or for invalid
or edge-touching boxes. Other pin classes need their exact reported names
verified before adding their known lengths.

These are **approximate camera-axis distances**, not ground-plane distances,
bumper clearances, or guaranteed measurements. Fully visible upright or
sideways objects aligned with an image axis work best. Diagonal orientation,
3D tilt, lying objects viewed from above, occlusion, detection-box jitter,
and the cup's transparent half can bias the box and therefore the estimate.
Bounding boxes alone cannot identify or correct those effects. No physical
accuracy has been established; verify at several measured distances and poses
before considering any future motion integration.

This feature issues no motor commands and changes no controller bindings.
The existing competition program still has motor initialization and controls;
calibration does not make that program safe for unsupported hardware testing.

For a **motor-free vision-only hardware test**, build with
`DEFINES="-DVexV5 -DAI_VISION_TEST"` and a separate output directory:

```sh
make all BUILD=build/vision-test P=Vision-test DEFINES="-DVexV5 -DAI_VISION_TEST" PRINTF_FLOAT=1
```

Use the installed VEX SDK/toolchain paths as needed. This entry point refuses
field-control/competition-switch connections and only updates the AI Vision
display; it skips controller selection, optical readouts, motor initialization,
and competition callbacks. It does not enable driver mode or test controller
input. Download to an explicitly approved spare slot (for example slot 4),
leaving normal slot 1 settings unchanged. Do not combine `AI_VISION_TEST` and
`AUTON_SELECTION_TEST`. Builds without either flag retain normal behavior.
The vision-only test uses screen double buffering to present complete frames
without clear/redraw flashing. Frame-presentation failures and recovery are
reported in console output.
Its snapshot/display interval is 1000 ms so changing detections are easier to
read during calibration; normal builds retain the 250 ms interval.
Normal VS Code build/download uses `build/` and slot 1 with neither hardware
test flag enabled. Host test sources are not part of the robot build.
The makefile rejects hardware-test flags in the normal `build/` directory;
test programs must use a separate output directory and an explicitly approved
spare slot. A build-mode stamp forces robot objects to rebuild when switching
between normal and hardware-test modes, preventing stale test objects from
entering a normal binary. The controller receives no separate robot program:
the program runs on the Brain.
The vision-only test also auto-calibrates once per program start from exactly
one valid `Cup`, ignoring pin detections for calibration. Hold the cup upright
and fully visible at the reference distance before starting the program.
Five consecutive snapshots must have the same long-axis orientation and a
maximum pixel length no more than 10% above the minimum; their mean length sets
the scale. Missing, duplicate, invalid, or clipped cups and snapshot failures
reset progress. After success, moving the cup does not recalibrate. A successful
manual calibration also stops automatic calibration. Sensor disconnection
clears the scale but does not rearm a completed automatic calibration; restart
at the reference distance or calibrate manually.
The current 32-inch reference was only roughly measured, so automatic success
is labeled `Auto Cal OK; rough 32in reference`. Stability does not prove the
reference distance, correct pose, or measurement accuracy. Normal competition
builds remain manual-calibration only.

#### AI Vision Unit Tests

Run the host-side tests without connecting any VEX hardware:

```sh
make test
```

This requires a host C++11 compiler (`c++` by default; override with
`HOST_CXX=clang++` if needed), but no VEX SDK or additional test dependencies.
The tests compile the actual [AI Vision display code](src/AIVision.cpp) against
small sensor, timer, and screen fakes in [tests/fakes/vex.h](tests/fakes/vex.h).
Each case runs in a separate process so the display's static state is isolated.

The 29 cases cover missing hardware, model and tag configuration, reported object
data, default all-class filtering, rejected object types/nonexistent objects,
stale-data clearing, the 3-object/two-row display limit with 24 detections,
invalid snapshot counts and recovery, the exact 250 ms refresh boundary,
and reconnection. Distance cases check exact reference/scaled output, sideways
boxes, cup versus pin dimensions, invalid/clipped boxes, calibration ambiguity,
touch debouncing and taps between snapshots, disconnect invalidation, and
snapshot-fault cancellation. Ten AprilTag cases cover the tag table, quadrant
names, geometry, filtering, output limits, snapshot errors, and the combined
model-and-tag display.
The same cases also run with `AI_VISION_TEST` and its exact 1000 ms refresh
boundary, plus three auto-calibration cases covering the five-sample threshold,
pin exclusion, averaging, instability/progress resets, and one-shot behavior.
The test binary is stored under `build/tests`, separately from the robot build.

These AI Vision tests replace the earlier vision diagnostic build mode, separate
from the selection-only hardware test described above. They do not download
or run anything on the Brain, test motor movement, validate a real AI model, or
prove physical recognition. The earlier Cup and RedYellowPin results above are
hardware observations; real-world recognition still requires a hardware test.

Keep object-detection test photographs outside this repository. They are local
test data, not required by `make test`; `/rypc.png` is ignored to prevent
accidental inclusion in future commits.

The source files, headers, and build files were copied from the team's
`for_now_use_this` project. They include device definitions, pre-autonomous
initialization, match setup and autonomous paths, PID movement, and separate
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
match configuration during initial setup.

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

Readings refresh approximately every 100 ms during initial match setup and,
after pre-autonomous setup finishes, while disabled, in autonomous, and in
driver control. Line 3 remains reserved for the wing name and alliance
color. The display only reads the sensors; it does not change motor control or
sensor lighting.

Optical lines are written only when their displayed text changes, without
clearing first. Disconnected sensors still show `sensor missing`, but the same
message is not repeatedly cleared and rewritten.

`make test` includes [tests/OpticalDisplayTest.cpp](tests/OpticalDisplayTest.cpp)
to check change-only rendering, the 100 ms refresh boundary, reconnection,
preservation of selection line 3, and the absence of display-driven motor commands.
Selector tests also cover release-before-confirm and batched, non-clearing redraws.

## Active Toggle Autonomous Helper

- **Left optical sensor (`toggleOptical2`):** Brain port **15**, controls the left toggle motor on
  port **13**.
- **Right optical sensor (`toggleOptical1`):** Brain port **5**, controls the right toggle motor on
  port **3**.
- During initial match setup, use **Up** to select **Red**, or **Down**
  to select **Blue**, then **A** to confirm.
  The controller displays the wing name and alliance. The default color is Red.
  Color selection affects the active toggle only; it does not mirror existing
  autonomous routes.
- Neither **Right Wing** nor **Left Wing** calls the active
  toggle automatically. To use it in another autonomous sequence, position
  the robot at the toggles, stop the drivetrain, and call `activeToggle()`.
- Each toggle runs independently in reverse at **100%** until its sensor
  detects a nearby object of the selected color. It keeps turning when no object
  is detected; the autonomous timeout or releasing the driver button stops it.
- When a side detects the selected color nearby, its motor brakes instead of
  coasting. Each side must keep detecting the target for at least **100 ms**.
  Both sides are checked throughout the action: a side resumes turning if the
  color or proximity is lost, even after it was previously confirmed. Autonomous
  succeeds only when both sides are currently confirmed, and leaves both motors
  braked. Driver **A** keeps checking and correcting until released.
- The temporary timeout is **3 seconds**. Timeout, a missing/disconnected sensor,
  or leaving autonomous stops motor power; failures print a message and rumble
  the controller. Sensor lights remain on after a toggle session so the
  controller readings use the same lighting as the color checks.

The implementation uses sensor lights at 100%, an object-detection threshold of
40, red hue at most 24 or at least 360, blue hue from 100 through 350, and yellow
hue from 24 through 70. The display labels these colors R, B, Y, or ?.
Verify detection under competition lighting
and tune the hue thresholds and `TIMEOUT_MS` in [src/Toggle.cpp](src/Toggle.cpp)
on the robot. Braking reduces overshoot but must still be checked on the robot;
test the 100% setting and `kTargetStableMs` carefully. Both independent wing
paths use passive toggles and do not automatically invoke the active toggle.

### Calling the Active Toggle from an Autonomous Sequence

With [include/autonomos.h](include/autonomos.h) included, call:

```cpp
if(!activeToggle()){
  return;
}
```

The no-argument function uses the selected Red/Blue alliance color and the
current competition state. It blocks until both toggles reach the target color
or the 3-second timeout ends. It returns `true` on success and `false` on a
timeout, sensor failure, or an invalid/ended autonomous mode, so the sequence can
avoid continuing after a failure. Position the robot at the toggles and stop
drivetrain motion before calling it.

## Getting Started

1. Clone this repository:
   ```sh
   git clone https://github.com/elliext/V2-robot.git
   ```
2. Open the `V2-robot` folder in Visual Studio Code with the VEX Robotics
   extension installed.
3. Install the VEX V5 C++ toolchain and the project SDK,
   `V5_20240802_15_00_00`, through the extension.
4. Adapt the copied device configuration, autonomous paths, and driver
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
   Use **slot 4** only as a temporary local override for vision-related testing.
   Restore slot 1 afterward; do not commit or push the temporary slot-4 override.
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
| [src/main.cpp](src/main.cpp) | Competition setup, pre-autonomous initialization, autonomous execution, and driver-control loop. |
| [src/robot-config.cpp](src/robot-config.cpp) | Device definitions and `vexcodeInit()`. |
| [include/robot-config.h](include/robot-config.h) | Device declarations shared by the robot code. |
| [include/vex.h](include/vex.h) | VEX SDK includes and common macros. |
| [src/Drivetrain.cpp](src/Drivetrain.cpp) and [src/PID.cpp](src/PID.cpp) | Joystick driving and PID-based movement. |
| [src/auton-selector.cpp](src/auton-selector.cpp) and [include/auton-selector.h](include/auton-selector.h) | Initial alliance and wing selection; shared settings for autonomous and driver control. |
| [src/autonomos.cpp](src/autonomos.cpp) | Autonomous path using the selected match configuration. |
| [src/AIVision.cpp](src/AIVision.cpp) and [src/AprilTag.cpp](src/AprilTag.cpp) | AI Vision display and AprilTag goal identification. |
| [src/Arm.cpp](src/Arm.cpp), [src/Claw.cpp](src/Claw.cpp), and [src/Toggle.cpp](src/Toggle.cpp) | Original robot mechanism controls to adapt for V2. |
| [.vscode/](.vscode/) | VEX project settings and C++ editor configuration. |
| [makefile](makefile) and [vex/](vex/) | Build configuration and rules. |

Generated build outputs are excluded from Git.

## Development

### Drive PID Distance Feedback

`drivePID` uses 3.25-inch wheels and a 36-tooth motor gear driving a 60-tooth
wheel gear. Distance is motor revolutions times wheel circumference divided by
`5/3`, with no extra calibration factor. Because positive drive commands spin
the motors in reverse, the `LeftBack` encoder reading is negated before this
conversion so measured distance and target distance have matching signs.
Test a short move in both directions in open space before running autonomous.

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

Positive turn targets are clockwise (right); negative targets are
counterclockwise (left), matching the inertial sensor's signed rotation.
The inertial sensor is currently configured on Brain port **1** and must finish
calibrating while the robot is stationary. Turns are rejected if the sensor
is missing or still calibrating. The +90-degree autonomous test explicitly uses
a 3000 ms timeout; other calls retain their chosen timeout behavior.

- Keep motor ports, gear cartridges, and reversal settings consistent with the
  physical V2 robot.
- Verify the two drive motors per side and the left/right toggle motor and sensor
  pairings against the physical robot. Confirm the back toggle's actuation.
- Establish safe travel limits and stopping behavior for the motor-controlled
  side toggle before testing it.
- Document driver controls and autonomous paths here as they are implemented.
- Build and test changes on the robot before using them in competition.
