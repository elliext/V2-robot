# Field Setup

## Quadrants

The field is a 6x6 grid of square tiles. The diagonal barriers divide it into 4 triangular quadrants: Red 1, Red 2, Blue 1, Blue 2 (a quadrant is not a corner).

- Red 1 and Blue 1 are opposite each other (so Red 2 and Blue 2 are opposite too).
- Red 2 is to the right of Red 1 when facing the field center from Red 1; likewise Blue 2 is to the right of Blue 1.
- Order Red 1 -> Red 2 -> Blue 1 -> Blue 2 is counterclockwise from above.

Layout (audience view):

```
            Blue 2

 Red 1               Blue 1

            Red 2
```

## Auton selector

[src/auton-selector.cpp](../src/auton-selector.cpp) picks `redAlliance` and `AutonWing` (Left/Right, as seen facing the field center from the alliance's side).

| Selection | Quadrant |
| --- | --- |
| Red Left | Red 1 |
| Red Right | Red 2 |
| Blue Left | Blue 1 |
| Blue Right | Blue 2 |

## Goals and AprilTags

- Each quadrant has an **alliance goal** (alliance color) and a **neutral goal** (black).
- One center goal (neutral, slightly taller). 9 goals total.
- Non-center goals sit at (1,2) or (2,1) tiles from a field corner; each quadrant's two goals come from different corners.
- Every goal has an AprilTag readable by the AI Vision sensor. Tag IDs 1-4 repeat across alliances, so identify a goal by ID plus color/quadrant.

| Quadrant | Alliance goal tag | Neutral goal tag |
| --- | --- | --- |
| Red 1 | 2 | 1 |
| Red 2 | 3 | 4 |
| Blue 1 | 2 | 1 |
| Blue 2 | 3 | 4 |
| Center | | 0 |
