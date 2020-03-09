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
#include <math.h>
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
using rhythmType = vector<int>;
using label = int;

#define i(ch) (insts[ch-1])
#define sync insts[insts.size()-1].step // sync to metronome
#define isync(ch) (insts[ch-1].step)
#define ccsync(ch) (insts[ch-1].ccStep)
#define f(x) [&](){return x;}
//#define n(c,a,d,o) [&]()->Notes{return (Notes){(vector<int>c),a,d,o};} // polyphonic
#define n(c,a,d,o) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<int> d),o};} // polyphonic
#define cc(ch,value) [&]()->CC{return (CC){ch,value};} // lambda values
//#define ccs(_cc...) vector<function<CC()>>{(function<CC()>)_cc}
//#define nocc vector<function<CC()>>{}

const short NUM_TASKS = 5;
const float BAR_DUR_REF = 4000000; // microseconds
const float BPM_REF = 60;
const int REST_NOTE = 127;

struct Notes {
  std::vector<int> notes;
  float amp;
  vector<int> dur;
  int oct;
  
  void print() {
    std::cout << "{ ";
    for_each(notes.begin(),notes.end(),[](int n){std::cout << n << " ";});
    std::cout << "}";
    std::cout << amp << " { ";
    for_each(dur.begin(),dur.end(),[](int d){
      std::cout << d << " ";
    });
    std::cout << "}" << oct << " " << endl;
  }
};

function<Notes()> silence = []()->Notes {return {(vector<int>{0}),0,{4,4,4,4},1};};

struct CC {
  int ch;
  int value;
  void print() {
    std::cout << "cc { ";
    std::cout << ch << "," << value;
    std::cout << "}" << std::endl;
  }
};

struct SJob {
  int id;
  std::function<Notes()>* job;
};

struct CCJob {
  int id;
  std::vector<std::function<CC()>>* job;
};

template<typename T>
struct TaskPool {
  TaskPool() {
    isRunning = true;
  }
  
  vector<future<int>> tasks;
  short numTasks;
  deque<T> jobs;
  
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
      return (n != REST_NOTE ? notes.oct*12+scale[n%scale.size()] : n);
    });
    
    notes.amp = ampToVel(notes.amp);
    notes.dur = parseDurPattern(fn);
    
    transform(notes.dur.begin(), notes.dur.end(), notes.dur.begin(), [&](int d){
      return static_cast<int>(duration[d]);
    });
    
    notes.oct = static_cast<int>(notes.oct);
    
    return notes;
  }
  
  static std::vector<CC> midiCC(const vector<function<CC()>>& ccsFn) {
    std::vector<CC> ccValues;
    
    for(auto& _ccFn : ccsFn)
      ccValues.push_back(_ccFn());
  
    return ccValues;
  }
  
  static std::atomic<float> bpmRatio() {
    return {bpm/BPM_REF};
  }
  
  static int barDurMs() {
    return BAR_DUR_REF/(bpm/BPM_REF);
  }
  
  static int barDurMs(const float _bpm) {
    if (_bpm > 0) {
      bpm = _bpm;
      return static_cast<unsigned int>(BAR_DUR_REF/(bpm/BPM_REF));
    }
    return bpm;
  }
  
  static vector<int> parseDurPattern(const std::function<Notes(void)>& fn) {
    Notes notes = fn();
    float accDurTotal = 0;
    short offset = 5;
    vector<int> tempDur;
    
    if (notes.dur.size() == 1) {
      notes.dur.resize(BAR_DUR_REF/duration[(notes.dur.front())]);
      fill(notes.dur.begin(),notes.dur.end(),notes.dur.front());
      return notes.dur;
    }
    
    // --- All time figures explicit
    for (int d : notes.dur)
      accDurTotal += duration[d]/(bpm/BPM_REF);
    
    if (accDurTotal >= barDurMs()-offset && accDurTotal <= barDurMs()+offset)
      return notes.dur;
    // ---
    
    accDurTotal = 0;
    // --- Short-typed note duration pattern. Eg. short-typed {4,3,6,16} parses into {4,3,3,3,6,6,6,6,6,6,16,16,16,16}
    for (int d : notes.dur) {
      auto numDurFigure = (d < 3 ? static_cast<int>(ceil(static_cast<double>(duration[4])/duration[d])) : duration[4]/duration[d]);
      
      for (int i = 0;i < numDurFigure; ++i)
        tempDur.push_back(d);
    }
    
    for (int d : tempDur)
      accDurTotal += duration[d]/(bpm/BPM_REF);

    if (accDurTotal >= barDurMs()-offset && accDurTotal <= barDurMs()+offset)
      return tempDur;
    // ---
    
    return {};
  }
  
  static float bpm;
  static vector<int> durPattern;
  static scaleType scale;
  
