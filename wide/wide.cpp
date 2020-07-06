//
//  wide - Live coding DSLish API MIDI sequencer
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

#ifdef __linux__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#elif __APPLE__
  #pragma cling load("$DYLD_LIBRARY_PATH/librtmidi.dylib")
#elif __unix__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
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
#define n(c,a,d,o) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<int> d),o};} // polyphonic
#define cc(ch,value) [&]()->CC{return (CC){ch,value};}
#define bar Metro::sync(Metro::metroPrecision)

const char* PROJ_NAME = "[w]AVES [i]N [d]ISTRESSED [e]NTROPY";
const uint16_t NUM_TASKS = 5;
const float BAR_DUR_REF = 4000000; // microseconds
const float BPM_REF = 60;
const int   REST_NOTE = 127;

void pushSJob(vector<Instrument>& insts) {
  SJob j;
  int id = 0;
  
  while (TaskPool_<SJob>::isRunning) {
    if (TaskPool_<SJob>::jobs.size() < 20) {
      id = id%insts.size();
      j.id = id;
      j.job = &*insts.at(id).f;
      
      TaskPool_<SJob>::jobs.push_back(j);
      
      id++;
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

void pushCCJob(vector<Instrument>& insts) {
  CCJob j;
  int id = 0;
  
  while (TaskPool_<CCJob>::isRunning) {
    if (TaskPool_<CCJob>::jobs.size() < 20) {
      id = id%insts.size();
      j.id = id;
      j.job = &*insts.at(id).ccs;
      
      TaskPool_<CCJob>::jobs.push_back(j);
      
      id++;
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

int taskDo(vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  unsigned long startTime, elapsedTime, deltaTime;
  vector<unsigned char> noteMessage;
  SJob j;
  Notes n;
  function<Notes()> nFunc;
  vector<int> dur4Bar, durationsPattern;
  
  midiOut.openPort(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  while (TaskPool_<SJob>::isRunning) {
    if (!TaskPool_<SJob>::jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      
      std::unique_lock<mutex> lock(TaskPool_<SJob>::mtx, std::try_to_lock);
      if (lock.owns_lock()) {
        j = TaskPool_<SJob>::jobs.front();
        TaskPool_<SJob>::jobs.pop_front();
        lock.unlock();
        
        nFunc = *j.job;
        dur4Bar = Generator::midiNote(nFunc).dur;
        durationsPattern = Generator::midiNote(*j.job).dur;
        
      } else {
        durationsPattern = {};
      }
      
      elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      deltaTime = elapsedTime-startTime;
      
      for (auto& dur : durationsPattern) {
        startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count()-deltaTime;
        
        // Every note param imediate change allowed except duration
        if (dur4Bar != Generator::midiNote(*j.job).dur)
          n = Generator::midiNote(nFunc);
        else
          n = Generator::midiNote(*j.job);
        
        insts[j.id].out = n;
        
        for (auto& pitch : n.notes) {
          noteMessage[0] = 144+j.id;
          noteMessage[1] = pitch;
          noteMessage[2] = (pitch != REST_NOTE && !insts[j.id].isMuted()) ? n.amp : 0;
          midiOut.sendMessage(&noteMessage);
        }
        
        insts.at(j.id).step++;
        if (!TaskPool_<SJob>::isRunning) break;

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
    }
  }
  
  // silencing playing notes before exit
  noteMessage[0] = 128+j.id;
  midiOut.sendMessage(&noteMessage);
  
  return j.id;
}

int ccTaskDo(vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  unsigned long startTime, elapsedTime;
  vector<unsigned char> ccMessage;
  CCJob j;
  std::vector<std::function<CC()>> ccs;
  vector<CC> ccComputed;
  
  midiOut.openPort(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  
  while (TaskPool_<CCJob>::isRunning) {
    if (!TaskPool_<CCJob>::jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      
      std::unique_lock<mutex> lock(TaskPool_<SJob>::mtx, std::try_to_lock);
      if (lock.owns_lock()) {
        j = TaskPool_<CCJob>::jobs.front();
        TaskPool_<CCJob>::jobs.pop_front();
        lock.unlock();
        
        ccs = *j.job;
        ccComputed = Generator::midiCC(ccs);
        
      } else {
        ccComputed = {};
      }
      
      for (auto &cc : ccComputed) {
        ccMessage[0] = 176+j.id;
        ccMessage[1] = cc.ch;
        ccMessage[2] = cc.value;
        midiOut.sendMessage(&ccMessage);
      }
      
      insts.at(j.id).ccStep++;
      
      elapsedTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      this_thread::sleep_for(chrono::milliseconds(100-(elapsedTime-startTime)));
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

void off() {
  TaskPool_<SJob>::stopRunning();
  TaskPool_<CCJob>::stopRunning();
  cout << PROJ_NAME << " off <>" << endl;
}

function<Notes()> silence = []()->Notes {return {(vector<int>{0}),0,{4,4,4,4},1};};
scaleType Generator::scale = Chromatic;

void wide() {
  if (TaskPool_<SJob>::isRunning) {
    thread th = std::thread([&](){
      TaskPool_<SJob>::numTasks = NUM_TASKS;
      TaskPool_<CCJob>::numTasks = NUM_TASKS;
      
      // init instruments
      for (int id = 0;id < TaskPool_<SJob>::numTasks;++id)
        insts.push_back(Instrument(id));
      
      Metro::setInst(insts.at(insts.size()-1)); // set last instrument as a metronome
    
      auto futPushSJob = async(launch::async,pushSJob,ref(insts));
      auto futPushCCJob = async(launch::async,pushCCJob,ref(insts));
      
      // init sonic task pool
      for (int i = 0;i < TaskPool_<SJob>::numTasks;++i)
        TaskPool_<SJob>::tasks.push_back(async(launch::async,taskDo,ref(insts)));
      
      // init cc task pool
      for (int i = 0;i < TaskPool_<CCJob>::numTasks;++i)
        TaskPool_<CCJob>::tasks.push_back(async(launch::async,ccTaskDo,ref(insts)));
    });
    th.detach();
    
    cout << PROJ_NAME << " on <((()))>" << endl;
  }
}

int main() {
  return 0;
}

