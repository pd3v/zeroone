#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <deque>
#include <thread>
#include <future>
#include <chrono>
#include <mutex>
#include <algorithm>

#include "notes.hpp"
#include "taskpool.hpp"
#include "instrument.hpp"
#include "generator.hpp"
#include "expression.hpp"
#include "../external/rtmidi/RtMidi.h"
#include "../external/diatonic/include/diatonic.h"

#define i1 i(1)
#define i2 i(2)
#define i3 i(3)
#define i4 i(4)
#define i5 i(5)
#define f(x) [&](){return x;}
#define n(c,a,d) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<int> d),1};}       // note's absolute value setting, no octave parameter
#define no(c,a,d,o) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<int> d),o};}    // note's setting with octave parameter
#define cc(ch,value) [&]()->CC{return (CC){ch,value};}

const char* PROJ_NAME = "zEROoNE";
const uint16_t NUM_TASKS = 5;
const float BAR_DUR_REF = 4000000; // microseconds
const uint8_t JOB_QUEUE_SIZE = 64;
const int CC_FREQ = 10; // Hz
constexpr uint8_t CC_RESOLUTION = 1000/CC_FREQ; //milliseconds
const float BPM_REF = 60;
const int REST_NOTE = 127;
const function<Notes()> SILENCE = []()->Notes {return {(vector<int>{}),0,{1},1};};
const vector<function<CC()>> NO_CTRL = {};

void pushSJob(vector<Instrument>& insts);
void pushCCJob(vector<Instrument>& insts);
inline Notes checkPlayingFunctionChanges(function<Notes()>& newFunc, function<Notes()>& currentFunc);
int taskDo(vector<Instrument>& insts);
int ccTaskDo(vector<Instrument>& insts);

std::vector<Instrument> insts;

Instrument& i(uint8_t ch);
void bpm(int _bpm);
void bpm();
uint32_t sync(uint8_t dur);
uint32_t isync(uint8_t ch);
uint32_t ccsync(uint8_t ch);
uint32_t playhead();
void mute();
void mute(int inst);
void solo(int inst);
void unmute();
void unmute(int inst);
void noctrl();
void stop();
void zeroone();
void on();
void off();