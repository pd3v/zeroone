## 0.3
 

. Refactoring some helper functions to become more generic

. new helper functions: **cycle**, **rcycle**, **slow**, **fast** and **sine**

. MIDI CC (virtually unlimited) to each instrument

. **ccstep**, for synchronizing CC changes to instrument rhythm

. **noctrl**, to remove all MIDI CC from each and every instrument

. **x**, stands for rest note

__


## 0.3.1


. Validations in some helper functions

. Improved sync

. Short-typed note duration pattern; eg. short-typed {4,3,6,8} parses into {4,3,3,3,6,6,6,6,6,6,8,8}. 1st 1/4 = 4, 2nd 1/4 = 3,3,3, etc.

. New helper functions - **edger**, **edgerx**, **swarm**, **chop**, **insync** and **bounce**

. Instruments +1 to work as a metronome (metro()), alias **sync**

. Rename **istep** to **isync**, **ccstep** to **ccsync**, **whenMod** to **when**

__
