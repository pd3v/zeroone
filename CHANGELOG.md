## 0.6

. Build now includes [__diatonic__](https://github.com/pd3v/diatonic) library

. Build now includes **RtMidi** library

____

## 0.5

. Builds as dynamic library

____

## 0.4.6
. New beat sync resettable random numbers functions - **rndsync** and **rndbunchsync** 

. Simpler instruments' identifications: i1, i2, ...,i5  adding to the already existent i(1), i(2), ..., i(5)

. Possible to set CC and note(s) within the instrument's **play** function definition

. **rotl** and **rotr** beat sync resettable

. Put back instruments' playing out notes feature

____

## 0.4.5

. New helper **rnd10**,  **rnd25**, **rnd50**, **rnd75**, **thisthator** functions

. Improved sync

___

## 0.3.2


. New **n** function with note(s), velocity and duration parameters; no octave

. **n** function is now **no**, and "o" stands for octave parameter

. Improved sync

. New helper function - **transp**

. Metronome on its own thread

____
 
## 0.3.1


. Validations in some helper functions

. Improved sync

. Short-typed note duration pattern; eg. short-typed {4,3,6,8} parses into {4,3,3,3,6,6,6,6,6,6,8,8}. 1st 1/4 = 4, 2nd 1/4 = 3,3,3, etc.

. New helper functions - **edger**, **edgerx**, **swarm**, **chop**, **insync** and **bounce**

. Instruments +1 to work as a metronome (metro()), alias **sync**

. Rename **istep** to **isync**, **ccstep** to **ccsync**, **whenMod** to **when**

___

## 0.3
 

. Refactoring some helper functions to become more generic

. new helper functions: **cycle**, **rcycle**, **slow**, **fast** and **sine**

. MIDI CC (virtually unlimited) to each instrument

. **ccstep**, for synchronizing CC changes to instrument rhythm

. **noctrl**, to remove all MIDI CC from each and every instrument

. **x**, stands for rest note

___