private:
  static int ampToVel(float amp) {
    return round(127*amp);
  }
  
  static unordered_map<int,int> duration;
  //{noteDur(1,4000000),noteDur(2,2000000),noteDur(4,1000000),noteDur(8,500000),noteDur(3,333333),noteDur(16,250000),noteDur(6,166667),noteDur(32,125000),noteDur(64,62500)};
};

class Instrument {
public:
  Instrument(int id) {
    this->id = id;
    _ch = id;
    ccs = make_shared<std::vector<std::function<CC()>>>((std::vector<std::function<CC()>>){[&]()->CC{return (CC){127,0};}});
  }
  
  void play(function<Notes(void)> fn) {
    if (!Generator::parseDurPattern(fn).empty())
      *f = fn;
  }
  
  /*
  void ctrl(vector<function<CC()>> _ccs) {
    ccs = _ccs;
  }*/
  
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
  unsigned long step = 0, ccStep = 0;
  shared_ptr<function<Notes(void)>> f = make_shared<function<Notes(void)>>([]()->Notes{return {{0},0,{4,4,4,4},1};}); // 1/4 note silence
  shared_ptr<vector<function<CC()>>> ccs;
  
private:
  int _ch;
  bool _mute = false;
  bool recurCCStart = true;
  vector<function<CC()>> _ccs{};
};

/*
//void pushSJob(TaskPool<SJob>& tp,vector<Instrument>& insts) {
  SJob j;
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

void pushCCJob(TaskPool<CCJob>& tp,vector<Instrument>& insts) {
  CCJob j;
  int id = 0;
  
  while (tp.isRunning) {
    if (tp.jobs.size() < 20) {
      id = id%insts.size();
      j.id = id;
      j.job = &*insts.at(id).ccs;

      tp.jobs.push_back(j);
      
      id++;
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}
*/

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


/*
int taskDo(TaskPool<SJob>& tp,vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  unsigned long startTime, elapsedTime, deltaTime;
  vector<unsigned char> noteMessage;
  SJob j;
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
        
        // Every note param imediate change allowed except duration
        if (dur4Bar != Generator::midiNote(*j.job).dur)
          n = Generator::midiNote(nFunc);
        else
          n = Generator::midiNote(*j.job);
        
        
        for (auto& pitch : n.notes) {
          noteMessage[0] = 144+j.id;
          noteMessage[1] = pitch;
          noteMessage[2] = (pitch != REST_NOTE && !insts[j.id].isMuted()) ? n.amp : 0;
          midiOut.sendMessage(&noteMessage);
        }
        
        insts.at(j.id).step++;
        if (!tp.isRunning) break;
        
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

int ccTaskDo(TaskPool_<CCJob>& tp,vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  unsigned long startTime, elapsedTime;
  vector<unsigned char> ccMessage;
  CCJob j;
  vector<CC> ccComputed;
  
  midiOut.openPort(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  
  while (tp.isRunning) {
    if (!tp.jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      tp.mtx.lock();
      j = tp.jobs.front();
      tp.jobs.pop_front();
      tp.mtx.unlock();
      
      auto ccs = *j.job;
      ccComputed = Generator::midiCC(ccs);
      
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
*/

