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
  
  std::string hey() {
    std::string ah = "this is ";
    ah += to_string(id);
    ah += ". Hey!";
    
    return ah;
  }
  
  void wait(bool _wait) {
    this->_wait = _wait;
  }
  
  bool wait() {
    return _wait;
  }
  
  void chrono(long long _chrono) {
    this->_chrono = _chrono;
  }
  
  long long chrono() {
    return _chrono;
  }
  
  int id;
  uint32_t step = 0, ccStep = 0;
  shared_ptr<function<Notes(void)>> const f = make_shared<function<Notes(void)>>(SILENCE);
  shared_ptr<vector<function<CC(void)>>> const ccs = make_shared<vector<function<CC(void)>>>(NO_CTRL);
  Notes out = {{0},0.,{1},1};
  bool _wait = false;
  long long _chrono = 0;
  
private:
  int _ch;
  bool _mute = false;
  bool recurCCStart = true;
  vector<function<CC()>> _ccs{};
//  bool _wait = false;
};




// Metro, an instrument wrapper struct with
// the specific job of being a metronome

struct _Metro {
public:
  static int tickPrecision;
  static Instrument* inst;
  
  static void setInst(Instrument& _inst) {
    inst = &_inst;
    
    // Instrument working as a metronome and ticks 64 times per bar
    function<Notes()> beatFunc = [&]()->Notes {return {(vector<int>{}),0,{tickPrecision},1};};
    inst->play(beatFunc);
  }
  
  static void setTick(int _tickPrecision) {
    tickPrecision = _tickPrecision;
  }
  
  //FIXME: this range conversion sometimes returns the same value for two contiguous steps
  static uint32_t sync(int timeSignature) {
    return floor(inst->step*(timeSignature/static_cast<float>(tickPrecision)));
  }
  
  static uint32_t playhead() {
    return sync(4);
  }
};

Instrument* _Metro::inst = nullptr;
int _Metro::tickPrecision = 64;

// Metronome standalone, independent thread
struct Metro {
public:
  static vector<Instrument>* insts;
  static condition_variable cv;
  static uint32_t step;
  static bool on;
  static bool waitOverall;
  
  static void setTick(int _tickPrecision) {
    tickPrecision = _tickPrecision;
  }
  
  static uint8_t tick() {
    return tickPrecision;
  }
  
  static void start() {
    on = true;
    
    std::thread([&](){
      //int waitCount = 0;
      
      while (on) {
        step++;
        /*
        waitCount = 0;
        waitOverall = false;
        
        for_each(insts->begin(),insts->end(),[&](auto& i){waitCount += (i.wait() == true) ? 1 : 0;});
        
        if (waitCount == NUM_TASKS)
          waitOverall = true;
        
        if (waitOverall) {
          cout << "entered notify.all()" << endl;
          std::lock_guard<std::mutex> lock (TaskPool<SJob>::mtx);
          waitOverall = false;
          for_each(insts->begin(),insts->end(),[](auto& i){i.wait(false);});
          cv.notify_all();
        }
        */
        
        this_thread::sleep_for(chrono::microseconds(Generator::barDurMs()/tickPrecision));
        
//        if (step%(tickPrecision*4) == 0) {
        if (step%tickPrecision == 0) {
          auto ah = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
          cout << step << " " << ah << " " << insts->at(0).chrono() << " " << (ah-insts->at(0).chrono())/1000. << endl;
        }
        
        
      }
    }).detach();
  }
  
  static void notify() {
    for_each(insts->begin(),insts->end(),[](auto& i){i.wait(false);});
    cv.notify_all();
  }
  
  static void stop() {
    on = false;
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
};

vector<Instrument>* Metro::insts = nullptr;
condition_variable Metro::cv;
uint16_t Metro::tickPrecision = 256;
uint32_t Metro::step = 0;
bool Metro::on = true;
bool Metro::waitOverall = false;

