//
//  wide
//
//  Created by @pd3v_
//

#include "sequencer.h"
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
//#define n(n,v,d,o) [&](){return std::vector<float>{n,v,d,o};} // monophonic
#define n(c,v,d,o) [&]()->Notes {return (Notes){(std::vector<int>c),v,d,o};} // polyphonic. Example for C7 chord: i(0).play(n(({0,2,4,7}),100,4,5))
#define inotes(x) i(x).metaNotes
#define iscale(x) i(x).scale()
#define e(x) i(x).express // alias to time dependent helper functions..not good
#define ccin(ch,func) (cc_t){ch,[](){return func;}}
#define ccins(vct) std::vector<cc_t>vct
#define nocc std::vector<cc_t>{}

using namespace std::chrono;
using instrumentsMap = std::unordered_map<std::string, std::shared_ptr<Instrument>>;

const int NUM_INST = 1;

std::thread seqThread;
pthread_t seqThreadHandle;
instrumentsMap insts;
bool seqRun = true;

StepTimer stepTimer;
const long _refBeatDur = 1000000000; //nanoseconds <=> 1000 milliseconds
float _bpm = 60.0;
const int _beatDivision = 200;
long _beatDur = 60.0/_bpm*_refBeatDur; // nanoseconds
long _beatDivisionDur = _beatDur/_beatDivision; // nanoseconds

void printNotes(Notes n) {
  std::cout << "{";
  for (auto& _n : n.notes)
    std::cout << _n << " ";
  std::cout << "} " << n.amp << " " << n.dur << " " << n.oct << std::endl;
}

//Generator generator;

//void stepSequencer() {
//  unsigned long startTime, elapsedTime;
//  
//  while(seqRun) {
//    for (int i = 0;i < _beatDivision;i++) {
//      startTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();
//      
//      stepTimer.step = i%_beatDivision+1;
//      stepTimer.playhead += (i%_beatDivision) == 0 ? 1 : 0;
//    
//      for (auto &inst : insts) {
//        if (!inst.second->playing)
//          inst.second->playIt();
//      }
//      
//      elapsedTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();
//      std::this_thread::sleep_for(nanoseconds(_beatDivisionDur-(elapsedTime-startTime)));
//    }
//  }
//}

void stepSequencer() {
  RtMidiOut midiout = RtMidiOut();
  std::vector<unsigned char> noteMessage;
  unsigned long startTime, elapsedTime;
  Job j;
  Notes n;
  
  midiout.openPort(0);
  
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  while(Generator::tskPool.isRunning) {
    startTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();
    Generator::tskPool.mtx.lock();
    j = Generator::tskPool.jobs.front();
    Generator::tskPool.jobs.pop_front();
    Generator::tskPool.mtx.unlock();
    
//    n = (*j.job)();
    std::cout << &j.job << std::endl;
    
    //printNotes(n);
    
//      // Notes off
//      for (auto& pitch : n.notes) {
//        noteMessage[0] = 128 ;
//        noteMessage[1] = pitch;
//        noteMessage[2] = 0;
//        midiout.sendMessage(&noteMessage);
//      }
//      
//      //      n = _generator.notes();
//      //n = _generator.notesTskPool();
//      
//      for (auto& pitch : n.notes) {
//        noteMessage[0] = 144;
//        noteMessage[1] = pitch;
//        noteMessage[2] = n.amp;
//        midiout.sendMessage(&noteMessage);
//      }
//    
      elapsedTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();
      
      std::this_thread::sleep_for(nanoseconds(n.dur-(elapsedTime-startTime)));
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
  insts.insert(std::pair<std::string,std::shared_ptr<Instrument>>(id, static_cast<std::shared_ptr<Instrument>>(new Instrument(id, ch))));
}

void newinsts(std::vector<int> instsIds) {
  for (auto& instId : instsIds)
    newinst(std::to_string(instId), instId);
}

void sequencer() {
  if (seqThreadHandle == nullptr) {
    Generator::initPatterns();
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

void patterns() {
  for (auto& f : *Generator::patterns)
    std::cout << &f << std::endl;
}

//std::function<Notes(void)> computeJob(int idx) {
//  return Generator::patterns->at(idx);
//}

Notes computeJob(int idx) {
  return Generator::patterns->at(idx)();
}

void lst() {
  for (auto& inst : insts)
    std::cout << "instrument " << inst.first << " at " << &inst << std::endl;
    
  if (insts.size() == 0) std::cout << "no instruments." << std::endl;
}

void quit(int instId) {;
  instrumentsMap::const_iterator itr = insts.find(std::to_string(instId));
  if (itr != insts.end()) {
    insts.erase(itr);
  }
}

void quit() {
  insts.erase(insts.begin(), insts.end());
}

void scale(Instrument* i, std::vector<int>scale) {
  i->scale(scale);
}

void play(Instrument* inst, std::function<Notes(void)>f) {
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

void on() {
  seqRun = true;
  sequencer();
}

void off() {
  quit();
  seqRun = false;
  seqThreadHandle = nullptr;
  std::cout << "wide is off." << std::endl;
}

