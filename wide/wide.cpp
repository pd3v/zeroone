//
//  wide - MIDI sequencer for live coding
//
//  Created by @pd3v_
//

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <deque>
#include <thread>
#include <future>
#include <chrono>
#include <mutex>
#include <algorithm>
#include "RtMidi.h"
#include "expression.hpp"

#ifdef __linux__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#elif __APPLE__
  #pragma cling load("$DYLD_LIBRARY_PATH/librtmidi.dylib")
#elif __unix__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#endif

using namespace std;

using noteDurMs = pair<int,float>;
using scaleType = vector<int>;
using chordType = vector<int>;
using rhyhtmType = vector<unsigned long>;

#define i(ch) (insts[ch])
#define istep(ch) (insts[ch].step)
//#define n(c,a,d,o) [&]()->Notes{return (Notes){(vector<int>c),a,d,o};} // polyphonic
#define n(c,a,d,o) [&]()->Notes{return (Notes){(chordType c),a,(rhyhtmType d),o};} // polyphonic

const short NUM_TASKS = 4;
const float BAR_DUR_REF = 4000000; // microseconds
const float BPM_REF = 60;

struct Notes {
  std::vector<int> notes;
  float amp;
  vector<unsigned long> dur;
  int oct;
  
  void print() {
    std::cout << "{ ";
    for_each(notes.begin(),notes.end(),[](int n){std::cout << n << " ";});
    std::cout << "}";
    std::cout << amp << " ";
    std::cout << "{ ";
    for_each(dur.begin(),dur.end(),[](int d){
      std::cout << d << " ";
    });
    std::cout << "}";
    std::cout << oct << " ";
    std::cout << std::endl;
  }
};

function<Notes()> silence = []()->Notes {return {(vector<int>{0}),0,{4,4,4,4},1};};

struct Job {
  int id;
  function<Notes()>* job;
};

struct TaskPool {
  TaskPool() {
    isRunning = true;
  }
  
  vector<future<int>> tasks;
  short numTasks;
  deque<Job> jobs;
  
  bool isRunning;
  mutex mtx;
  
  void stopRunning() {
    isRunning = false;
    
    for (auto& t : tasks)
      t.get();
    
    jobs.clear();
    tasks.clear();
  }
};

class Generator {
public:
  static Notes midiNote(const std::function<Notes(void)>& fn) {
    Notes notes = fn();
    
    transform(notes.notes.begin(), notes.notes.end(), notes.notes.begin(), [&](int n){
      return notes.oct*12+scale[n%scale.size()];
    });
    
    notes.amp = ampToVel(notes.amp);
    
    if (notes.dur.size() == 1) {
      notes.dur.resize(BAR_DUR_REF/duration[static_cast<int>(notes.dur.front())]);
      fill(notes.dur.begin(),notes.dur.end(),notes.dur.front());
    }
    
    transform(notes.dur.begin(), notes.dur.end(), notes.dur.begin(), [&](unsigned long d){
      return static_cast<unsigned long>(duration[static_cast<int>(d)]/(bpm/BPM_REF));
    });
    
    notes.oct = static_cast<int>(notes.oct);
    
    return notes;
  }
  
  static unsigned int barDur() {
    return BAR_DUR_REF/(bpm/BPM_REF);
  }
  
  
  static unsigned int barDur(const float _bpm) {
    bpm = _bpm;
    return static_cast<unsigned int>(BAR_DUR_REF/(bpm/BPM_REF));
  }
  
  static bool validateDurPattern(const std::function<Notes(void)>& fn) {
    Notes notes = fn();
    float accDurTotal = 0;
    short offset = 4;
    
    if (notes.dur.size() == 1) return true;
    
    for (auto& d : notes.dur)
      accDurTotal += duration[static_cast<int>(d)]/(bpm/BPM_REF);
      
    if (accDurTotal >= barDur()-offset && accDurTotal <= barDur()+offset) return true;
    
    return false;
  }

  static float bpm;
  static vector<unsigned long> durPattern;
  static scaleType scale;
  
private:
  static int ampToVel(float amp) {
    return round(127*amp);
  }
  
  
  static unordered_map<int,unsigned long> duration;// {noteDur(1,4000000),noteDur(2,2000000),noteDur(4,1000000),noteDur(8,500000),noteDur(3,333333),noteDur(16,250000),noteDur(6,166667),noteDur(32,125000),noteDur(64,62500)};
};

