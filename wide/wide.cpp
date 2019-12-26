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
using rhythmType = vector<unsigned long>;

#define i(ch) (insts[ch])
#define istep(ch) (insts[ch].step)
#define f(x) [&](){return x;}
#define ccstep(ch) (insts[ch].ccStep)
//#define n(c,a,d,o) [&]()->Notes{return (Notes){(vector<int>c),a,d,o};} // polyphonic
#define n(c,a,d,o) [&]()->Notes{return (Notes){(vector<int> c),a,(vector<unsigned long> d),o};} // polyphonic
#define cc(ch,value) [&]()->CC{return (CC){ch,value};} // lambda values
//#define ccs(_cc...) vector<function<CC()>>{(function<CC()>)_cc}
//#define nocc vector<function<CC()>>{}

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
    
    if (notes.dur.size() == 1) {
      notes.dur.resize(BAR_DUR_REF/duration[static_cast<int>(notes.dur.front())]);
      fill(notes.dur.begin(),notes.dur.end(),notes.dur.front());
    }
    
    transform(notes.dur.begin(), notes.dur.end(), notes.dur.begin(), [&](unsigned long d){
      return static_cast<unsigned long>(duration[static_cast<int>(d)]);
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
  
  static unsigned long barDurMs() {
    return BAR_DUR_REF/(bpm/BPM_REF);
  }
  
  static unsigned long barDurMs(const float _bpm) {
    if (_bpm > 0) {
      bpm = _bpm;
      return static_cast<unsigned int>(BAR_DUR_REF/(bpm/BPM_REF));
    }
    return bpm;
  }
  
  static bool isDurValid(const std::function<Notes(void)>& fn) {
    Notes notes = fn();
    float accDurTotal = 0;
    short offset = 4;
    
    if (notes.dur.size() == 1) return true;
    
    for (auto& d : notes.dur)
      accDurTotal += duration[static_cast<int>(d)]/(bpm/BPM_REF);
      
    if (accDurTotal >= barDurMs()-offset && accDurTotal <= barDurMs()+offset) return true;
    
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
    ccs = make_shared<std::vector<std::function<CC()>>>((std::vector<std::function<CC()>>){[&]()->CC{return (CC){127,0};}});
  }
  
  void play(function<Notes(void)> fn) {
    if (Generator::isDurValid(fn))
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

void pushSJob(TaskPool<SJob>& tp,vector<Instrument>& insts) {
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
        deltaTime = 0;
        
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

int ccTaskDo(TaskPool<CCJob>& tp,vector<Instrument>& insts) {
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


TaskPool<SJob> sTskPool;
TaskPool<CCJob> ccTskPool;
vector<Instrument> insts;

void bpm(int _bpm) {
  Generator::barDurMs(_bpm);
}

float bpmext(int _bpm) {
  bpm(_bpm);

  return round((_bpm-20.f)*127/(999-20));
}

void bpm() {
  std::cout << (int)Generator::bpm << " bpm" << endl;
}

void mute() {
  for (auto &inst : insts)
    inst.mute();
}

void unmute() {
  for (auto &inst : insts)
    inst.unmute();
}

void noctrl() {
  for (auto &inst : insts)
    inst.noctrl();
}

void off() {
  sTskPool.stopRunning();
  ccTskPool.stopRunning();
  cout << "wide off <>" << endl;
}

float Generator::bpm = BPM_REF;
scaleType Generator::scale = Chromatic;
rhythmType Generator::durPattern{4};
unordered_map<int,unsigned long> Generator::duration{noteDurMs(1,4000000),noteDurMs(2,2000000),noteDurMs(4,1000000),noteDurMs(8,500000),noteDurMs(3,333333),noteDurMs(16,250000),noteDurMs(6,166666),noteDurMs(32,125000),noteDurMs(64,62500)};

void wide() {
  if (sTskPool.isRunning) {
    thread th = std::thread([&](){
      sTskPool.numTasks = NUM_TASKS;
      ccTskPool.numTasks = NUM_TASKS;
      
      // init instruments
      for (int id = 0;id < sTskPool.numTasks;++id)
        insts.push_back(Instrument(id));
      
      auto futPushSJob = async(launch::async,pushSJob,ref(sTskPool),ref(insts));
      auto futPushCCJob = async(launch::async,pushCCJob,ref(ccTskPool),ref(insts));
      
      // init sonic task pool
      for (int i = 0;i < sTskPool.numTasks;++i)
        sTskPool.tasks.push_back(async(launch::async,taskDo,ref(sTskPool),ref(insts)));
      
      // init cc task pool
      for (int i = 0;i < ccTskPool.numTasks;++i)
        ccTskPool.tasks.push_back(async(launch::async,ccTaskDo,ref(ccTskPool),ref(insts)));

    });
    th.detach();
    
    cout << "wide on <((()))>" << endl;
  }
}

int main() {
  return 0;
}

