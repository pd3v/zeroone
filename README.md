# wide 

__wide__ is a polyphonic instrument, multi-instrument, DSLish/API MIDI sequencer for live coding music. It sends midi messages to any stand-alone synthesiser or DAW.

### Dependencies

[RtMidi](http://www.music.mcgill.ca/~gary/rtmidi/) library

### Live coding

[__cling__](https://github.com/root-project/cling.git) (an interative C++ interpreter) is for the live coding enverionment.

*Or you still can compile/link __wide__ as any other c++ library and code with it as so.*
	
### How to use it

##### 1. run cling in terminal
##### 2. at cling prompt load *wide* by typing the following line:
	
	.x <path to>wide.cpp
	
#### 3. after message "wide is on <((()))>" appears, type:
	
`i(1).play(n(({0,2,4}),0.9,{4},4)) // Instrument "0" sends "C Major" chord notes to midi channel 1, 0.9 amplitude, 1/4 duration and 4th octave`

the same as	

`i(1).play(n((CMaj),0.9,{4},4)) // Instrument's scale defaults C Major scale`


or

`i(1).play(n(({0}),0.5,({4,8,4,8,4}),5)) // Instrument "0" sends c note to midi channel 1, 0.5 amplitude, 1/4, 1/8, 1/4, 1/8, 1/4  duration sequence and 5th octave`


