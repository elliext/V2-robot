# Game Objects

Dimensions given by the repo owner.

| Object | Size |
| --- | --- |
| Cup | About 3.15 in (80 mm) in diameter and 6.5 in (164.5 mm) tall |
| Pin | About 6.5 in long; about 3.1-3.15 in wide at its widest (the middle), narrowing to about 1.25-1.5 in at the ends |

## Implications

- An upright cup's bounding box is about 80 x 164.5 mm, a height:width ratio of about 2.06. A cup on its side is the same shape turned, about 0.49. The "upright cup" filter (`kUprightRatio` in [src/AutoAim.cpp](../src/AutoAim.cpp)) separates these.
- A pin is the same 6.5 in length as a cup and about as wide at its middle, so a cup and a pin have a similar box. Telling them apart relies on the model class name, not on box shape. Pins come in several variants, so code treats any class whose name ends in `Pin` (for example `RedYellowPin`) as a pin and `Cup` as a cup. See [include/GameObjects.h](../include/GameObjects.h).
- `objectLengthMm` in [src/AIVision.cpp](../src/AIVision.cpp) uses 164.5 mm for the cup and 165.1 mm (6.5 in) for the pin.
