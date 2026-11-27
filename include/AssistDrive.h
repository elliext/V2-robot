#pragma once

// Driver aim-and-approach assist. Hold Up to aim at the nearest upright cup, or
// Down for the nearest goal tag; release to drive up to it. Pushing either stick
// firmly during the approach cancels it.

// Call once per drivetrain loop. Returns true if the assist drove the motors this
// tick, in which case the stick drive must be skipped.
bool assistDrive();
void assistReset();