class Instrument {
public:
  Instrument(int id) {
    this->id = id;
    _ch = id;
  }
  
  void play(function<Notes(void)> fn) {
    if (Generator::validateDurPattern(fn))
      *f = fn;
  }
  
  int id;
  unsigned long step = 0;
  shared_ptr<function<Notes(void)>> f = make_shared<function<Notes(void)>>([]()->Notes{return {{0},0,{4,4,4,4},1};}); // 1/4 note silence
private:
  int _ch;
};

void pushJob(TaskPool& tp,vector<Instrument>& insts) {
  Job j;
  int id = 0;
  
  while (tp.isRunning) {
    if (tp.jobs.size() < 20) {
      id = id%insts.size();
      j.id = id;
      j.job = &*insts.at(id).f;
      
      tp.jobs.push_back(j);
      
      id++;
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

int taskDo(TaskPool& tp,vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  unsigned long startTime, elapsedTime, deltaTime;
  vector<unsigned char> noteMessage;
  Job j;
  Notes n;

  midiOut.openPort(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  while (tp.isRunning) {
    if (!tp.jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      tp.mtx.lock();
      j = tp.jobs.front();
      tp.jobs.pop_front();
      tp.mtx.unlock();
      
      auto nFunc = *j.job;
      auto dur4Bar = Generator::midiNote(nFunc).dur;
      auto durationsPattern = Generator::midiNote(*j.job).dur;
    
      elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      deltaTime = elapsedTime-startTime;
      
      for (auto& dur : durationsPattern) {
        startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count()-deltaTime;
        deltaTime = 0;
        
        // Every note param imediate change allowed except duration
        if (dur4Bar != Generator::midiNote(*j.job).dur)
          n = Generator::midiNote(nFunc);
        else
          n = Generator::midiNote(*j.job);

        for (auto& pitch : n.notes) {
          noteMessage[0] = 144+j.id;
          noteMessage[1] = pitch;
          noteMessage[2] = n.amp;
          midiOut.sendMessage(&noteMessage);
        }
        
        insts.at(j.id).step++;
        if (!tp.isRunning) break;
        
        elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
        this_thread::sleep_for(chrono::microseconds(dur-(elapsedTime-startTime)));
        
        for (auto& pitch : n.notes) {
          noteMessage[0] = 128+j.id;
          noteMessage[1] = pitch;
          noteMessage[2] = 0;
          midiOut.sendMessage(&noteMessage);
        }
      }
    }
  }
  
  // silencing playing notes before exit
  noteMessage[0] = 128+j.id;
  midiOut.sendMessage(&noteMessage);
  
  return j.id;
}

TaskPool tskPool;
vector<Instrument> insts;

void bpm(int _bpm) {
  Generator::barDur(_bpm);
}

void exit() {
  tskPool.stopRunning();
  cout << "wide is off <>" << endl;
}

float Generator::bpm = BPM_REF;
scaleType Generator::scale = Major;
rhyhtmType Generator::durPattern{4};
unordered_map<int,unsigned long> Generator::duration {noteDurMs(1,4000000),noteDurMs(2,2000000),noteDurMs(4,1000000),noteDurMs(8,500000),noteDurMs(3,333333),noteDurMs(16,250000),noteDurMs(6,166667),noteDurMs(32,125000),noteDurMs(64,62500)};

void wide() {
  if (tskPool.isRunning) {
    thread th = std::thread([&](){
      tskPool.numTasks = NUM_TASKS;
      
      // init instruments
      for (int id = 0;id < tskPool.numTasks;++id)
        insts.push_back(Instrument(id));
      
      auto futPushJob = async(launch::async,pushJob,ref(tskPool),ref(insts));
      
      // init task pool
      for (int i = 0;i < tskPool.numTasks;++i)
        tskPool.tasks.push_back(async(launch::async,taskDo,ref(tskPool),ref(insts)));
    });
    th.detach();
    
    cout << "wide is on <((()))>" << endl;
  }
}

int main() {
  return 0;
}

