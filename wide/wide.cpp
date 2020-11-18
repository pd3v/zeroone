//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//

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
#include "mnotation.h"

#ifdef __linux__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
  #pragma cling load("$LD_LIBRARY_PATH/libmnotation.dylib")
#elif __APPLE__
  #pragma cling load("$DYLD_LIBRARY_PATH/librtmidi.dylib")
  #pragma cling load("$DYLD_LIBRARY_PATH/libmnotation.dylib")
#elif __unix__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
  #pragma cling load("$LD_LIBRARY_PATH/libmnotation.dylib")
#endif

using namespace std;

using scaleType = vector<int>;
using chordType = vector<int>;
using rhythmType = vector<int>;
using label = int;

#define i(ch) (insts[ch-1])
#define isync(ch) (insts[ch-1].step)
#define ccsync(ch) (insts[ch-1].ccStep)
#define f(x) [&](){return x;}
#define n(c,a,d) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<int> d),1};}     // note's absolute value setting, no octave parameter
#define no(c,a,d,o) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<int> d),o};}  // note's setting with octave parameter
#define cc(ch,value) [&]()->CC{return (CC){ch,value};}
#define bar Metro::sync(Metro::metroPrecision)

const char* PROJ_NAME = "[w]AVES [i]N [d]ISTRESSED [en]TROPY";
const uint16_t NUM_TASKS = 5;
const float BAR_DUR_REF = 4000000; // microseconds
const float BPM_REF = 60;
const int   REST_NOTE = 127;
const function<Notes()> SILENCE = []()->Notes {return {(vector<int>{}),0,{1},1};};
const vector<function<CC()>> NO_CTRL = {};

