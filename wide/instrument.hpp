//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//

#pragma once

#include <vector>
#include <functional>
#include <math.h>
#include <numeric>
#include "notes.hpp"
#include "generator.hpp"

extern const uint16_t NUM_TASKS;
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

// Metronome standalone, in an independent thread
struct Metro {
public:
  static uint32_t step;
  static bool on;
  static std::future<int> metronomeTask;
  static std::vector<long> instsWaitingTimes;
  static unsigned long startTime, elapsedTime;
  
  static void setTick(int _tickPrecision) {
    tickPrecision = _tickPrecision;
  }
  
  static uint8_t tick() {
    return tickPrecision;
  }

  static int metronome() {
    long t = 0;
    
    while (on) {
      startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();

      step++;
      
      elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      t = static_cast<unsigned int>((Generator::barDur())/tickPrecision)-(elapsedTime-startTime);
      t = (t > 0 ? t : 0);
      
      this_thread::sleep_for(chrono::microseconds(t));
    }
    
    return 0;
  }
  
  static void start() {
    on = true;
    metronomeTask = async(launch::async,Metro::metronome);
  }
  
  static void stop() {
    on = false;
    metronomeTask.get();
    TaskPool<SJob>::yieldTaskCntr.store(0);
    step = 0;
  }
  
  // will attempt to metro sync SJobs' tasks/threads
  static int syncInstTask(int instId) {
    TaskPool<SJob>::yieldTaskCntr.store(++TaskPool<SJob>::yieldTaskCntr);
    
    while ((TaskPool<SJob>::yieldTaskCntr.load() >= 1 && TaskPool<SJob>::yieldTaskCntr.load() <= NUM_TASKS) && on) {
      this_thread::yield();
      if (TaskPool<SJob>::yieldTaskCntr.load() >= NUM_TASKS) TaskPool<SJob>::yieldTaskCntr.store(0);
    }
    
    return instId;
  }

  static long minWaitingTime() {
    long minWait = *min_element(instsWaitingTimes.begin(),instsWaitingTimes.end());
    return (minWait < 0 ? 0 : minWait);
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

uint16_t Metro::tickPrecision = 64;
uint32_t Metro::step = 0;
bool Metro::on = false;
std::future<int> Metro::metronomeTask;
unsigned long Metro::startTime = 0, Metro::elapsedTime = 0;
std::vector<long> Metro::instsWaitingTimes(NUM_TASKS,0);