int taskDo(vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  unsigned long startTime, elapsedTime, deltaTime;
  vector<unsigned char> noteMessage;
  SJob j;
  Notes n;
  
  midiOut.openPort(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  while (TaskPool_<SJob>::isRunning) {
    if (!TaskPool_<SJob>::jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      TaskPool_<SJob>::mtx.lock();
      j = TaskPool_<SJob>::jobs.front();
      TaskPool_<SJob>::jobs.pop_front();
      TaskPool_<SJob>::mtx.unlock();
      
      TaskPool_<SJob>::step += 1;//TaskPool_<SJob>::step/(TaskPool_<SJob>::numTasks-1);
      
      auto nFunc = *j.job;
      auto dur4Bar = Generator::midiNote(nFunc).dur;
      auto durationsPattern = Generator::midiNote(*j.job).dur;
      
      elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      deltaTime = elapsedTime-startTime;
      
      for (auto& dur : durationsPattern) {
        startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count()-deltaTime;
        
        // Every note param imediate change allowed except duration
        if (dur4Bar != Generator::midiNote(*j.job).dur)
          n = Generator::midiNote(nFunc);
        else
          n = Generator::midiNote(*j.job);
        
        
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
  vector<CC> ccComputed;
  
  midiOut.openPort(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  
  while (TaskPool_<CCJob>::isRunning) {
    if (!TaskPool_<CCJob>::jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      TaskPool_<CCJob>::mtx.lock();
      j = TaskPool_<CCJob>::jobs.front();
      TaskPool_<CCJob>::jobs.pop_front();
      TaskPool_<CCJob>::mtx.unlock();
      
      auto ccs = *j.job;
      ccComputed = Generator::midiCC(ccs);
      
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


TaskPool<SJob> sTskPool;
TaskPool<CCJob> ccTskPool;
vector<Instrument> insts;

void bpm(int _bpm) {
  Generator::barDurMs(_bpm);
}

void bpm() {
  std::cout << (int)Generator::bpm << " bpm" << endl;
}

// metro is alias to an instrument automatically added last and set to 1/4 notes beat by default
unsigned long metro(int d) {
  insts.at(insts.size()-1).play(n({0},0,{d},1));
  return insts.at(insts.size()-1).step;
}

unsigned long metro() {
  return insts.at(insts.size()-1).step;
}

void mute() {
  for (auto &inst : insts)
    inst.mute();
}

void mute(int inst) {
  insts[inst-1].mute();
}

void unmute() {
  for (auto &inst : insts)
    inst.unmute();
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
  sTskPool.stopRunning();
  ccTskPool.stopRunning();
  cout << "mcode off <>" << endl;
}

float Generator::bpm = BPM_REF;
scaleType Generator::scale = Chromatic;
rhythmType Generator::durPattern{4};
//unordered_map<int,unsigned long> Generator::duration{noteDurMs(1,4000000),noteDurMs(2,2000000),noteDurMs(4,1000000),noteDurMs(8,500000),noteDurMs(3,333333),noteDurMs(16,250000),noteDurMs(6,166666),noteDurMs(32,125000),noteDurMs(64,62500)};

unordered_map<int,int> Generator::duration{noteDurMs(1,4000000),noteDurMs(2,2000000),noteDurMs(4,1000000),noteDurMs(8,500000),noteDurMs(3,333333),noteDurMs(16,250000),noteDurMs(6,166666),noteDurMs(32,125000),noteDurMs(64,62500)};

void wide() {
  if (sTskPool.isRunning) {
    thread th = std::thread([&](){
      //sTskPool.numTasks = NUM_TASKS+1; // +1 is metronome
      TaskPool_<SJob>::numTasks = NUM_TASKS+1;
      //ccTskPool.numTasks = NUM_TASKS;
      TaskPool_<CCJob>::numTasks = NUM_TASKS;
      
      // init instruments
//      for (int id = 0;id < sTskPool.numTasks;++id)
      for (int id = 0;id < TaskPool_<SJob>::numTasks;++id)
        insts.push_back(Instrument(id));
      
//      auto futPushSJob = async(launch::async,pushSJob,ref(sTskPool),ref(insts));
      auto futPushSJob = async(launch::async,pushSJob,ref(insts));
//      auto futPushCCJob = async(launch::async,pushCCJob,ref(ccTskPool),ref(insts));
      auto futPushCCJob = async(launch::async,pushCCJob,ref(insts));
      
      // init sonic task pool
      for (int i = 0;i < sTskPool.numTasks;++i)
        //sTskPool.tasks.push_back(async(launch::async,taskDo,ref(sTskPool),ref(insts)));
        TaskPool_<SJob>::tasks.push_back(async(launch::async,taskDo,ref(insts)));
      
      // init cc task pool
      for (int i = 0;i < ccTskPool.numTasks;++i)
        //ccTskPool.tasks.push_back(async(launch::async,ccTaskDo,ref(ccTskPool),ref(insts)));
        TaskPool_<SJob>::tasks.push_back(async(launch::async,ccTaskDo,ref(insts)));

    });
    th.detach();
    
    cout << "mcode on <((()))>" << endl;
  }
}

int main() {
  return 0;
}

