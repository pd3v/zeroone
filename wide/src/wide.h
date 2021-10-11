#include <iostream>
#include <vector>
#include <functional>
#include <deque>
#include <thread>
#include <future>
#include <chrono>
#include <mutex>
#include <algorithm>

#include "RtMidi.h"
#include "notes.hpp"
#include "taskpool.hpp"
#include "instrument.hpp"
#include "generator.hpp"
#include "expression.hpp"

#ifdef __linux__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#elif __APPLE__
  #pragma cling load("$DYLD_LIBRARY_PATH/librtmidi.dylib")
#elif __unix__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#endif

#define i(ch) (insts[(ch-1)])
#define i1 (insts[0])
#define i2 (insts[1])
#define i3 (insts[2])
#define i4 (insts[3])
#define i5 (insts[4])
#define isync(ch) (insts[ch-1].step)
#define ccsync(ch) (insts[ch-1].ccStep)
#define f(x) [&](){return x;}
#define n(c,a,d) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<int> d),1};}       // note's absolute value setting, no octave parameter
#define no(c,a,d,o) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<int> d),o};}    // note's setting with octave parameter
#define cc(ch,value) [&]()->CC{return (CC){ch,value};}
#define bar Metro::sync(Metro::metroPrecision)

const char* PROJ_NAME = "[w]AVES [i]N [d]ISTRESSED [en]TROPY";
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
void bpm(int _bpm);
void bpm();
uint32_t sync(int dur);
uint32_t playhead();
void mute();
void mute(int inst);
void solo(int inst);
void unmute();
void unmute(int inst);
void noctrl();
void stop();
// debugging funcs
int instson();
int isinst(int id);
Instrument& isinstaddress(int id);
//
void wide();
void on();
void off();

std::vector<Instrument> insts;