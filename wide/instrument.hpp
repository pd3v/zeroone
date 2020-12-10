//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//

#pragma once

#include <vector>
#include <functional>
#include <math.h>
#include "notes.hpp"
#include "generator.hpp"
#include "/Volumes/Data/Xcode Projects/chronometer/chronometer/chronometer.h"

extern const function<Notes()> SILENCE;
extern const vector<function<CC()>> NO_CTRL;

using namespace std;

class Instrument {
public:
  Instrument(int id) {
    this->id = id;
    _ch = id;
  }
  
  void play(function<Notes(void)> fn) {
    if (!Generator::parseDurPattern(fn).empty())
      *f = fn;
  }
  
  vector<int> outNotes() {
    return out.notes;
  }

  int outNotes(int notePos) {
    return out.notes.at(notePos);
  }
  
  double outAmp() {
    return out.amp;
  }
  
  vector<int> outDur() {
    return out.dur;
  }
  
  int outDur(int durPos) {
    return out.dur.at(durPos);
  }
  
  int outOct() {
    return out.oct;
  }
  
  void ctrl(){
    *ccs = _ccs;
    recurCCStart = true;
  }
  
  template <typename T,typename... U>
  void ctrl(T fn1,U... fn2) {
    if (recurCCStart) {
      _ccs.clear();
      recurCCStart = false;
    }
    
    _ccs.push_back(fn1);
    ctrl(fn2...);
  }
  
  void noctrl() {
    ccs->clear();
    _ccs.clear();
  }
  
  void mute() {
    _mute = true;
  }
  
  void unmute() {
    _mute = false;
  }
  
  bool isMuted() {
    return _mute;
  }
  
  int id;
  uint32_t step = 0, ccStep = 0;
  shared_ptr<function<Notes(void)>> const f = make_shared<function<Notes(void)>>(SILENCE);
  shared_ptr<vector<function<CC(void)>>> const ccs = make_shared<vector<function<CC(void)>>>(NO_CTRL);
  Notes out = {{0},0.,{1},1};

private:
  int _ch;
  bool _mute = false;
  bool recurCCStart = true;
  vector<function<CC()>> _ccs{};
};

// Metronome standalone, independent thread
struct Metro {
public:
  static vector<Instrument>* insts;
  static uint32_t step;
  static bool isRunning;
  
  static void setTick(int _tickPrecision) {
    tickPrecision = _tickPrecision;
  }
  
  static uint8_t tick() {
    return tickPrecision;
  }
  
  static bool playTask() {
    isRunning = true;
    
    while (isRunning) {
      step++;
      
      if (!isRunning) break;
      
      this_thread::sleep_for(chrono::microseconds(Generator::barDurMs()/tickPrecision));
    }
    
    return false;
  }
  
  static void start() {
     taskDo = async(launch::async,Metro::playTask);
  }
  
  static void stop() {
    isRunning = false;
    step = 0;
  }

  //FIXME: this range conversion sometimes returns the same value for two contiguous steps
  static uint32_t sync(int timeSignature) {
    return floor(step*(timeSignature/static_cast<float>(tickPrecision)));
  }
  
  static uint32_t playhead() {
    return sync(4);
  }
  
private:
  static uint16_t tickPrecision;
  static future<bool> taskDo;
};

vector<Instrument>* Metro::insts = nullptr;
uint16_t Metro::tickPrecision = 64;
uint32_t Metro::step = 0;
bool Metro::isRunning = true;
future<bool> Metro::taskDo;

