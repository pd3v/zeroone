//
//  wide
//
//  Created by @pd3v_
//

#include "sequencer.h"
#include <stdio.h>
#include <iostream>
#include <thread>
#include <pthread.h>
#include <chrono>
#include <vector>
#include <RtMidi.h>
#include <unordered_map>
#include "generator.h"
#include "instrument.h"
#include "expression.hpp"

#pragma cling load("/usr/local/lib/librtmidi.dylib")

#define i(x) (*insts[std::to_string(x)])
#define n(n,v,d,o) [&](){return std::vector<float>{n,v,d,o};}
//#define c(c,v,d,o) [&](){return std::vector<float>{n,v,d,o};}
#define e(x) i(x).express // alias to time dependent helper functions..not good
#define ccin(ch,func) (cc_t){ch,[](){return func;}}
#define ccins(vct) std::vector<cc_t>vct
#define nocc std::vector<cc_t>{}

using instrumentsMap = std::unordered_map<std::string, Instrument*>;

const int NUM_INST = 5;

std::thread seqThread;
pthread_t seqThreadHandle;
instrumentsMap insts;

StepTimer stepTimer;
const long _refBeatDur = 1000000000; //nanoseconds <=> 1000 milliseconds
float _bpm = 60.0;
const int _beatDivision = 200;
long _beatDur = 60.0/_bpm*_refBeatDur; // nanoseconds
long _beatDivisionDur = _beatDur/_beatDivision; // nanoseconds

using namespace std::chrono;

void stepSequencer() {
  unsigned long startTime, elapsedTime;
  
  while(true) {
    for (int i = 0;i < _beatDivision;i++) {
      startTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();
      
      stepTimer.step = i%_beatDivision+1;
      stepTimer.playhead += (i%_beatDivision) == 0 ? 1 : 0;
    
      for(auto &inst : insts) {
        if (!inst.second->playing)
          inst.second->playIt();
      }
      
      elapsedTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();
      std::this_thread::sleep_for(nanoseconds(_beatDivisionDur-(elapsedTime-startTime)));
    }
  }
}

int bpm() {
  return _bpm;
}

void bpm(float bpm){
  _bpm = bpm;
  _beatDur = 60.0/bpm*_refBeatDur;
  _beatDivisionDur = _beatDur/_beatDivision;
  
  for(auto& inst : insts)
    inst.second->bpm(_bpm);
}

unsigned long int playhead(){
  return stepTimer.playhead;
}

int main() {
  return 0;
}

void newinst(std::string id, int ch) {
  Instrument* inst = new Instrument(id, ch);
  insts.insert(std::pair<std::string,Instrument*>(id,inst));
}

void newinsts(std::vector<int> instsIds) {
  for (auto& instId : instsIds)
    newinst(std::to_string(instId), instId);
}

void sequencer() {
  if (seqThreadHandle == nullptr) {
    seqThread = std::thread(stepSequencer);
    seqThreadHandle = seqThread.native_handle();
    seqThread.detach();
    std::cout << "wide is on..." << std::endl;
  } else {
    std::cout << "wide is already running..." << std::endl;
    std::cout << _bpm << " bpm " << stepTimer.playhead << " beats played" << std::endl;
    std::cout << insts.size() << " instruments playing." << std::endl;
  }
  
  // create NUM_INST instruments
  for (int i = 0;i < NUM_INST; i++)
    newinst(std::to_string(i),i);
}

void lst() {
  for (auto& inst : insts)
    std::cout << "instrument " << inst.first << " at " << &inst << std::endl;
    
  if (insts.size() == 0) std::cout << "no instruments." << std::endl;
}

void quit(int instId) {;
  instrumentsMap::const_iterator itr = insts.find(std::to_string(instId));
  if (itr != insts.end()) {
    delete itr->second;
    insts.erase(itr);
  }
}

void quit() {
  for(auto& inst : insts)
    delete inst.second;
  
  insts.clear();
}

void scale(Instrument* i, std::vector<int>scale) {
  i->scale(scale);
}

void play(Instrument* inst, std::function<std::vector<float>(void)>f) {
  inst->play(f);
}

void silence() {
  for (auto &inst : insts)
    inst.second->mute();
}

void sound() {
  for (auto &inst : insts)
    inst.second->unmute();
}

void solo(int instId) {
  std::string instIdStr = std::to_string(instId);
  for (auto &inst: insts) {
    if (inst.first != instIdStr)
      inst.second->mute();
    else
      inst.second->unmute();
  }
}

void unmute() {
  for (auto &inst : insts)
    inst.second->unmute();
}