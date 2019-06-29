# wide 

__wide__ is an DSLish API for live coding music. It sends midi messages to any stand-alone synthesiser or DAW. Release v0.1.

### Dependencies

RtMidi library.

It can be downloaded here: [http://www.music.mcgill.ca/~gary/rtmidi/](http://)

### Live coding

__cling__ (an interative interperter C++ interpreter) is for the live coding enverionment.

It can be download here: [https://github.com/root-project/cling.git](http://)

*Or you still can compile/link __wide__ as any other c++ library and code with it as so.*
	
### How to use it

#####1. run cling in terminal
#####2. at cling prompt load *wide* by typing the following lines:
	.x <path to>expression.h
	.x <path to>expression.cpp
	.x <path to>instrument.h
	.x <path to>instrument.cpp
	.x <path to>generator.h
	.x <path to>generator.cpp
	.x <path to>sequencer.cpp
####3. after message "wide is on..." appears, type:
	
`// Instrument "0" sends C notes to midi channel 0, 100 velocity, 1/4 duration, and 4th octave`	

`i(0).play(n(0,100,4,4))` 




	