void pushSJob(vector<Instrument>& insts) {
  SJob j;
  int id = 0;
    
  while (TaskPool<SJob>::isRunning) {
    if (TaskPool<SJob>::jobs.size() < 20) {
      id = id%insts.size();
      j.id = id;
      j.job = &*insts.at(id).f;
      
      TaskPool<SJob>::jobs.push_back(j);
      
      id++;
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

void pushCCJob(vector<Instrument>& insts) {
  CCJob j;
  int id = 0;
  
  while (TaskPool<CCJob>::isRunning) {
    if (TaskPool<CCJob>::jobs.size() < 20) {
      id = id%insts.size();
      j.id = id;
      j.job = &*insts.at(id).ccs;
      
      TaskPool<CCJob>::jobs.push_back(j);
      
      id++;
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

int taskDo(vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  std::unique_lock<mutex> lock(TaskPool<SJob>::mtx,std::defer_lock);
  unsigned long startTime, elapsedTime, deltaTime = 0;
  int16_t spuriousMs = 0;
  vector<unsigned char> noteMessage;
  SJob j;
  Notes n;
  function<Notes()> nFunc;
  vector<int> dur4Bar, durationsPattern;
  
  midiOut.openPort(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  while (TaskPool<SJob>::isRunning) {
    if (!TaskPool<SJob>::jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count()-deltaTime;
      
      lock.try_lock();
      
      if(lock.owns_lock()) {
        j = TaskPool<SJob>::jobs.front();
        TaskPool<SJob>::jobs.pop_front();
        lock.unlock();
        
        nFunc = *j.job;
        dur4Bar = Generator::midiNote(nFunc).dur;
        durationsPattern = Generator::midiNote(*j.job).dur;
        
        elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
        deltaTime = elapsedTime-startTime-spuriousMs;
        spuriousMs = 0;
        
        for (auto& dur : durationsPattern) {
          startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count()-deltaTime;
          
          // Every note param imediate change allowed except duration
          if (dur4Bar != Generator::midiNote(*j.job).dur)
            n = Generator::midiNote(nFunc);
          else {
            n = Generator::midiNote(*j.job);
            insts[j.id].out = Generator::protoNotes; // Notes object before converting to MIDI spec
          }
          
          for (auto& pitch : n.notes) {
            noteMessage[0] = 144+j.id;
            noteMessage[1] = pitch;
            noteMessage[2] = (pitch != REST_NOTE && !insts[j.id].isMuted()) ? n.amp : 0;
            midiOut.sendMessage(&noteMessage);
          }
          
          insts.at(j.id).step++;
          if (!TaskPool<SJob>::isRunning) goto finishTask;
          
          dur = dur/Generator::bpmRatio();
          
          elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
          this_thread::sleep_for(chrono::microseconds(dur-(elapsedTime-startTime)));
          
          startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
          for (auto& pitch : n.notes) {
            noteMessage[0] = 128+j.id;
            noteMessage[1] = pitch;
            noteMessage[2] = 0;
            midiOut.sendMessage(&noteMessage);
          }
          elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
          deltaTime = elapsedTime-startTime;
        }
      } else {
          this_thread::sleep_for(chrono::microseconds(1000));
          spuriousMs += 1000;
      }
    }
  }
  
  finishTask:
  // silencing playing notes before exit
  for (auto& pitch : n.notes) {
    noteMessage[0] = 128+j.id;
    noteMessage[1] = pitch;
    noteMessage[2] = 0;
    midiOut.sendMessage(&noteMessage);
  }
  
  return j.id;
}

int ccTaskDo(vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  std::unique_lock<mutex> lock(TaskPool<CCJob>::mtx,std::defer_lock);
  unsigned long startTime, elapsedTime;
  int16_t spuriousMs = 0;
  vector<unsigned char> ccMessage;
  CCJob j;
  std::vector<std::function<CC()>> ccs;
  vector<CC> ccComputed;
  
  midiOut.openPort(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  
  while (TaskPool<CCJob>::isRunning) {
    if (!TaskPool<CCJob>::jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      
      lock.try_lock();
      
      if (lock.owns_lock()) {
        j = TaskPool<CCJob>::jobs.front();
        TaskPool<CCJob>::jobs.pop_front();
        lock.unlock();
        
        ccs = *j.job;
        ccComputed = Generator::midiCC(ccs);
        
        for (auto &cc : ccComputed) {
          ccMessage[0] = 176+j.id;
          ccMessage[1] = cc.ch;
          ccMessage[2] = cc.value;
          midiOut.sendMessage(&ccMessage);
        }
        
        insts.at(j.id).ccStep++;
        
        elapsedTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
        this_thread::sleep_for(chrono::milliseconds(100-(elapsedTime-startTime-spuriousMs)));
        spuriousMs = 0;
      } else {
        this_thread::sleep_for(chrono::milliseconds(1));
        spuriousMs += 1;
      }
    }
  }
  
  return j.id;
}

vector<Instrument> insts;

void bpm(int _bpm) {
  Generator::barDurMs(_bpm);
}

void bpm() {
  cout << floor(Generator::bpm) << " bpm" << endl;
}

uint32_t sync(int dur) {
  return Metro::sync(dur);
}

uint32_t playhead() {
  return Metro::playhead();
}

void mute() {
  for (auto &inst : insts)
    inst.mute();
}

void mute(int inst) {
  insts[inst-1].mute();
}

void solo(int inst) {
  insts[inst-1].unmute();
  
  for (auto &_inst : insts)
    if (_inst.id != inst-1)
      _inst.mute();
}

void unmute() {
  for (auto &inst : insts)
    inst.unmute();
}

void unmute(int inst) {
  insts[inst-1].unmute();
}

void noctrl() {
  for (auto &inst : insts)
    inst.noctrl();
}

void stop() {
  mute();
  noctrl();
}

scaleType Generator::scale = Chromatic;
Notes nPlaying;

void wide() {
  if (TaskPool<SJob>::isRunning) {
    std::thread([&](){
      TaskPool<SJob>::numTasks = NUM_TASKS;
      TaskPool<CCJob>::numTasks = NUM_TASKS;
      
      // init instruments
      for (int id = 0;id < TaskPool<SJob>::numTasks;++id)
        insts.push_back(Instrument(id));
      
      Metro::setInst(insts.at(insts.size()-1)); // set last instrument as a metronome
    
      auto futPushSJob = async(launch::async,pushSJob,ref(insts));
      auto futPushCCJob = async(launch::async,pushCCJob,ref(insts));
      
      // init sonic task pool
      for (int i = 0;i < TaskPool<SJob>::numTasks;++i)
        TaskPool<SJob>::tasks.push_back(async(launch::async,taskDo,ref(insts)));
      
      // init cc task pool
      for (int i = 0;i < TaskPool<CCJob>::numTasks;++i)
        TaskPool<CCJob>::tasks.push_back(async(launch::async,ccTaskDo,ref(insts)));
    }).detach();
    
    cout << PROJ_NAME << " on <((()))>" << endl;
  }
}

void on(){
  if (!TaskPool<SJob>::isRunning) {
    bpm(60);
  
    for (auto &inst : insts) {
      inst.play(SILENCE);
      inst.noctrl();
    }

    TaskPool<SJob>::isRunning = true;
    TaskPool<CCJob>::isRunning = true;
    wide();
  } else {
      cout << PROJ_NAME << " : already : on <((()))>" << endl;
  }
}

void off() {
  TaskPool<SJob>::stopRunning();
  TaskPool<CCJob>::stopRunning();
  
  insts.clear();

  cout << PROJ_NAME << " off <>" << endl;
}

int main() {
  return 0;
}

