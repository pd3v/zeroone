# zeroOne 

__zeroOne__ is a polyphonic instrument, multi-instrument, DSLish/API MIDI sequencer for live coding music. It sends MIDI messages to any stand-alone synthesiser or DAW.

![livecoding_screenshot](https://github.com/pd3v/wide/blob/develop/livecoding_screenshot.png)

### Dependencies

[RtMidi](http://www.music.mcgill.ca/~gary/rtmidi/) library

### Live coding

[__cling__](https://github.com/root-project/cling.git) (an interative C++ interpreter) is for the live coding enverionment.

[__cling builds for Linux and MacOS__](https://root.cern.ch/download/cling/)


*Or you still can compile/link __zeroOne__ as any other c++ library and code with it as so.*
	
### How to use it

##### 1. open your synth ready to listen MIDI messages
##### 2. open your command line software
##### 3. run ./build.sh
##### 5. at cling's prompt load *zeroOne* by entering the following lines:
	
	[cling]$ .L zeroone
	[cling]$ #include "../src/zoengine.h"
	[cling]$ zeroone()
	
##### 6. after message "zEROoNE on <((()))>" appears, copy/paste the code below:
	
Instrument 1 sends "C Major" chord notes to midi channel 1, 0.9 amplitude and 1/4 duration.

```
i1.play(n(
	({36,40,43}),
	0.9,
	{4},
)) 
```

and/or

Instrument 2 sends cycling c4,cs4 and d4 notes to midi channel 2, 0.5 amplitude, 1/4, 1/8, 1/4, 1/8, 1/4  duration sequence.

```
i2.play(n(
	{cycle({48,49,50},isync(2))},
	0.5,
	({4,8,4,8,4})
)) 
```

and/or

Instrument 3 sends cycling pentatonic minor scale notes to midi channel 3 and modulates cc channel 1 and 2.

```
i3.play(n(
	{cycle(
		scale_::transpose(scale_::pentatonicminor,5),
		isync(3))
	},
	0.9,
	({4,8,4,8,4})),
	cc(1,rnd(chop(10))),
	cc(2,bounce(0,127))
) 
```

## Make some noise!
